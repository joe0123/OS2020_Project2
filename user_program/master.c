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

size_t get_filesize(const char* filename);//get the size of the input file

int main (int argc, char* argv[])
{
	char buf[BUF_SIZE];
	int i, j, dev_fd, file_fd;// the fd for the device and the fd for the input file
	size_t ret, file_size, offset = 0, tmp;
	char file_name[100][50], method[20], num_of_files[50];
	char *kernel_address = NULL, *file_address = NULL;
	struct timeval start;
	struct timeval end;
	double trans_time; //calulate the time between the device is opened and it is closed
	void *src, *dst;
	int len;
	long int file_size_sum=0;


	strcpy(num_of_files, argv[1]);
	int num_of_file=atoi(num_of_files);
	assert(argc == num_of_file + 3);
	for(int i=0;i<num_of_file;i++)
		strcpy(file_name[i], argv[i+2]);
	strcpy(method, argv[argc-1]);

	if( (dev_fd = open("/dev/master_device", O_RDWR)) < 0)
	{
		perror("failed to open /dev/master_device\n");
		return 1;
	}
	
	gettimeofday(&start ,NULL);

	//dst = mmap(NULL, NPAGE * PAGE_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, dev_fd, 0);
	dst = mmap(NULL, MAP_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, dev_fd, 0);
	for(int i=0;i<num_of_file;i++){
		if( (file_fd = open (file_name[i], O_RDWR)) < 0 )
		{
			perror("failed to open input file\n");
			return 1;
		}

		if( (file_size = get_filesize(file_name[i])) < 0)
		{
			perror("failed to get filesize\n");
			return 1;
		}
		if(ioctl(dev_fd, 0x12345677) == -1) //0x12345677 : create socket and accept the connection from the slave
		{
			perror("ioclt server create socket error\n");
			return 1;
		}

		switch(method[0])
		{
			case 'f': //fcntl : read()/write()
				do
				{
					ret = read(file_fd, buf, sizeof(buf)); // read from the input file
					write(dev_fd, buf, ret);//write to the the device
				}while(ret > 0);
				break;
	        case 'm': //mmap: mmap()/memcpy()
	        	offset=0;
				do{
					len = (MAP_SIZE < file_size - offset)? MAP_SIZE: (file_size - offset);
					src = mmap(NULL, len, PROT_READ, MAP_SHARED, file_fd, offset);					
					memcpy(dst, src, len);
					offset += len;
					ioctl(dev_fd, 0x12345678, len);
					if(munmap(src, len) == -1 && len > 0){
						fprintf(stderr, "Cannot unmap\n");
						exit(9);
					}
				}while(offset < file_size);
				break;
		}
		if(ioctl(dev_fd, 0x12345679) == -1) // end sending data, close the connection
		{
			perror("ioclt server exits error\n");
			return 1;
		}
		close(file_fd);
		file_size_sum+=file_size;
	}

	gettimeofday(&end, NULL);
	trans_time = (end.tv_sec - start.tv_sec)*1000 + (end.tv_usec - start.tv_usec)*0.001;
	ioctl(dev_fd, 0x111, dst);
	if(munmap(dst, MAP_SIZE) == -1 && len > 0){
		fprintf(stderr, "Cannot unmap\n");
		exit(9);
	}
	printf("Master: Transmission time: %lf ms, File size: %ld bytes\n", trans_time, file_size_sum);
	close(dev_fd);


	return 0;
}

size_t get_filesize(const char* filename)//get the size of the input file
{
	struct stat st;
	stat(filename, &st);
	return st.st_size;
}
