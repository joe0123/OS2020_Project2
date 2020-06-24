#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/net.h>
#include <net/sock.h>
#include <asm/processor.h>
#include <linux/netdevice.h>
#include <linux/ip.h>
#include <linux/in.h>
#include <linux/uio.h>      // iov_iter
#include <linux/miscdevice.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/debugfs.h>
#include <linux/mm.h>
#include <asm/page.h>
#include <asm/siginfo.h>  //send_sig_info

#ifndef VM_RESERVED
#define VM_RESERVED   (VM_DONTEXPAND | VM_DONTDUMP)
#endif

#define slave_IOCTL_CREATESOCK 0x12345677
#define slave_IOCTL_MMAP 0x12345678
#define slave_IOCTL_EXIT 0x12345679

#define BUF_SIZE PAGE_SIZE
#define MAP_SIZE PAGE_SIZE
//#define MAP_SIZE (PAGE_SIZE * 50)

struct dentry  *file1;//debug file

typedef struct socket * ksocket_t;

//functions about kscoket are exported,and thus we use extern here
extern ksocket_t ksocket(int domain, int type, int protocol);
extern int kconnect(ksocket_t socket, struct sockaddr *address, int address_len);
extern ssize_t krecv(ksocket_t socket, void *buffer, size_t length, int flags);
extern int kclose(ksocket_t socket);
extern unsigned int inet_addr(char* ip);
extern char *inet_ntoa(struct in_addr *in); //DO NOT forget to kfree the return pointer

static int __init slave_init(void);
static void __exit slave_exit(void);

int slave_close(struct inode *inode, struct file *filp);
int slave_open(struct inode *inode, struct file *filp);
static long slave_ioctl(struct file *file, unsigned int ioctl_num, unsigned long ioctl_param);
ssize_t receive_msg(struct file *filp, char *buf, size_t count, loff_t *offp );
static int slave_mmap(struct file *filp, struct vm_area_struct *vma);

static mm_segment_t old_fs;
static ksocket_t sockfd_cli;//socket to the master server
static struct sockaddr_in addr_srv; //address of the master server

void mmap_open(struct vm_area_struct *vma) { /* do nothing */ }
void mmap_close(struct vm_area_struct *vma) { /* do nothing */ }


//#ifdef ASYNCHRONOUS
ssize_t async_receive_msg(struct kiocb *iocb, struct iov_iter *iter);
static struct workqueue_struct *io_wq;
//#endif


//file operations
static struct file_operations slave_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = slave_ioctl,
	.open = slave_open,
	.read = receive_msg,
	.read_iter = async_receive_msg,
	.mmap = slave_mmap,
	.release = slave_close
};

struct vm_operations_struct mmap_vm_ops = {
	.open = mmap_open,
	.close = mmap_close,
};

//device info
static struct miscdevice slave_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "slave_device",
	.fops = &slave_fops
};

static int __init slave_init(void)
{
	int ret;
	file1 = debugfs_create_file("slave_debug", 0644, NULL, NULL, &slave_fops);

	//register the device
	if( (ret = misc_register(&slave_dev)) < 0){
		printk(KERN_ERR "misc_register failed!\n");
		return ret;
	}

	//#ifdef ASYNCHRONOUS
	if(( io_wq = create_workqueue("slave_wq"))==NULL)
	{
		printk(KERN_ERR "create_workqueue slave_wq returned NULL\n");
		return -1;
	}
	printk(KERN_INFO "slave using asychronous");
	//#endif

	printk(KERN_INFO "slave has been registered!\n");

	return 0;
}

static void __exit slave_exit(void)
{
	misc_deregister(&slave_dev);
	//#ifdef ASYNCHRONOUS
	destroy_workqueue(io_wq);
	//#endif
	printk(KERN_INFO "slave exited!\n");
	debugfs_remove(file1);
}


int slave_close(struct inode *inode, struct file *filp)
{
	kfree(filp->private_data);
	return 0;
}

int slave_open(struct inode *inode, struct file *filp)
{
	filp->private_data = kmalloc(MAP_SIZE, GFP_KERNEL);
	return 0;
}

