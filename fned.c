/*
 * fned: Rename files using $EDITOR
 */
#include "base.h"
#include "vec.h"
#include <dirent.h>
#include <signal.h>
#include <sys/stat.h>

typedef char **strvec;

char *editor;
char *tmp;

void cleanup(void) {
	if (tmp != NULL) {
		unlink(tmp);
	}
}

void sigsetup(int sig, void (*handler)(int)) {
	struct sigaction sa = {.sa_handler = handler};
	sigaction(sig, &sa, NULL);
}

void sighandle(int sig) {
	cleanup();
	sigsetup(sig, SIG_DFL);
	kill(getpid(), sig);
	_exit(1);
}

int isdir(const char *path) {
	struct stat st;
	return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

int mkdirs(char *path) {
	char c, *s = path;
	while (*s != '\0') {
		if (*s == '/') {
			s++;
			continue;
		}
		for (; *s != '\0' && *s != '/'; s++);
		c = *s;
		*s = '\0';
		if (!isdir(path) && mkdir(path, 0755) == -1) {
			return -1;
		}
		*s = c;
	}
	return 0;
}

int rmdirs(char *path) {
	char c, *s = path + strlen(path);
	while (s != path) {
		if (s[-1] == '/') {
			s--;
			continue;
		}
		c = *s;
		*s = '\0';
		if (rmdir(path) == -1) {
			return -1;
		}
		for (*s-- = c; s != path && *s != '/'; s--);
	}
	return 0;
}

char *dirop(char *path, int (*op)(char *)) {
	char *sep = strrchr(path, '/');
	if (sep == NULL) {
		return path;
	}
	*sep = '\0';
	int res = op(path);
	*sep = '/';
	if (res == -1) {
		return NULL;
	}
	return sep + 1;
}

int rm(const char *path) {
	if (isdir(path)) {
		return rmdir(path);
	} else {
		return unlink(path);
	}
}

int fncmp(const void *a, const void *b) {
	return strcoll(*(const char **)a, *(const char **)b);
}

strvec ls(const char *path) {
	strvec entries = vec_new();
	DIR *d = opendir(path);
	if (d == NULL) {
		error(EXIT_FAILURE, errno, "%s", path);
	}
	for (;;) {
		errno = 0;
		struct dirent *e = readdir(d);
		if (e == NULL) {
			if (errno != 0) {
				error(EXIT_FAILURE, errno, "%s", path);
			}
			break;
		}
		if (e->d_name[0] != '.') {
			vec_push(&entries, estrdup(e->d_name));
		}
	}
	closedir(d);
	qsort(entries, vec_len(&entries), sizeof(entries[0]), fncmp);
	return entries;
}

strvec readlines(FILE *f) {
	char *buf = NULL;
	size_t size = 0;
	strvec lines = vec_new();
	for (;;) {
		ssize_t len = getline(&buf, &size, f);
		if (len == -1) {
			if (ferror(f)) {
				error(EXIT_FAILURE, errno, "getline");
			}
			break;
		}
		if (len > 0 && buf[len - 1] == '\n') {
			buf[--len] = '\0';
		}
		vec_push(&lines, estrdup(buf));
	}
	return lines;
}

strvec readfile(const char *path) {
	FILE *f = fopen(path, "r");
	if (f == NULL) {
		error(EXIT_FAILURE, errno, "%s", path);
	}
	strvec lines = readlines(f);
	fclose(f);
	return lines;
}

void writefile(const char *path, const strvec lines) {
	FILE *f = fopen(path, "w");
	if (f == NULL) {
		error(EXIT_FAILURE, errno, "%s", path);
	}
	for (size_t i = 0, n = vec_len(&lines); i < n; i++) {
		fprintf(f, "%s\n", lines[i]);
	}
	fclose(f);
}

void editfile(char *path) {
	char *argv[] = {editor, path, NULL};
	if (call(argv) != 0) {
		error(EXIT_FAILURE, 0, "aborting");
	}
}

char *mktmp(void) {
	char tp[] = "/tmp/fned.XXXXXX";
	int fd = mkstemp(tp);
	if (fd == -1) {
		error(EXIT_FAILURE, errno, "mkstemp: %s", tp);
	}
	close(fd);
	return estrdup(tp);
}

void init(void) {
	editor = getenv("EDITOR");
	if (editor == NULL || editor[0] == '\0') {
		error(EXIT_FAILURE, 0, "EDITOR not set");
	}
	atexit(cleanup);
	sigsetup(SIGHUP, sighandle);
	sigsetup(SIGINT, sighandle);
	sigsetup(SIGQUIT, sighandle);
	sigsetup(SIGABRT, sighandle);
	sigsetup(SIGTERM, sighandle);
	sigsetup(SIGUSR1, sighandle);
	sigsetup(SIGUSR2, sighandle);
	tmp = mktmp();
}

int main(int argc, char *argv[]) {
	argv0 = argv[0];
	init();
	strvec sv;
	if (argc < 2) {
		sv = ls(".");
	} else {
		sv = vec_new();
		for (int i = 1; i < argc; i++) {
			vec_push(&sv, argv[i]);
		}
	}
	writefile(tmp, sv);
	editfile(tmp);
	strvec dv = readfile(tmp);
	size_t dn = vec_len(&dv);
	size_t sn = vec_len(&sv);
	int err = 0;
	for (size_t i = 0; i < dn || i < sn; i++) {
		char *dst = i < dn ? dv[i] : NULL;
		char *src = i < sn ? sv[i] : NULL;
		if (dst != NULL && src != NULL && strcmp(dst, src) == 0) {
			continue;
		}
		if (dst == NULL || dst[0] == '\0') {
			if (rm(src) == -1) {
				err = 1;
				error(0, errno, "%s", src);
			}
			continue;
		}
		if (access(dst, F_OK) == 0) {
			err = 1;
			error(0, EEXIST, "%s", dst);
			continue;
		}
		char *base = dirop(dst, &mkdirs);
		if (base == NULL) {
			err = 1;
			error(0, errno, "%s", dst);
			continue;
		}
		if (src != NULL) {
			if (rename(src, dst) == -1) {
				err = 1;
				error(0, errno, "rename: %s, %s", src, dst);
			}
		} else if (base[0] != '\0') {
			FILE *f = fopen(dst, "a");
			if (f == NULL) {
				err = 1;
				error(0, errno, "%s", dst);
			} else {
				fclose(f);
			}
		}
	}
	for (size_t i = 0; i < sn; i++) {
		dirop(sv[i], &rmdirs);
	}
	return err ? EXIT_FAILURE : EXIT_SUCCESS;
}
