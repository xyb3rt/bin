/*
 * fned: Rename files using $EDITOR
 */
#include "base.h"
#include <sys/stat.h>

struct buf {
	char *d;
	size_t size;
};

int tmpfd = -1;
char tmppath[] = "/tmp/fned.XXXXXX";

void cleanup() {
	if (tmpfd >= 0) {
		unlink(tmppath);
	}
}

int mkdirs(char *path) {
	char c, *s = path;
	struct stat st;
	while (*s != '\0') {
		if (*s == '/') {
			s++;
			continue;
		}
		for (; *s != '\0' && *s != '/'; s++);
		c = *s;
		*s = '\0';
		if (mkdir(path, 0755) == -1) {
			if (errno != EEXIST || stat(path, &st) == -1 || !S_ISDIR(st.st_mode)) {
				return -1;
			}
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

int rm(const char *path) {
	struct stat st;
	if (stat(path, &st) == 0 && S_ISDIR(st.st_mode)) {
		return rmdir(path);
	} else {
		return unlink(path);
	}
}

ssize_t readline(struct buf *buf, FILE *f) {
	ssize_t n = getline(&buf->d, &buf->size, f);
	if (n > 0 && buf->d[n - 1] == '\n') {
		buf->d[--n] = '\0';
	}
	return n;
}

int main(int argc, char *argv[]) {
	int i;
	struct buf buf;
	FILE *f, *tmpf;
	char *editor[] = { NULL, tmppath, NULL };
	char *dst = NULL, *s, *src;
	argv0 = argv[0];
	atexit(cleanup);
	if ((editor[0] = getenv("EDITOR")) == NULL || *editor[0] == '\0') {
		error(EXIT_FAILURE, 0, "EDITOR not set");
	}
	if ((tmpfd = mkstemp(tmppath)) < 0 || (tmpf = fdopen(tmpfd, "r+")) == NULL) {
		error(EXIT_FAILURE, errno, "%s", tmppath);
	}
	for (i = 1; i < argc; i++) {
		fprintf(tmpf, "%s\n", argv[i]);
	}
	fclose(tmpf);
	if (call(editor) != 0) {
		return EXIT_FAILURE;
	}
	if ((tmpf = fopen(tmppath, "r")) == NULL) {
		error(EXIT_FAILURE, errno, "%s", tmppath);
	}
	for (i = 1; i < argc && readline(&buf, tmpf) != -1; i++) {
		dst = buf.d;
		src = argv[i];
		if (strcmp(src, dst) == 0) {
			continue;
		}
		if (access(dst, F_OK) == 0) {
			error(0, EEXIST, "%s -> %s", src, dst);
			continue;
		}
		if ((s = strrchr(dst, '/')) != NULL) {
			*s = '\0';
			if (mkdirs(dst) == -1) {
				error(0, errno, "%s -> %s/%s: %s", src, dst, s + 1, dst);
				continue;
			}
			*s = '/';
		}
		if (dst[0] != '\0') {
			if (rename(src, dst) == -1) {
				error(0, errno, "%s -> %s", src, dst);
			}
		} else {
			if (rm(src) == -1) {
				error(0, errno, "%s", src);
			}
		}
	}
	while (readline(&buf, tmpf) != -1) {
		dst = buf.d;
		if ((s = strrchr(dst, '/')) != NULL) {
			*s = '\0';
			if (mkdirs(dst) == -1) {
				error(0, errno, "%s/%s: %s", dst, s + 1, dst);
				continue;
			}
			if (s[1] == '\0') {
				continue;
			}
			*s = '/';
		}
		if ((f = fopen(dst, "a")) == NULL) {
			error(0, errno, "%s", dst);
			continue;
		}
		fclose(f);
	}
	for (i = 1; i < argc; i++) {
		src = argv[i];
		if ((s = strrchr(src, '/')) != NULL) {
			*s = '\0';
			rmdirs(src);
		}
	}
	fclose(tmpf);
	return EXIT_SUCCESS;
}
