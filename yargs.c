/*
 * yargs: `xargs -I {} -P 10 -r` but keep output order, as if run sequentially
 */
#include "io.h"
#include <fcntl.h>

struct proc {
	char *buf;
	int fd;
	pid_t pid;
};

int chld;
size_t maxprocs = 10;
size_t numprocs;
struct proc *procs;

int wait_() {
	int ret = 0;
	while (numprocs > 0) {
		int stat;
		pid_t pid = waitpid(-1, &stat, WNOHANG);
		if (pid == 0) {
			break;
		}
		if (pid == -1 && errno != EINTR) {
			error(EXIT_FAILURE, errno, "wait");
		}
		if (WIFSTOPPED(stat) || WIFCONTINUED(stat)) {
			continue;
		}
		for (size_t i = 0; i < vec_len(&procs); i++) {
			if (procs[i].pid == pid) {
				procs[i].pid = 0;
				numprocs--;
				break;
			}
		}
		ret |= (WIFEXITED(stat) && WEXITSTATUS(stat) != 0) ||
			WIFSIGNALED(stat);
	}
	return ret;
}

void sigchld(int sig) {
	(void)sig;
	chld = 1;
}

strvec cmdv(char *argv[], char *arg) {
	strvec cmd = vec_new();
	for (size_t i = 0;; i++) {
		if (argv[i] != NULL && strcmp(argv[i], "{}") == 0) {
			vec_push(&cmd, arg);
		} else {
			vec_push(&cmd, argv[i]);
		}
		if (argv[i] == NULL) {
			break;
		}
	}
	return cmd;
}

void spawn(char *argv[]) {
	int fd[2];
	if (pipe(fd) == -1) {
		error(EXIT_FAILURE, errno, "pipe");
	}
	pid_t pid = fork();
	if (pid == -1) {
		error(EXIT_FAILURE, errno, "fork");
	} else if (pid == 0) {
		dup2(fd[1], 1);
		dup2(fd[1], 2);
		close(fd[0]);
		close(fd[1]);
		execvp(argv[0], argv);
		error(EXIT_FAILURE, errno, "exec");
	}
	fcntl(fd[0], F_SETFL, O_NONBLOCK);
	close(fd[1]);
	struct proc *proc = vec_dig(&procs, -1, 1);
	proc->fd = fd[0];
	proc->buf = vec_new();
	proc->pid = pid;
	numprocs++;
}

int preselect(fd_set *fds) {
	int nfd = 0;
	for (size_t i = 0; i < vec_len(&procs); i++) {
		if (procs[i].fd != -1) {
			FD_SET(procs[i].fd, fds);
			if (nfd <= procs[i].fd) {
				nfd = procs[i].fd + 1;
			}
		}
	}
	return nfd;
}

void read_(struct proc *p) {
	static char d[4096];
	for (;;) {
		ssize_t n = read(p->fd, d, sizeof d);
		if (n == 0) {
			close(p->fd);
			p->fd = -1;
			return;
		} else if (n > 0) {
			memcpy(vec_dig(&p->buf, vec_len(&p->buf), n), d, n);
		} else if (errno == EAGAIN) {
			return;
		} else if (errno != EINTR) {
			error(EXIT_FAILURE, errno, "read");
		}
	}
}

void write_(struct proc *p) {
	size_t n = fwrite(p->buf, 1, vec_len(&p->buf), stdout);
	if (n > 0) {
		vec_erase(&p->buf, 0, n);
		fflush(stdout);
	}
}

void handle(fd_set *fds) {
	for (size_t i = 0; i < vec_len(&procs);) {
		struct proc *p = &procs[i];
		if (p->fd != -1 && FD_ISSET(p->fd, fds)) {
			read_(p);
		}
		if (i == 0) {
			write_(p);
		}
		if (p->fd == -1 && vec_len(&p->buf) == 0 && p->pid == 0) {
			vec_erase(&procs, i, 1);
		} else {
			i++;
		}
	}
}

int main(int argc, char *argv[]) {
	argv0 = argv[0];
	signal(SIGCHLD, sigchld);
	strvec args = splitlines(xreadall(stdin));
	size_t spawned = 0;
	int status = 0;
	procs = vec_new();
	while (spawned < vec_len(&args) || vec_len(&procs) > 0) {
		while (numprocs < maxprocs && spawned < vec_len(&args)) {
			strvec cmd = cmdv(&argv[1], args[spawned]);
			spawn(cmd);
			spawned++;
			vec_free(&cmd);
		}
		fd_set fds;
		FD_ZERO(&fds);
		if (!chld) {
			int nfd = preselect(&fds);
			if (select(nfd, &fds, NULL, NULL, NULL) == -1) {
				if (errno != EINTR) {
					error(EXIT_FAILURE, errno, "select");
				}
				FD_ZERO(&fds);
			}
		}
		if (chld) {
			chld = 0;
			if (wait_()) {
				status = 1;
			}
		}
		handle(&fds);
	}
	return status;
}
