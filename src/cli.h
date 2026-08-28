#ifndef DFG_CLI_H
#define DFG_CLI_H

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

struct option {
	bool help;
	bool dry;
	bool force;
	bool unlink;
	char store[PATH_MAX];
	char root[PATH_MAX];
};

/// Returns error message or NULL for success.
const char *cli_parse(int argc, char **argv, struct option *option);

#endif
