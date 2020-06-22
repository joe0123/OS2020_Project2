#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <stdlib.h>
#include <assert.h>

#define PAGE_SIZE 4096
#define BUF_SIZE 512
#define MAP_SIZE (PAGE_SIZE * 50)

int main (int argc, char* argv[])
{
	char buf[BUF_SIZE];
	int i, j, dev_fd, file_fd;// the fd for the device and the fd for the input file
	size_t ret, file_size = 0, data_size = -1;
	char file_name[100][50], num_of_files[50];
	char method[20];
	char ip[20];
	struct timeval start;
	struct timeval end;
	double trans_time; //calulate the time between the device is opened and it is closed
	char *kernel_address, *file_address;
	void *dst, *src;
	long int file_size_sum=0;


	strcpy(num_of_files, argv[1]);
	int num_of_file=atoi(num_of_files);
	assert(argc == num_of_file + 4);
	for(int i=0;i<num_of_file;i++)
		strcpy(file_name[i], argv[i+2]);
	strcpy(method, argv[argc-2]);
	strcpy(ip, argv[argc-1]);



	if( (dev_fd = open("/dev/slave_device", O_RDWR)) < 0)//should be O_RDWR for PROT_WRITE when mmap()
	{
		perror("failed to open /dev/slave_device\n");
		return 1;
	}
	
	gettimeofday(&start ,NULL);

	//src = mmap(NULL, NPAGE * PAGE_SIZE , PROT_READ, MAP_SHARED, dev_fd, 0);
	src = mmap(NULL, MAP_SIZE, PROT_READ, MAP_SHARED, dev_fd, 0);
	for(int i=0;i<num_of_file;i++){
		file_size=0;
		if( (file_fd = open (file_name[i], O_RDWR | O_CREAT | O_TRUNC)) < 0)
		{
			perror("failed to open input file\n");
			return 1;
		}
		if(ioctl(dev_fd, 0x12345677, ip) == -1)	//0x12345677 : connect to master in the device
		{
			perror("ioclt create slave socket error\n");
			return 1;
		}

		write(1, "ioctl success\n", 14);
		switch(method[0])
		{
			case 'f': //fcntl : read()/write()
				do
				{
					ret = read(dev_fd, buf, sizeof(buf)); // read from the the device
					write(file_fd, buf, ret); //write to the input file
					file_size += ret;
				}while(ret > 0);
				break;
	        case 'm': //mmap: mmap()/memcpy()
				do
				{	
					ret = ioctl(dev_fd, 0x12345678);
					posix_fallocate(file_fd, file_size, ret);
					dst = mmap(NULL, ret, PROT_READ|PROT_WRITE, MAP_SHARED, file_fd, file_size);
					//printf("%d %d\n", ret, file_size);
					memcpy(dst, src, ret);
					if(munmap(dst, ret) == -1 && ret > 0){
						fprintf(stderr, "Cannot unmap\n");
						exit(9);
					}
					file_size += ret;
				} while(ret > 0);
				ftruncate(file_fd, file_size);
				break;
		}
		if(ioctl(dev_fd, 0x12345679) == -1)// end receiving data, close the connection
		{
			perror("ioclt client exits error\n");
			return 1;
		}
		close(file_fd);
		file_size_sum+=file_size;
	}
	gettimeofday(&end, NULL);
	trans_time = (end.tv_sec - start.tv_sec)*1000 + (end.tv_usec - start.tv_usec)*0.001;
	printf("Slave: Transmission time: %lf ms, File size: %ld bytes\n", trans_time, file_size_sum);
	ioctl(dev_fd, 0x111, src);
	if(munmap(src, MAP_SIZE) == -1 && ret > 0){
		fprintf(stderr, "Cannot unmap\n");
		exit(9);
	}
	close(dev_fd);


	return 0;
}


