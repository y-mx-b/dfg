#include <string.h>
#include <limits.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <stdarg.h>
#include <sys/types.h>
#include <stdlib.h>

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
