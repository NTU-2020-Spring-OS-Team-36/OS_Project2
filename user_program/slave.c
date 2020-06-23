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

#include "common.h"

int main(int argc, char *argv[]) { 
	char buf[BUF_SIZE], filename[FILE_NUM][FILENAME_LEN], method[METHOD_LEN], ip[IP_LEN];
	int n_files;

	sscanf(argv[1], "%d", &n_files);
	assert(argc == n_files + 4);
	for (int i = 0; i < n_files; i++) {
		strncpy(filename[i], argv[2 + i], FILENAME_LEN);
	}
	strncpy(method, argv[argc - 2], METHOD_LEN);
	strncpy(ip, argv[argc - 1], IP_LEN);

	int dev_fd, file_fd;
	struct timespec start, end;

	if ((dev_fd = open("/dev/slave_device", O_RDWR)) <
	    0)  // should be O_RDWR for PROT_WRITE when mmap()
	{
		perror("failed to open /dev/slave_device");
		return errno;
	}

	if (ioctl(dev_fd, SLAVE_IOCTL_CREATESOCK, ip) ==
	    -1)  // connect to master in the device
	{
		perror("ioctl create slave socket error");
		return errno;
	}

	fprintf(stderr, "ioctl success\n");

	assert(clock_gettime(CLOCK_MONOTONIC, &start) == 0); 

	int64_t tot_file_size = 0;
	for (int i = 0; i < n_files; i++) { 
		if ((file_fd = open(filename[i], O_RDWR | O_CREAT | O_TRUNC, 0666)) < 0) {
			perror("failed to open input file");
			return errno;
		}

		int64_t file_size = 0;
		if (strcmp(method, "fcntl") == 0) {
			int64_t ret = 0;
			do {
				ret = read(dev_fd, buf, sizeof(buf));  // read from the the device
				write(file_fd, buf, ret);              // write to the input file
				file_size += ret;
			} while (ret > 0);
		} else if (strcmp(method, "mmap") == 0) {
			int64_t ret = 0;
			char *dev_mem = mmap(NULL, MMAP_BUF_SIZE, PROT_READ, MAP_SHARED, dev_fd, 0);
			do {
				assert((ret = ioctl(dev_fd, SLAVE_IOCTL_MMAP)) >= 0 &&
						"mmap receiving failed");
				assert(write(file_fd, dev_mem, ret) >= 0);
				file_size += ret;
			} while (ret > 0);
		} else {
			fprintf(stderr, "Operation not supported\n");
			return EXIT_FAILURE;
		}

		if (ioctl(dev_fd, SLAVE_IOCTL_EXIT) ==
				-1)  // end receiving data, close the connection
		{
			perror("ioctl client exits error");
			return errno;
		} 

		tot_file_size += file_size;
	}

	assert(clock_gettime(CLOCK_MONOTONIC, &end) == 0);
	double trans_time = ts_diff_to_milli(&start, &end);
	printf("Transmission time: %lf ms, File size: %ld bytes\n", trans_time,
			tot_file_size);
}
