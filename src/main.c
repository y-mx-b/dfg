#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <stdarg.h>

#include "path.h"

/// Print to `stderr`.
void eprintf(const char *restrict format, ...) {
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
}

#define USAGE "usage: dfg [-hdfu] [-s <store-path>] [-r <root-path>] <[profile]:[link]>..."
#define HELP \
	"A dotfile configuration utility.\n" \
	"\n" \
	USAGE"\n" \
	"\n" \
	"options:\n" \
	"    -h          Display usage information.\n" \
	"    -d          Perform a dry run and print all actions insteaed of executing.\n" \
	"    -f          Overwrite any existing files encountered.\n" \
	"    -u          Unlink the given profiles instead of linking them.\n" \
	"    -s          Path to the profile store. [default: $HOME/.dfg]\n" \
	"    -r          Path to the root directory for links. [default: $HOME]\n"

int main(int argc, char **argv) {
	struct option {
		bool help; // -h
		bool dry; // -d
		bool force; // -f
		bool unlink; // -u
		char store[PATH_MAX]; // -s <store-path>
		char root[PATH_MAX]; // -r <root-path>
	} option = { 0 };

	// -- PARSE COMMAND-LINE OPTIONS
	{
		opterr = 0;
		int opt;
		size_t len = 0;
		while ((opt = getopt(argc, argv, "hdfus:r:")) != -1) {
			switch (opt) {
			case 'h':
				option.help = true;
				break;
			case 'd':
				option.dry = true;
				break;
			case 'f':
				option.force = true;
				break;
			case 'u':
				option.unlink = true;
				break;
			case 's':
				len = strlcpy(option.store, optarg, PATH_MAX);
				if (len >= PATH_MAX) {
					eprintf("error: store path too long\n");
					exit(EXIT_FAILURE);
				}
				len = 0;
				break;
			case 'r':
				len = strlcpy(option.root, optarg, PATH_MAX);
				if (len >= PATH_MAX) {
					eprintf("error: root path too long\n");
					exit(EXIT_FAILURE);
				}
				len = 0;
				break;
			case '?':
				if (optopt == 's' && optarg == NULL) {
					eprintf("error: expected argument for option `-s`\n");
				} else if (optopt == 'r' && optarg == NULL) {
					eprintf("error: expected argument for option `-r`\n");
				} else {
					eprintf("error: unknown option, `-%c`\n", optopt);
				}
				exit(EXIT_FAILURE);
			default:
				printf("getopt default case?\n");
				break;
			}
		}

		if (optind >= argc) {
			eprintf("error: expected arguments\n");
			eprintf("%s\n", USAGE);
			exit(EXIT_FAILURE);
		}
	}

	// -- INIT STORE PATH --
	if (option.store[0] == 0) {
		char *dfg_store = getenv("DFG_STORE");
		if (dfg_store) {
			strlcpy(option.store, dfg_store, PATH_MAX);
		} else {
			path_join(PATH_MAX, option.store, home_dir(), ".dfg");
		}
	}
	if (option.dry) { printf("store: %s\n", option.store); }

	// -- INIT ROOT PATH --
	if (option.root[0] == 0) {
		strlcpy(option.root, home_dir(), PATH_MAX);
	}
	if (option.dry) { printf("root: %s\n", option.root); }

	// -- MAIN LOGIC
	for (char *arg; (arg = argv[optind]) != NULL; optind += 1) {
		// -- PARSE ARG --
		// if of the form `:link`, profile will be null
		// if of the form `profile:`, link will be null
		// if of the form `:`, both will be null
		struct {
			char *profile;
			char *link;
		} split = { 0 };
		{
			char c;
			int i = 0;
			// search for colon separator
			for (c = arg[i]; c != ':' && c != '\0'; c = arg[++i]) {}
			// if colon separator exists, overwrite colon with '\0' to split
			// the string such at `arg` is the profile and `arg[i]` is the link
			if (c == ':') { arg[i++] = '\0'; }

			split.profile = arg[0] ? arg : NULL;
			split.link = arg[i] ? &arg[i] : NULL;
		}

		// -- BUILD PROFILE PATH --
		// if profile is a relative path, use store as the base bath
		// if profile is an absolute path, use the absolute path
		// if profile starts with `~/`, replace with home directory
		char profile[PATH_MAX] = { 0 };
		char *profile_name;
		{
			size_t n = 0;
			int last_sep = 0;
			if (split.profile) {
				for (int j = 0, c = split.profile[j]; c != '\0'; c = split.profile[++j]) {
					if (c == '/') { last_sep = j; }
				}
				if (last_sep > 0) {
					profile_name = &split.profile[last_sep + 1];
				} else {
					profile_name = split.profile;
				}
				switch (split.profile[0]) {
				case '/':
					n = strlcpy(profile, split.profile, PATH_MAX);
					break;
				case '~':
					if (split.profile[1] != '/') { break; }
					n = snprintf(profile, PATH_MAX, "%s/%s", home_dir(), &split.profile[2]);
					break;
				default:
					n = snprintf(profile, PATH_MAX, "%s/%s", option.store, split.profile);
					break;
				}
			}
			if (n >= PATH_MAX) {
				eprintf("error: profile path too long, skipping (truncated profile path: \"%s\")\n", profile);
				continue;
			}
		}

		// -- BUILD LINK PATH --
		// if absolute path is specified, use absolute path
		// if link path starts with `~/`, replace with home directory
		// if relative path is specified, append to root path
		// if no link path is specified, append profile name to root path
		char link[PATH_MAX] = { 0 };
		{
			size_t n = 0;
			if (split.link == NULL && split.profile == NULL) {
				eprintf("error: failed to construct link path\n");
				continue;
			}
			if (split.link == NULL) {
				n = snprintf(link, PATH_MAX, "%s/%s", option.root, profile_name);
			} else {
				switch (split.link[0]) {
				case '/':
					n = strlcpy(link, split.link, PATH_MAX);
					break;
				case '~':
					if (split.link[1] != '/') { break; }
					n = snprintf(link, PATH_MAX, "%s/%s", home_dir(), &split.link[2]);
					break;
				default:
					n = snprintf(link, PATH_MAX, "%s/%s", option.root, split.link);
					break;
				}
			}
			if (n >= PATH_MAX) {
				eprintf("error: link path too long, skipping (truncated link path: \"%s\")\n", link);
				continue;
			}
		}

		const char *err_msg;
		if (option.unlink) {
			// -- UNLINK --
			if (option.dry) {
				printf("unlink: \"%s\"\n", link);
				continue;
			}

			err_msg = dfg_unlink(link);
			if (err_msg) {
				eprintf("failed to unlink \"%s\"", link);
				eprintf("%s\n", err_msg);
			}
		} else {
			// -- LINK --
			if (split.profile == NULL) {
				eprintf("error: expected profile\n");
				exit(EXIT_FAILURE);
			}
			if (option.dry) {
				printf("link: \"%s\" -> \"%s\"\n", profile, link);
				continue;
			}
			err_msg = dfg_link(profile, link, option.force);
			if (err_msg) {
				eprintf("failed to link \"%s\" -> \"%s\"\n", profile, link);
				eprintf("%s\n", err_msg);
			}
		}
	}

	return EXIT_SUCCESS;
}
