/*
 * timely: Run command every time any file in given directories is written
 */
#include "base.h"
#include <limits.h>
#include <poll.h>
#include <sys/inotify.h>

struct pollfd pollfd;

void usage() {
	fprintf(stderr, "usage: %s COMMAND\n", strbsnm(argv0));
}

void setup() {
	int inotifyfd = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);
	if (inotifyfd == -1) {
		error(EXIT_FAILURE, errno, "inotify_init");
	}
	pollfd.fd = inotifyfd;
	pollfd.events = POLLIN;
}

void watch(const char *path) {
	int watchfd = inotify_add_watch(pollfd.fd, path, IN_CLOSE_WRITE);
	if (watchfd == -1) {
		error(EXIT_FAILURE, errno, "inotify_add_watch: %s", path);
	}
}

void block() {
	while (poll(&pollfd, 1, -1) == -1) {
		if (errno != EINTR) {
			error(EXIT_FAILURE, errno, "poll");
		}
	}
}

void drain() {
	static char buf[sizeof(struct inotify_event) + NAME_MAX + 1];
	for (;;) {
		if (read(pollfd.fd, buf, sizeof(buf)) == -1) {
			if (errno == EAGAIN) {
				break;
			} else if (errno == EINTR) {
				continue;
			}
			error(EXIT_FAILURE, errno, "read");
		}
		/*
		 * Wait 100ms for more notifications because there is
		 * typically more than one and the read call above hits
		 * EAGAIN between them.
		 */
		if (poll(&pollfd, 1, 100) == 0) {
			break;
		}
	}
}

void clearbuf() {
	char *argv[] = {"avim", "-c", NULL, NULL};
	argv[2] = getenv("ACMEVIMOUTBUF");
	if (argv[2] != NULL && argv[2][0] != '\0') {
		call(argv, NULL);
	}
}

int main(int argc, char *argv[]) {
	argv0 = argv[0];
	if (argc < 2) {
		usage();
		return EXIT_FAILURE;
	}
	setup();
	if (argc == 2) {
		watch(".");
	} else for (int i = 2; i < argc; i++) {
		watch(argv[i]);
	}
	system(argv[1]);
	drain();
	for (;;) {
		block();
		drain();
		clearbuf();
		system(argv[1]);
		drain();
	}
	return 0;
}
