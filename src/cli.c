#include <stdbool.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#include "cli.h"

const char *cli_parse(int argc, char **argv, struct option *option) {
	static char err[PATH_MAX];

	memset(err, 0, PATH_MAX);
	opterr = 0;
	int opt;
	size_t len = 0;
	while ((opt = getopt(argc, argv, "hdfus:r:")) != -1) {
		switch (opt) {
		case 'h':
			option->help = true;
			break;
		case 'd':
			option->dry = true;
			break;
		case 'f':
			option->force = true;
			break;
		case 'u':
			option->unlink = true;
			break;
		case 's':
			len = strlcpy(option->store, optarg, PATH_MAX);
			if (len >= PATH_MAX) {
				return "error: store path too long\n";
			}
			len = 0;
			break;
		case 'r':
			len = strlcpy(option->root, optarg, PATH_MAX);
			if (len >= PATH_MAX) {
				return "error: root path too long\n";
			}
			len = 0;
			break;
		case '?':
			if (optopt == 's' && optarg == NULL) {
				return "error: expected argument for option `-s`\n";
			} else if (optopt == 'r' && optarg == NULL) {
				return "error: expected argument for option `-r`\n";
			} else {
				snprintf(
					err,
					PATH_MAX,
					"error: unknown option, `-%c`\n",
					optopt
				);
				return err;
			}
		default:
			break;
		}
	}

	return NULL;

}
