/*
 * timely: Run command every time any file in current directory is written
 */
#include "base.h"
#include <arpa/inet.h>
#include <limits.h>
#include <poll.h>
#include <sys/inotify.h>
#include <sys/socket.h>
#include <sys/types.h>

struct sockaddr_in avaddr;
const char *avbuf, *avpid;
struct pollfd pollfd;

void usage() {
	fprintf(stderr, "usage: %s COMMAND\n", strbsnm(argv0));
}

void setup() {
	int inotifyfd = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);
	if (inotifyfd == -1) {
		error(EXIT_FAILURE, errno, "inotify_init");
	}
	int watchfd = inotify_add_watch(inotifyfd, ".", IN_CLOSE_WRITE);
	if (watchfd == -1) {
		error(EXIT_FAILURE, errno, "inotify_add_watch");
	}
	pollfd.fd = inotifyfd;
	pollfd.events = POLLIN;
	avbuf = getenv("ACMEVIMBUF");
	avpid = getenv("ACMEVIMPID");
	char *avport = getenv("ACMEVIMPORT");
	if (avbuf == NULL || avpid == NULL || avport == NULL) {
		return;
	}
	char *end;
	int port = strtol(avport, &end, 0);
	if (*end != '\0') {
		return;
	}
	avaddr.sin_family = AF_INET;
	avaddr.sin_port = htons(port);
	avaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
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
		// Wait 100ms for more notifications because there is
		// typically more than one and the read loop above hits
		// EAGAIN between them.
		if (poll(&pollfd, 1, 100) == 0) {
			break;
		}
	}
}

void clearbuf() {
	static char msg[1024];
	if (avaddr.sin_port == 0) {
		return;
	}
	int sockfd = socket(PF_INET, SOCK_STREAM, 0);
	if (sockfd == -1) {
		return;
	}
	if (connect(sockfd, (struct sockaddr *)&avaddr, sizeof(avaddr)) == -1) {
		goto end;
	}
	size_t len = snprintf(msg, sizeof(msg), "%s\x1f%u\x1f"
	                      "clear\x1f%s\x1e", avpid, getpid(), avbuf);
	send(sockfd, msg, len, 0);
end:
	close(sockfd);
}

int main(int argc, char *argv[]) {
	argv0 = argv[0];
	if (argc != 2) {
		usage();
		return EXIT_FAILURE;
	}
	setup();
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
