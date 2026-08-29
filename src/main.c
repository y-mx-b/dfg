#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/stat.h>
#include <stdarg.h>

#include "path.h"
#include "cli.h"

struct arg {
	char *profile;
	char *link;
};

/// Parse an argument of the form `[profile]:[link]`.
///
/// If either `profile` or `link` are not specified, then the corresponding
/// return value will be set to NULL.
struct arg arg_parse(char *arg) {
	char c;
	int i = 0;
	// search for colon separator
	for (c = arg[i]; c != ':' && c != '\0'; c = arg[++i]) {}
	// if colon separator exists, overwrite colon with '\0' to split
	// the string such at `arg` is the profile and `arg[i]` is the link
	if (c == ':') { arg[i++] = '\0'; }

	return (struct arg) {
		.profile = arg[0] ? arg : NULL,
		.link = arg[i] ? &arg[i] : NULL
	};
}

/// Print to `stderr`.
void eprintf(const char *restrict format, ...) {
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
}

int main(int argc, char **argv) {
	struct option option = {
		.help = false,
		.dry = false,
		.force = false,
		.unlink = false,
		.store = { 0 },
		.root = { 0 },
	};

	const char *err = cli_parse(argc, argv, &option);
	if (err) {
		eprintf("%s\n", err);
		exit(EXIT_FAILURE);
	}

	if (optind >= argc) {
		eprintf("error: expected arguments\n");
		eprintf("%s\n", USAGE);
		exit(EXIT_FAILURE);
	}

	if (option.root[0] == 0) {
		strlcpy(option.root, home_dir(), PATH_MAX);
	}
	if (option.dry) { printf("root: %s\n", option.root); }

	if (option.store[0] == 0) {
		char *dfg_store = getenv("DFG_STORE");
		if (dfg_store) {
			strlcpy(option.store, dfg_store, PATH_MAX);
		} else {
			path_join(PATH_MAX, option.store, home_dir(), ".dfg");
		}
	}
	if (option.dry) { printf("store: %s\n", option.store); }

	// parse and operate over non-option arguments
	for (char *arg_str; (arg_str = argv[optind]) != NULL; optind += 1) {
		char profile[PATH_MAX] = { 0 };
		char link[PATH_MAX] = { 0 };

		struct arg arg = arg_parse(arg_str);

		// build profile path
		// if profile is a relative path, use store as the base bath
		// if profile is an absolute path, use the absolute path
		// if profile starts with `~/`, replace with home directory
		size_t n = 0;
		int last_sep = 0;
		char *profile_name;
		if (arg.profile) {
			for (int j = 0, c = arg.profile[j]; c != '\0'; c = arg.profile[++j]) {
				if (c == '/') { last_sep = j; }
			}
			if (last_sep > 0) {
				profile_name = &arg.profile[last_sep + 1];
			} else {
				profile_name = arg.profile;
			}
			switch (arg.profile[0]) {
			case '/':
				n = strlcpy(profile, arg.profile, PATH_MAX);
				break;
			case '~':
				if (arg.profile[1] == '/') {
					n = snprintf(profile, PATH_MAX, "%s/%s", home_dir(), &arg.profile[2]);
					break;
				}
			default:
				n = snprintf(profile, PATH_MAX, "%s/%s", option.store, arg.profile);
			}
		}
		if (n >= PATH_MAX) {
			eprintf("error: profile path too long, skipping (truncated profile path: \"%s\")\n", profile);
			continue;
		}

		// build link path
		// if absolute path is specified, use absolute path
		// if link path starts with `~/`, replace with home directory
		// if relative path is specified, append to root path
		// if no link path is specified, append profile name to root path
		n = 0;
		if (arg.link) {
			switch (arg.link[0]) {
			case '/':
				n = strlcpy(link, arg.link, PATH_MAX);
				break;
			case '~':
				if (arg.link[1] == '/') {
					n = snprintf(link, PATH_MAX, "%s/%s", home_dir(), &arg.link[2]);
					break;
				}
			default:
				n = snprintf(link, PATH_MAX, "%s/%s", option.root, arg.link);
				break;
			}
		} else {
			n = snprintf(link, PATH_MAX, "%s/%s", option.root, profile_name);
		}
		if (n >= PATH_MAX) {
			eprintf("error: link path too long, skipping (truncated link path: \"%s\")\n", link);
			continue;
		}

		int err = 0;
		if (option.unlink) {
			if (option.dry) {
				printf("unlink: \"%s\" -> \"%s\"\n", profile, link);
				continue;
			}

			// TODO: extract into separate function and check errno
			struct stat link_stat;
			if ((err = lstat(link, &link_stat)) == 0) {
				if (S_ISLNK(link_stat.st_mode)) {
					err = remove(link);
				} else {
					eprintf("error: expected symlink at `%s`\n", link);
				}
			}
		} else {
			if (arg.profile) {
				eprintf("error: expected profile\n");
				exit(EXIT_FAILURE);
			}
			if (option.dry) {
				printf("link: \"%s\" -> \"%s\"\n", profile, link);
				continue;
			}
			const char *err_msg = dfg_link(profile, link);
			if (err_msg) {
				eprintf("failed to link \"%s\" -> \"%s\"", profile, link);
				eprintf("%s\n", err_msg);
			}
		}
	}

	return EXIT_SUCCESS;
}
