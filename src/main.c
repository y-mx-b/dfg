#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>

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
				fprintf(stderr, USAGE);
			} else {
				fprintf(stderr, "error: unknown option `-%c`\n", optopt);
				fprintf(stderr, USAGE);
			}
		default:
			fprintf(stderr, USAGE);
			exit(EXIT_FAILURE);
			break;
		}
	}

	if (optind >= argc) {
		fprintf(stderr, "error: expected arguments\n");
		fprintf(stderr, USAGE);
	}

	if (option.store == NULL) {
		// TODO
		printf("use default store\n");
	}

	while (optind < argc) {
		// TODO
		printf("arg: %s\n", argv[optind++]);
	}

	return EXIT_SUCCESS;
}
