/*
 * fned: Rename files using $EDITOR
 */
#include "base.h"
#include <sys/stat.h>

int tmpfd = -1;
char tmppath[] = "/tmp/fned.XXXXXX";

void usage() {
	fprintf(stderr, "usage: %s PATH...\n", strbsnm(argv0));
}

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

int main(int argc, char *argv[]) {
	int i;
	FILE *tmpfs;
	char *editor[] = { NULL, tmppath, NULL };
	char *dst = NULL, *s, *src;
	size_t n, size = 0;
	argv0 = argv[0];
	atexit(cleanup);
	if (argc == 1) {
		usage();
		return EXIT_FAILURE;
	}
	if ((editor[0] = getenv("EDITOR")) == NULL || *editor[0] == '\0') {
		error(EXIT_FAILURE, 0, "EDITOR not set");
	}
	if ((tmpfd = mkstemp(tmppath)) < 0 || (tmpfs = fdopen(tmpfd, "r+")) == NULL) {
		error(EXIT_FAILURE, errno, "%s", tmppath);
	}
	for (i = 1; i < argc; i++) {
		fprintf(tmpfs, "%s\n", argv[i]);
	}
	fclose(tmpfs);
	if (call(editor) != 0) {
		return EXIT_FAILURE;
	}
	if ((tmpfs = fopen(tmppath, "r")) == NULL) {
		error(EXIT_FAILURE, errno, "%s", tmppath);
	}
	for (i = 1; i < argc && (n = getline(&dst, &size, tmpfs)) != -1; i++) {
		src = argv[i];
		if (dst[n-1] == '\n') {
			dst[--n] = '\0';
		}
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
	for (i = 1; i < argc; i++) {
		src = argv[i];
		if ((s = strrchr(src, '/')) != NULL) {
			*s = '\0';
			rmdirs(src);
		}
	}
	fclose(tmpfs);
	return EXIT_SUCCESS;
}
