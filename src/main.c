#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/types.h>
#include <pwd.h>
#include <string.h>

#define USAGE "usage: dfg [-hfu] [-s <store-path>] [-r <root-path>] <profile|profile:link>..."
#define HELP \
	"A dotfile configuration utility.\n" \
	"\n" \
	USAGE"\n" \
	"\n" \
	"options:\n" \
	"    -h          Display usage information.\n" \
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

int main(int argc, char **argv) {
	struct {
		bool force;
		bool unlink;
		char store[PATH_MAX];
		char root[PATH_MAX];
	} option = { 0 };

	opterr = 0;
	int opt;
	while ((opt = getopt(argc, argv, "hfus:r:")) != -1) {
		switch (opt) {
		case 'h':
			printf(HELP);
			exit(EXIT_SUCCESS);
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
				fprintf(stderr, "error: expected argument for option `-s`\n");
				fprintf(stderr, "%s\n", USAGE);
			} else if (optopt == 'r' && optarg == NULL) {
				fprintf(stderr, "error: expected argument for option `-r`\n");
				fprintf(stderr, "%s\n", USAGE);
			} else {
				fprintf(stderr, "error: unknown option `-%c`\n", optopt);
				fprintf(stderr, "%s\n", USAGE);
			}
		default:
			fprintf(stderr, "%s\n", USAGE);
			exit(EXIT_FAILURE);
			break;
		}
	}

	if (optind >= argc) {
		fprintf(stderr, "error: expected arguments\n");
		fprintf(stderr, "%s\n", USAGE);
	}

	if (option.root[0] == 0) {
		strlcpy(option.root, home_dir(), PATH_MAX);
	}
	printf("root: %s\n", option.root);

	if (option.store[0] == 0) {
		char *dfg_store = getenv("DFG_STORE");
		if (dfg_store) {
			strlcpy(option.store, dfg_store, PATH_MAX);
		} else {
			strlcpy(option.store, home_dir(), PATH_MAX);
			strlcat(option.store, "/.dfg", PATH_MAX);
		}
	}
	printf("store: %s\n", option.store);

	while (optind < argc) {
		char *arg = argv[optind++];
		char profile[PATH_MAX] = { 0 };
		char link[PATH_MAX] = { 0 };

		int i = 0;
		char c;
		// search for colon separator
		while ((c = arg[i++]) != ':' && c != '\0');
		// if colon separator exists, overwrite colon with '\0' to split
		// the string such at `arg` is the profile and `arg[i]` is the link
		if (c == ':') { arg[i-1] = '\0'; }

		// build profile path
		if (snprintf(profile, PATH_MAX, "%s/%s", option.store, arg) >= PATH_MAX) {
			fprintf(stderr, "error: profile path too long\n");
			exit(EXIT_FAILURE);
		}

		// build link path
		size_t n;
		switch (c) {
		case '/':
			n = strlcpy(link, &arg[i], PATH_MAX);
			break;
		case '~':
			n = snprintf(link, PATH_MAX, "%s/%s", home_dir(), &arg[i+2]);
			break;
		default:
			n = snprintf(link, PATH_MAX, "%s/%s", option.root, arg);
			break;
		}
		if (n >= PATH_MAX) {
			fprintf(stderr, "error: link path too long\n");
			exit(EXIT_FAILURE);
		}

		// TODO: implement actual linking/unlinking
		if (option.unlink) {
			printf("unlink: %s : %s\n", profile, link);
		} else {
			printf("link: %s : %s\n", profile, link);
		}
	}

	return EXIT_SUCCESS;
}
