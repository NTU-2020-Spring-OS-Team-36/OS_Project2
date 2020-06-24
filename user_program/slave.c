#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "common.h"

int main(int argc, char *argv[]) {
	char buf[BUF_SIZE];

	int opt, method = -1;
	bool async = false;
	while ((opt = getopt(argc, argv, "fma")) != -1) {
		switch (opt) {
			case 'm':
				method = MMAP;
				break;
			case 'f':
				method = FCNTL;
				break;
			case 'a':
				async = true;
				break;
			default:
				fprintf(stderr, "Usage: slave [-fma] server files...\n");
				return EXIT_FAILURE;
		}
	}

	int dev_fd, file_fd;
	struct timespec start, end;

	// should be O_RDWR for PROT_WRITE when mmap()
	if ((dev_fd = open("/dev/slave_device", O_RDWR)) < 0) {
		perror("failed to open /dev/slave_device");
		return errno;
	}

	fprintf(stderr, "ioctl success\n");

	assert(clock_gettime(CLOCK_MONOTONIC, &start) == 0);

	const char *ip = argv[optind];
	int64_t tot_file_size = 0;
	for (int i = optind + 1; i < argc; i++) {
		const char *filename = argv[i];

		// connect to master in the device
		if (ioctl(dev_fd, SLAVE_IOCTL_CREATESOCK, ip) == -1) {
			perror("ioctl create slave socket error");
			return errno;
		}

		if ((file_fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0666)) < 0) {
			perror("failed to open input file");
			return errno;
		}

		int64_t file_size = 0;
		if (method == FCNTL) {
			int64_t ret = 0;
			do {
				ret = read(dev_fd, buf, sizeof(buf));  // read from the the device
				assert(write(file_fd, buf, ret) == ret);              // write to the input file
				file_size += ret;
			} while (ret > 0);
		} else if (method == MMAP) {
			int64_t ret = 0;
			char *dev_mem =
			    mmap(NULL, MMAP_BUF_SIZE, PROT_READ, MAP_SHARED, dev_fd, 0);
			for (;;) {
				assert((ret = ioctl(dev_fd, SLAVE_IOCTL_MMAP)) >= 0 &&
				       "mmap receiving failed");
				if (!ret) break;
				assert(ftruncate(file_fd, file_size + ret) == 0);
				char *file_mem =
				    mmap(NULL, ret, PROT_WRITE, MAP_SHARED, file_fd, file_size);
				memcpy(file_mem, dev_mem, ret);
				munmap(file_mem, ret);
				file_size += ret;
			}
		} else {
			fprintf(stderr, "Operation not supported\n");
			return EXIT_FAILURE;
		}

		// end receiving data, close the connection
		if (ioctl(dev_fd, SLAVE_IOCTL_EXIT) == -1) {
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
