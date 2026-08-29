#ifndef DFG_PATH_H
#define DFG_PATH_H

/// Return a lazily initialized static path to the user's home directory.'
const char *home_dir(void);

/// Internal function to join path components into a single path.
void _path_join(size_t n, char *buf, ...);

/// Helper macro to join path components into a single path.
#define path_join(n, buf, ...) _path_join(n, buf, __VA_ARGS__, NULL);

/// Link profile. Returns error message on failure or NULL on success.
const char *dfg_link(const char *profile, const char *link);

/// Link profile. Returns error message on failure or NULL on success.
const char *dfg_unlink(const char *profile, const char *link);

#endif
