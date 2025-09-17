#include <errno.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

static const char *argv0;

static int isdir(const char *path) {
	struct stat st;
	return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

static char *strbsnm(const char *s) {
	char *t = strrchr((char *)s, '/');
	return t != NULL && t[1] != '\0' ? &t[1] : (char *)s;
}

static void error(int, int, const char *, ...)
	__attribute__ ((format (printf, 3, 4)));

static void error(int status, int errnum, const char *fmt, ...) {
	va_list ap;
	if (argv0 != NULL) {
		fputs(strbsnm(argv0), stderr);
		fputs(": ", stderr);
	}
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	if (errnum != 0) {
		fputs(": ", stderr);
		fputs(strerror(errnum), stderr);
	}
	fputc('\n', stderr);
	if (status != 0) {
		exit(status);
	}
}

static char *xstrdup(const char *s) {
	char *p = strdup(s);
	if (p == NULL) {
		error(EXIT_FAILURE, errno, "strdup");
	}
	return p;
}

static int call(char *const argv[], int fds[3]) {
        int status = 0;
        pid_t pid = fork();
        if (pid == -1) {
                error(EXIT_FAILURE, errno, "exec: %s", argv[0]);
        }
        if (pid == 0) {
                if (fds != NULL) {
                        dup2(fds[0], 0);
                        dup2(fds[1], 1);
                        dup2(fds[2], 2);
                }
                execvp(argv[0], argv);
                error(EXIT_FAILURE, errno, "exec: %s", argv[0]);
        }
        while (waitpid(pid, &status, 0) == -1) {
                if (errno != EINTR) {
                        error(EXIT_FAILURE, errno, "wait: %s", argv[0]);
                }
        }
        return status;
}