char buf[BUF_SIZE];
static long slave_ioctl(struct file *file, unsigned int ioctl_num, unsigned long ioctl_param)
{
	long ret = -EINVAL;

	int addr_len ;
	unsigned int i;
	size_t len, data_size = 0;
	struct page *p_print;
    char *tmp, ip[20];
	unsigned char *px;

	pgd_t *pgd;
	p4d_t *p4d;
	pud_t *pud;
	pmd_t *pmd;
	pte_t *ptep, pte;
	old_fs = get_fs();
	set_fs(KERNEL_DS);

	printk("slave device ioctl");

	switch(ioctl_num){
		case slave_IOCTL_CREATESOCK:// create socket and connect to master
			printk("slave device ioctl create socket");

			if(copy_from_user(ip, (char*)ioctl_param, sizeof(ip)))
				return -ENOMEM;

			sprintf(current->comm, "ksktcli");

			memset(&addr_srv, 0, sizeof(addr_srv));
			addr_srv.sin_family = AF_INET;
			addr_srv.sin_port = htons(2325);
			addr_srv.sin_addr.s_addr = inet_addr(ip);
			addr_len = sizeof(struct sockaddr_in);

			sockfd_cli = ksocket(AF_INET, SOCK_STREAM, 0);
			printk("sockfd_cli = 0x%p  socket is created\n", sockfd_cli);
			if (sockfd_cli == NULL)
			{
				printk("socket failed\n");
				return -1;
			}
			if (kconnect(sockfd_cli, (struct sockaddr*)&addr_srv, addr_len) < 0)
			{
				printk("connect failed\n");
				return -1;
			}
			tmp = inet_ntoa(&addr_srv.sin_addr);
			printk("connected to : %s %d\n", tmp, ntohs(addr_srv.sin_port));
			kfree(tmp);
			printk("kfree(tmp)");

			ret = 0;
			break;
		case slave_IOCTL_MMAP:
			ret = krecv(sockfd_cli, file->private_data, MAP_SIZE, MSG_WAITALL);
			break;

		case slave_IOCTL_EXIT:
			if(kclose(sockfd_cli) == -1)
			{
				printk("kclose cli error\n");
				return -1;
			}
			ret = 0;
			break;
		default:
			pgd = pgd_offset(current->mm, ioctl_param);
			p4d = p4d_offset(pgd, ioctl_param);
			pud = pud_offset(p4d, ioctl_param);
			pmd = pmd_offset(pud, ioctl_param);
			ptep = pte_offset_kernel(pmd , ioctl_param);
			pte = *ptep;
			printk("slave: %lX\n", pte);
			ret = 0;
			break;
	}
	set_fs(old_fs);

	return ret;
}

char msg[BUF_SIZE];
ssize_t receive_msg(struct file *filp, char *buf, size_t count, loff_t *offp )
{
//call when user is reading from this device
	size_t len;
	len = krecv(sockfd_cli, msg, sizeof(msg), MSG_WAITALL);
	if(copy_to_user(buf, msg, len))
		return -ENOMEM;
	return len;
}


//#ifdef ASYNCHRONOUS
struct receive_msg_data{
    struct kiocb *iocb;
    struct iov_iter *iter;
    struct work_struct work;
};
void receive_msg_work(struct work_struct *work){
	struct receive_msg_data *param= container_of(work, struct receive_msg_data, work);
	int ret = 0;
	ret = receive_msg(param->iocb->ki_filp, param->iter->iov->iov_base, param->iter->iov->iov_len, &(param->iocb->ki_pos));
	/*
	//send signal
	sturct task_struct *t;
	if( (t = pid_task( find_vpid(pid), PIDTYPE_PID)) == NULL){
		printk(KERN_ERR "slave: pid_task returned NULL\n");
		return;
	}
	
	struct siginfo info;
	info.si_signo= 44;
	*/
	/* real-time signals should use SI_QUEUE */
	/*
	info.si_code = SI_QUEUE;
	info._si_fields._rt._sigval= {.sival_int .sival_ptr};
	send_sig_info(44, &info, t); 	//44, a real-time sigal
	*/
	param->iocb->ki_complete(param->iocb, ret, 0);
	return;
}

ssize_t async_receive_msg(struct kiocb *iocb, struct iov_iter *iter){
	struct receive_msg_data *my_data = (struct receive_msg_data*)kmalloc(sizeof(struct receive_msg_data), GFP_KERNEL);
	my_data->iocb = iocb;
	if( iocb->ki_complete == NULL)
	{
		printk(KERN_ERR "no ki_complete....................\n");
		return -1;
	}
	my_data->iter = iter;

   	INIT_WORK( &(my_data->work), receive_msg_work);
	queue_work(io_wq, &(my_data->work));
	return -EIOCBQUEUED;
}
//#endif


static int slave_mmap(struct file *filp, struct vm_area_struct *vma)
{
	/* Remap-pfn-range will mark the range VM_IO */
	if(remap_pfn_range(vma, vma->vm_start, virt_to_phys(filp->private_data)>>PAGE_SHIFT, vma->vm_end - vma->vm_start, vma->vm_page_prot) < 0)
		return -EIO;
	vma->vm_ops = &mmap_vm_ops;
	vma->vm_flags |= VM_RESERVED;
	vma->vm_private_data = filp->private_data;
	return 0;
}


module_init(slave_init);
module_exit(slave_exit);

MODULE_LICENSE("GPL");
