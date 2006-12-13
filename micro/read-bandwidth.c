#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <err.h>
#include "../benchmarks.h"

#ifndef O_DIRECT
#define O_DIRECT	00040000	/* direct disk access hint */
#endif

#define TESTFILE "/tmp/virtbench-read-bandwidth.test"

#define READ_SIZE (16 kB)
static const char fmtstr[] = "Time to read from disk " __stringify(READ_SIZE) ": %u nsec";
#define kB * 1024

static void do_read_bandwidth(int fd, u32 runs,
			      struct sockaddr *from, socklen_t *fromlen,
			      struct benchmark *bench, const void *opts)
{
	int testfd;
	struct stat st;
	char *p, *pa;

	p = malloc(READ_SIZE*runs+getpagesize()-1);
	/* O_DIRECT wants an aligned pointer. */
	pa = (void *)(((unsigned long)p+getpagesize()-1) & ~(getpagesize()-1));

	testfd = open(TESTFILE, O_RDONLY|O_DIRECT, 0);
	if (testfd < 0) {
		testfd = open(TESTFILE, O_CREAT|O_EXCL|O_RDWR, 0600);
		if (testfd < 0)
			err(1, "creating " TESTFILE);

		if (write(testfd, p, READ_SIZE*runs) != READ_SIZE*runs)
				err(1, "writing to " TESTFILE);
		fsync(testfd);
		close(testfd);

		testfd = open(TESTFILE, O_RDONLY|O_DIRECT, 0);
		if (testfd < 0)
			err(1, "opening " TESTFILE " after creation");
	}

	fstat(testfd, &st);

	send_ack(fd, from, fromlen);
	if (wait_for_start(fd)) {
		u32 i;

		for (i = 0; i < runs; i++) {
			lseek(testfd, 0, SEEK_SET);
			if (read(testfd, pa, READ_SIZE*runs) != READ_SIZE*runs)
				err(1, "reading from " TESTFILE);
		}
		send_ack(fd, from, fromlen);
	}
	close(testfd);
	free(p);
}

struct benchmark read_bandiwdth_benchmark _benchmark_
= { "read-bandwidth", fmtstr, do_single_bench, do_read_bandwidth };
