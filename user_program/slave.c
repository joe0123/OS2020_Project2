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

#include <aio.h>
#include <signal.h>

#define PAGE_SIZE 4096
#define BUF_SIZE 512
//#define MAP_SIZE (PAGE_SIZE * 10)
#define MAP_SIZE PAGE_SIZE

void aio_completion_handler( int signo, siginfo_t *info, void *context );
struct aiocb_fd_pair{                 //for signal handler
	struct aiocb* aiocb;
	int file_fd;
};

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
		if(i==0)
			gettimeofday(&start ,NULL);

		write(1, "ioctl success\n", 14);


	struct aiocb my_aiocb;

	/* Zero out the aiocb structure (recommended) */
	bzero( (char *)&my_aiocb, sizeof(struct aiocb) );

	/* Allocate a data buffer for the aiocb request */
	my_aiocb.aio_buf = buf;

	/* Initialize the necessary fields in the aiocb */
	my_aiocb.aio_fildes = dev_fd;
	my_aiocb.aio_nbytes = BUF_SIZE;
	my_aiocb.aio_offset = 0;

	my_aiocb.aio_sigevent.sigev_notify = SIGEV_SIGNAL;
	my_aiocb.aio_sigevent.sigev_signo = SIGIO;
	struct aiocb_fd_pair my_love_ppp= {.aiocb=&my_aiocb, .file_fd=file_fd};
	my_aiocb.aio_sigevent.sigev_value.sival_ptr = &my_love_ppp;

	struct sigaction sig_act;
	/* Set up the signal handler */
	sigemptyset(&sig_act.sa_mask);
	sig_act.sa_flags = SA_SIGINFO;
	sig_act.sa_sigaction = aio_completion_handler;
	sigaction( SIGIO, &sig_act, NULL );

	ret = aio_read( &my_aiocb );
	if (ret < 0) perror("aio_read");

	while ( aio_error( &my_aiocb ) == EINPROGRESS ) ;

	if (aio_return( &my_aiocb ) > 0) {
		printf("good\n");;
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
	trans_time = (end.tv_sec - start.tv_sec)*1000 + (end.tv_usec - start.tv_usec)*0.0001;
	printf("Slave: Transmission time: %lf ms, File size: %ld bytes\n", trans_time, file_size_sum / 8);
	close(dev_fd);

	return 0;
}

 
void aio_completion_handler( int signo, siginfo_t *info, void *context )
{
	struct aiocb_fd_pair *req;


	/* Ensure it's our signal */
	if (info->si_signo == SIGIO) {

		req = (struct aiocb_fd_pair *)info->si_value.sival_ptr;

		/* Did the request complete? */
		if (aio_error( req->aiocb ) == 0) {

			/* Request completed successfully, get the return status */
			ssize_t ret = aio_return( req->aiocb );
			printf("%d\n", ret);
		
			write(req->file_fd, req->aiocb->aio_buf, ret);
		}

	}

	return;
}
