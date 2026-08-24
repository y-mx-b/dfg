#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/types.h>
#include <pwd.h>
#include <string.h>

// TODO: add `-r` option to specify root directory for link paths instead of using $HOME
// ^ once this is done, use option.root instead of a random home variable
#define USAGE "usage: dfg [-hfu] [-s <store-path>] <profile|profile:path>..."
#define HELP \
	"A dotfile configuration utility.\n" \
	"\n" \
	USAGE"\n" \
	"\n" \
	"options:\n" \
	"    -h          Display usage information.\n" \
	"    -f          Overwrite any existing files encountered.\n" \
	"    -u          Unlink the given profiles instead of linking them.\n" \
	"    -s          Path to the profile store. [default: $HOME/.dfg]\n"

int main(int argc, char **argv) {
	struct {
		bool force;
		bool unlink;
		char *store;
	} option = { 0 };

	opterr = 0;
	int opt;
	while ((opt = getopt(argc, argv, "hfus:")) != -1) {
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
			option.store = optarg;
			break;
		case '?':
			if (optopt == 's' && optarg == NULL) {
				fprintf(stderr, "error: expected argument for option `-s`\n");
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

	char *home;
	if (option.store == NULL) {
		char *dfg_store = getenv("DFG_STORE");
		if (dfg_store) {
			option.store = dfg_store;
		} else {
			char default_store[PATH_MAX] = { 0 };
			home = getenv("HOME");
			if (!home) {
				struct passwd *pw = getpwuid(getuid());
				home = pw->pw_dir;
			}
			strlcpy(default_store, home, PATH_MAX);
			strlcat(default_store, "/.dfg", PATH_MAX);
			option.store = default_store;
		}
		printf("use default store: %s\n", option.store);
	}

	while (optind < argc) {
		// TODO: handle ~ at the start of link path
		char *profile = argv[optind++];
		char *link_path = 0;
		char link_buf[PATH_MAX] = { 0 };

		int i = 0;
		char c;
		while ((c = profile[i++]) != ':' && c != '\0');
		if (c == ':') {
			profile[i-1] = '\0';
			link_path = &profile[i];
		} else {
			strlcpy(link_buf, home, PATH_MAX);
			strlcat(link_buf, "/", PATH_MAX);
			strlcat(link_buf, profile, PATH_MAX);
			link_path = link_buf;
		}

		// TODO: implement actual linking/unlinking
		if (option.unlink) {
			printf("unlink: %s:%s\n", profile, link_path);
		} else {
			printf("link: %s:%s\n", profile, link_path);
		}
	}

	return EXIT_SUCCESS;
}
