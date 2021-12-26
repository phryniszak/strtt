#include "helper_types.h"
#include "helper_replacements.h"

#ifdef __MINGW32__
char *strndup(const char *s, size_t n)
{
	size_t len = strnlen(s, n);
	char *new = malloc(len + 1);

	if (!new)
		return NULL;

	new[len] = '\0';
	return (char *) memcpy(new, s, len);
}
#endif
