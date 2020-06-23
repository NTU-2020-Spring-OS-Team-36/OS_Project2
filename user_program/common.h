#define BUF_SIZE 512
#define FILENAME_LEN 1024
#define METHOD_LEN 8
#define IP_LEN 256
#define FILE_NUM 256

#define MASTER_IOCTL_CREATESOCK 0x12345677
#define MASTER_IOCTL_MMAP 0x12345678
#define MASTER_IOCTL_EXIT 0x12345679
#define SLAVE_IOCTL_CREATESOCK 0x12345677
#define SLAVE_IOCTL_MMAP 0x12345678
#define SLAVE_IOCTL_EXIT 0x12345679

#define PAGE_SIZE 4096  // TODO: Not hardcode this
#define MMAP_BUF_PAGES 1
#define MMAP_BUF_SIZE PAGE_SIZE * MMAP_BUF_PAGES

static double ts_diff_to_milli(struct timespec *t0, struct timespec *t1) {
	return (double)(t1->tv_sec - t0->tv_sec) * 1000 +
	       (double)(t1->tv_nsec - t0->tv_nsec) * 1e-6;
}
