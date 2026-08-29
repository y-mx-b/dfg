#include <string.h>
#include <limits.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <stdarg.h>
#include <sys/types.h>
#include <stdlib.h>
#include <sys/errno.h>
#include <stdio.h>

const char *home_dir(void) {
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

// TODO: return total length of attempted write like `strlcat` and `snprintf`
void _path_join(size_t n, char *buf, ...) {
	va_list components;
	va_start(components, buf);

	// TODO: keep track of total length written and replace `strlcat` with
	// `snprintf`
	const char *component;
	memset(buf, 0, n); // clear out buffer so strlcat works properly
	if ((component = va_arg(components, const char *)) != NULL) {
		if (component[0] == '~'
			&& (component[1] == '\0' || component[1] == '/'))
		{
			strlcpy(buf, home_dir(), n);
			component += 1;
		}
		strlcat(buf, component, n);
	}
	while ((component = va_arg(components, const char *)) != NULL) {
		// TODO: remove extraneous path separators
		strlcat(buf, "/", n);
		strlcat(buf, component, n);
	}

	va_end(components);
}

// TODO: remove static err msg buffers, change signature to receive err msg buffer
const char *dfg_link(const char *profile, const char *link) {
	static char err[1024];
	memset(err, 0, 1024);

	if (access(profile, F_OK) != 0) {
		snprintf(err, 1024, "error: profile `%s` does not exist\n", profile);
		return err;
	}

	if (symlink(profile, link) == 0) { return NULL; }
	switch(errno) {
	case EACCES:
		return "error: permission denied";
		break;
	case EDQUOT:
		return "error: disk quota exceed";
		break;
	case EEXIST:
		return "error: link path already exists";
		break;
	case EFAULT:
		return "error: profile or link path points outside accessible address space";
		break;
	case EIO:
		return "error: I/O error";
		break;
	case ELOOP:
		return "error: failed to resolve link path (too many symlinks)";
		break;
	case ENAMETOOLONG:
		return "error: profile or link path too long";
		break;
	case ENOENT:
		return "error: link path has a component that does not exist";
		break;
	case ENOSPC:
		return "error: ran out of space";
		break;
	case ENOTDIR:
		return "error: link path contains a component that is not a directory";
		break;
	case EROFS:
		return "error: link path is on a read-only filesystem";
		break;
	case EILSEQ:
		return "error: filename does not match encoding rules";
		break;
	default:
		return "error: unknown error";
		break;
	}
	// TODO: implement -f flag for replacing existing file system entries
	// TODO: check if profile exists, error if not
}
