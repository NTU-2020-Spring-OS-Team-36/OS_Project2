#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define BUF_SIZE 4096
#define FILENAME_LEN 1024
#define METHOD_LEN 8
#define MASTER_IOCTL_CREATESOCK 0x12345677
#define MASTER_IOCTL_MMAP 0x12345678
#define MASTER_IOCTL_EXIT 0x12345679
#define PAGE_SIZE 4096  // TODO: Not hardcode this
#define MMAP_BUF_PAGES 1
#define MMAP_BUF_SIZE PAGE_SIZE *MMAP_BUF_PAGES

static size_t get_filesize(const char *filename) {
	struct stat st;
	stat(filename, &st);
	return st.st_size;
}

static double ts_diff_to_milli(struct timespec *t0, struct timespec *t1) {
	return (double)(t1->tv_sec - t0->tv_sec) * 1000 +
	       (double)(t1->tv_nsec - t0->tv_nsec) * 1e-6;
}

int main(int argc, char *argv[]) {
	char buf[BUF_SIZE], filename[FILENAME_LEN], method[METHOD_LEN];
	int n_files;

	sscanf(argv[1], "%d", &n_files);
	assert(n_files == 1);  // TODO
	strncpy(filename, argv[2], FILENAME_LEN);
	strncpy(method, argv[3], METHOD_LEN);

	int dev_fd, file_fd;
	struct timespec start, end;

	if ((dev_fd = open("/dev/master_device", O_RDWR)) < 0) {
		perror("failed to open /dev/master_device\n");
		return errno;
	}

	assert(clock_gettime(CLOCK_MONOTONIC, &start) == 0);

	if ((file_fd = open(filename, O_RDWR)) < 0) {
		perror("failed to open input file\n");
		return errno;
	}

	int64_t file_size = 0, offset;
	if ((file_size = get_filesize(filename)) < 0) {
		perror("failed to get filesize\n");
		return errno;
	}

	if (ioctl(dev_fd, MASTER_IOCTL_CREATESOCK) ==
	    -1)  // create socket and accept
	         // the connection from the slave
	{
		perror("ioctl server create socket error\n");
		return errno;
	}

	if (strcmp(method, "fnctl") == 0) {
		int64_t ret = 0;
		do {
			ret = read(file_fd, buf, sizeof(buf));  // read from the input file
			assert(write(dev_fd, buf, ret) >= 0);   // write to the the device
		} while (ret > 0);
	} else if (strcmp(method, "mmap") == 0) {
		int64_t ret = 0;
		char *dev_mem =
		    mmap(NULL, MMAP_BUF_SIZE, PROT_WRITE, MAP_SHARED, dev_fd, 0);
		do {
			ret = read(file_fd, dev_mem, MMAP_BUF_SIZE);  // read from the input file
			assert(ioctl(dev_fd, MASTER_IOCTL_MMAP, ret) >= 0 &&
			       "mmap sending failed");
		} while (ret > 0);
	} else {
		fprintf(stderr, "Operation not supported\n");
		return EXIT_FAILURE;
	}

	if (ioctl(dev_fd, MASTER_IOCTL_EXIT) ==
	    -1)  // end sending data, close the connection
	{
		perror("ioclt server exits error\n");
		return 1;
	}
	assert(clock_gettime(CLOCK_MONOTONIC, &end) == 0);

	double trans_time = ts_diff_to_milli(&start, &end);
	printf("Transmission time: %lf ms, File size: %ld bytes\n", trans_time,
	       file_size);
}
