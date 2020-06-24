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

static size_t get_filesize(const char *filename) {
	struct stat st;
	stat(filename, &st);
	return st.st_size;
}

int main(int argc, char *argv[]) {
	char buf[BUF_SIZE], filename[FILE_NUM][FILENAME_LEN], method[METHOD_LEN];

	int n_files;
	sscanf(argv[1], "%d", &n_files);
	assert(argc == n_files + 3);
	for (int i = 0; i < n_files; i++) {
		strncpy(filename[i], argv[2 + i], FILENAME_LEN);
	}
	strncpy(method, argv[argc - 1], METHOD_LEN);

	int dev_fd, file_fd;
	struct timespec start, end;

	if ((dev_fd = open("/dev/master_device", O_RDWR)) < 0) {
		perror("failed to open /dev/master_device");
		return errno;
	}

	fprintf(stderr, "ioctl success\n");

	assert(clock_gettime(CLOCK_MONOTONIC, &start) == 0);

	int64_t tot_file_size = 0;
	for (int i = 0; i < n_files; i++) {
		// create socket and accept the connection from the slave
		if (ioctl(dev_fd, MASTER_IOCTL_CREATESOCK) == -1) {
			perror("ioctl server create socket error");
			return errno;
		}

		if ((file_fd = open(filename[i], O_RDWR)) < 0) {
			perror("failed to open input file");
			return errno;
		}

		int64_t file_size = 0, offset = 0;
		if ((file_size = get_filesize(filename[i])) < 0) {
			perror("failed to get filesize");
			return errno;
		}

		if (strcmp(method, "fcntl") == 0) {
			int64_t ret = 0;
			do {
				ret = read(file_fd, buf, sizeof(buf));  // read from the input file
				assert(write(dev_fd, buf, ret) >= 0);   // write to the the device
			} while (ret > 0);
		} else if (strcmp(method, "mmap") == 0) {
			char *dev_mem =
			    mmap(NULL, MMAP_BUF_SIZE, PROT_WRITE, MAP_SHARED, dev_fd, 0);
			while (offset < file_size) {
				int64_t remain = file_size - offset;
				int64_t len = remain < MMAP_BUF_SIZE ? remain : MMAP_BUF_SIZE;
				char *file_mem =
				    mmap(NULL, len, PROT_READ, MAP_SHARED, file_fd, offset);
				offset += len;
				memcpy(dev_mem, file_mem, len);
				assert(ioctl(dev_fd, MASTER_IOCTL_MMAP, len) >= 0 &&
				       "mmap sending failed");
				munmap(file_mem, len);
			}
		} else {
			fprintf(stderr, "Operation not supported");
			return EXIT_FAILURE;
		}

		// end sending data, close the connection
		if (ioctl(dev_fd, MASTER_IOCTL_EXIT) == -1) {
			perror("ioctl server exits error");
			return 1;
		}

		tot_file_size += file_size;
	}

	assert(clock_gettime(CLOCK_MONOTONIC, &end) == 0);
	double trans_time = ts_diff_to_milli(&start, &end);
	printf("Transmission time: %lf ms, File size: %ld bytes\n", trans_time,
	       tot_file_size);
}
