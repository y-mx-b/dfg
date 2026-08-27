#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/types.h>
#include <pwd.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/stat.h>
#include <stdarg.h>

#define USAGE "usage: dfg [-hdfu] [-s <store-path>] [-r <root-path>] <profile|profile:link>..."
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

/// Return a lazily initialized static path to the user's home directory.'
const char *home_dir() {
	static char home[PATH_MAX] = { 0 };

	if (home[0] == 0) {
		char *home_ptr = getenv("HOME");
		if (!home_ptr) {
			struct passwd *pw = getpwuid(getuid());
			home_ptr = pw->pw_dir;
		}
		strlcpy(home, home_ptr, PATH_MAX);
	}

	return home;
}

/// Print to `stderr`.
void eprintf(const char *restrict format, ...) {
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
}

/// Internal function to join path components into a single path.
void _path_join(size_t n, char *buf, ...) {
	va_list components;
	va_start(components, buf);

	char *component;
	memset(buf, 0, n);
	if ((component = va_arg(components, char *)) != NULL) {
		// TODO: handle `~`
		strlcpy(buf, component, n);
	}
	while ((component = va_arg(components, char *)) != NULL) {
		// TODO: remove extraneous path separators
		strlcat(buf, "/", n);
		strlcat(buf, component, n);
	}

	va_end(components);
}

#define path_join(n, buf, ...) _path_join(n, buf, __VA_ARGS__, NULL);

int main(int argc, char **argv) {
	char buf[1024];
	path_join(1024, buf, "hello", "world");
	printf("%s\n", buf);

	struct option {
		bool dry;
		bool force;
		bool unlink;
		char store[PATH_MAX];
		char root[PATH_MAX];
	} option = { 0 };

	opterr = 0;
	int opt;
	while ((opt = getopt(argc, argv, "hdfus:r:")) != -1) {
		switch (opt) {
		case 'h':
			printf(HELP);
			exit(EXIT_SUCCESS);
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
			strlcpy(option.store, optarg, PATH_MAX);
			break;
		case 'r':
			strlcpy(option.root, optarg, PATH_MAX);
			break;
		case '?':
			if (optopt == 's' && optarg == NULL) {
				eprintf("error: expected argument for option `-s`\n");
				eprintf("%s\n", USAGE);
			} else if (optopt == 'r' && optarg == NULL) {
				eprintf("error: expected argument for option `-r`\n");
				eprintf("%s\n", USAGE);
			} else {
				eprintf("error: unknown option `-%c`\n", optopt);
				eprintf("%s\n", USAGE);
			}
		default:
			eprintf("%s\n", USAGE);
			exit(EXIT_FAILURE);
			break;
		}
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
			strlcpy(option.store, home_dir(), PATH_MAX);
			strlcat(option.store, "/.dfg", PATH_MAX);
		}
	}
	if (option.dry) { printf("store: %s\n", option.store); }

	while (optind < argc) {
		char *arg = argv[optind++];
		char profile[PATH_MAX] = { 0 };
		char link[PATH_MAX] = { 0 };

		int i = 0;
		char c;
		// search for colon separator
		for (c = arg[i]; c != ':' && c != '\0'; c = arg[++i]);
		// if colon separator exists, overwrite colon with '\0' to split
		// the string such at `arg` is the profile and `arg[i]` is the link
		if (c == ':') { arg[i++] = '\0'; }

		// build profile path
		// if profile is a relative path, use store as the base bath
		// if profile is an absolute path, use the absolute path
		// if profile starts with `~/`, replace with home directory
		// TODO: use only the last path segment
		size_t n = 0;
		int last_sep = 0;
		char *profile_name;
		for (int j = 0, c = arg[j]; c != '\0'; c = arg[++j]) {
			if (c == '/') { last_sep = j; }
		}
		if (last_sep > 0) {
			profile_name = &arg[last_sep + 1];
		}
		switch (arg[0]) {
		case '/':
			n = strlcpy(profile, arg, PATH_MAX);
			break;
		case '~':
			if (arg[1] == '/') {
				n = snprintf(profile, PATH_MAX, "%s/%s", home_dir(), &arg[2]);
				break;
			}
		default:
			n = snprintf(profile, PATH_MAX, "%s/%s", option.store, profile_name);
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
		switch (arg[i]) {
		case '\0':
			n = snprintf(link, PATH_MAX, "%s/%s", option.root, profile_name);
			break;
		case '/':
			n = strlcpy(link, &arg[i], PATH_MAX);
			break;
		case '~':
			if (arg[i+1] == '/') {
				n = snprintf(link, PATH_MAX, "%s/%s", home_dir(), &arg[i+2]);
				break;
			}
		default:
			n = snprintf(link, PATH_MAX, "%s/%s", option.root, &arg[i]);
			break;
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

			struct stat link_stat;
			if ((err = lstat(link, &link_stat)) == 0) {
				if (S_ISLNK(link_stat.st_mode)) {
					err = remove(link);
				} else {
					eprintf("error: expected symlink at `%s`\n", link);
				}
			}
		} else {
			if (option.dry) {
				printf("link: \"%s\" -> \"%s\"\n", profile, link);
				continue;
			}

			if (access(profile, F_OK) != 0) {
				eprintf("error: profile `%s` does not exist\n", arg);
				continue;
			}

			err = symlink(profile, link);
			if (err == 0) { continue; }
			eprintf("failed to link \"%s\" -> \"%s\"", profile, link);
			switch(errno) {
			case EACCES:
				eprintf("error: permission denied\n");
				break;
			case EDQUOT:
				eprintf("error: disk quota exceed\n");
				break;
			case EEXIST:
				eprintf("error: link path already exists\n");
				break;
			case EFAULT:
				eprintf("error: profile or link path points outside accessible address space\n");
				break;
			case EIO:
				eprintf("error: I/O error\n");
				break;
			case ELOOP:
				eprintf("error: failed to resolve link path (too many symlinks)\n");
				break;
			case ENAMETOOLONG:
				eprintf("error: profile or link path too long\n");
				break;
			case ENOENT:
				eprintf("error: link path has a component that does not exist\n");
				break;
			case ENOSPC:
				eprintf("error: ran out of space\n");
				break;
			case ENOTDIR:
				eprintf("error: link path contains a component that is not a directory\n");
				break;
			case EROFS:
				eprintf("error: link path is on a read-only filesystem\n");
				break;
			case EILSEQ:
				eprintf("error: filename does not match encoding rules\n");
				break;
			default:
				eprintf("error: unknown error\n");
				break;
			}
			// TODO: implement -f flag for replacing existing file system entries
			// TODO: check if profile exists, error if not
		}

		// TODO: extract functionality into separate functions that return error code + context
		// TODO: implement error handling
		if (err == 0) { continue; }
		switch (errno) {

		}
	}

	return EXIT_SUCCESS;
}
