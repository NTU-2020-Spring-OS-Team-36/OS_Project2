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
#include <linux/miscdevice.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/debugfs.h>
#include <linux/mm.h>
#include <asm/page.h>
#include <linux/async.h>


#ifndef VM_RESERVED
#define VM_RESERVED   (VM_DONTEXPAND | VM_DONTDUMP)
#endif

#define slave_IOCTL_CREATESOCK 0x12345677
#define slave_IOCTL_MMAP 0x12345678
#define slave_IOCTL_EXIT 0x12345679
#define slave_IOCTL_MMAP_ASYNC_COMMIT 0x12345680
#define slave_IOCTL_MMAP_ASYNC_CHECK 0x12345681


#define BUF_SIZE 512

#define MMAP_BUF_PAGES_LOG 0
#define MMAP_BUF_SIZE PAGE_SIZE * (1UL << MMAP_BUF_PAGES_LOG)



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

int slave_mmap(struct file *filp, struct vm_area_struct *vma);

static mm_segment_t old_fs;
static ksocket_t sockfd_cli;//socket to the master server
static struct sockaddr_in addr_srv; //address of the master server

static struct page *buffer;
static async_cookie_t async_cookie;
static volatile long async_ret;

//file operations
static struct file_operations slave_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = slave_ioctl,
	.open = slave_open,
	.read = receive_msg,
	.release = slave_close,
	.mmap = slave_mmap
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

	buffer = alloc_pages(GFP_KERNEL, MMAP_BUF_PAGES_LOG);
	async_ret = -1;

	//register the device
	if( (ret = misc_register(&slave_dev)) < 0){
		printk(KERN_ERR "misc_register failed!\n");
		return ret;
	}

	printk(KERN_INFO "slave has been registered!\n");

	return 0;
}

static void __exit slave_exit(void)
{
	misc_deregister(&slave_dev);
	printk(KERN_INFO "slave exited!\n");
	debugfs_remove(file1);

	__free_pages(buffer, MMAP_BUF_PAGES_LOG);
}


int slave_close(struct inode *inode, struct file *filp)
{
	return 0;
}

int slave_open(struct inode *inode, struct file *filp)
{
	return 0;
}

static void recv_async(void *data, async_cookie_t cookie) {
	async_ret = krecv(sockfd_cli, page_to_virt(buffer), MMAP_BUF_SIZE, 0);
}

static long slave_ioctl(struct file *file, unsigned int ioctl_num, unsigned long ioctl_param)
{
	long ret = -EINVAL;

	int addr_len ;
	unsigned int i;
	size_t len, data_size = 0;
	char *tmp, ip[20], buf[BUF_SIZE];
	struct page *p_print;
	unsigned char *px;

    pgd_t *pgd;
	p4d_t *p4d;
	pud_t *pud;
	pmd_t *pmd;
    pte_t *ptep, pte;
	old_fs = get_fs();
	set_fs(KERNEL_DS);

    //printk("slave device ioctl");

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
			ret = krecv(sockfd_cli, page_to_virt(buffer), MMAP_BUF_SIZE, 0);
			break;
		case slave_IOCTL_MMAP_ASYNC_COMMIT:
			async_cookie = async_schedule(recv_async, NULL);
			async_ret = -1;
			ret = 0;
			break;
		case slave_IOCTL_MMAP_ASYNC_CHECK:
			ret = async_ret;
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

ssize_t receive_msg(struct file *filp, char *buf, size_t count, loff_t *offp )
{
//call when user is reading from this device
	char msg[BUF_SIZE];
	size_t len;
	len = krecv(sockfd_cli, msg, sizeof(msg), 0);
	if(copy_to_user(buf, msg, len))
		return -ENOMEM;
	return len;
}

int slave_mmap(struct file *filp, struct vm_area_struct *vma) {
	if (vma->vm_end - vma->vm_start > MMAP_BUF_SIZE) {
		pr_err("mmap requested size too large, aborting...");
	}
	vma->vm_flags |= VM_RESERVED;
	int pfn = page_to_pfn(buffer);
	int ret = remap_pfn_range(vma, vma->vm_start, pfn, vma->vm_end - vma->vm_start, vma->vm_page_prot);
	printk("%lX\n", buffer->flags);
	if (ret < 0) {
		return -EIO;
	}
	return 0;
}


module_init(slave_init);
module_exit(slave_exit);
MODULE_LICENSE("GPL");
