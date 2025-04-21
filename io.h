#include "base.h"
#include "vec.h"

typedef char **strvec;

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
		vec_push(&lines, xstrdup(buf));
	}
	free(buf);
	return lines;
}

