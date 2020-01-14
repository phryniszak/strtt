#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <malloc.h>

#include <errno.h>
#include <limits.h>

#include "log.h"

int debug_level = -1;
static FILE *log_output;

static int64_t start;
static int64_t last_time;

static const char *const log_strings[6] = {
    "User : ",
    "Error: ",
    "Warn : ", /* want a space after each colon, all same width, colons aligned */
    "Info : ",
    "Debug: ",
    "Debug: "};

static int count;

//-----------------------------------------------------------------------------------------------

/* simple and low overhead fetching of ms counter. Use only
 * the difference between ms counters returned from this fn.
 */
int64_t timeval_ms(void)
{
    struct timeval now;
    int retval = gettimeofday(&now, NULL);
    if (retval < 0)
        return retval;
    return (int64_t)now.tv_sec * 1000 + now.tv_usec / 1000;
}

/* The log_puts() serves two somewhat different goals:
 *
 * - logging
 * - feeding low-level info to the user in GDB or Telnet
 *
 * The latter dictates that strings without newline are not logged, lest there
 * will be *MANY log lines when sending one char at the time(e.g.
 * target_request.c).
 *
 */
static void log_puts(enum log_levels level,
                     const char *file,
                     int line,
                     const char *function,
                     const char *string)
{
    const char *f;
    if (level == LOG_LVL_OUTPUT)
    {
        /* do not prepend any headers, just print out what we were given and return */
        fputs(string, log_output);
        fflush(log_output);
        return;
    }

    f = strrchr(file, '/');
    if (f != NULL)
        file = f + 1;

    if (strlen(string) > 0)
    {
        if (debug_level >= LOG_LVL_DEBUG)
        {
            /* print with count and time information */
            int64_t t = timeval_ms() - start;
#ifdef _DEBUG_FREE_SPACE_
            struct mallinfo info;
            info = mallinfo();
#endif
            fprintf(log_output, "%s%d %lld %s:%d %s()"
#ifdef _DEBUG_FREE_SPACE_
                                " %d"
#endif
                                ": %s",
                    log_strings[level + 1], count, (long long int)t, file, line, function,
#ifdef _DEBUG_FREE_SPACE_
                    info.fordblks,
#endif
                    string);
        }
        else
        {
            /* if we are using gdb through pipes then we do not want any output
			 * to the pipe otherwise we get repeated strings */
            fprintf(log_output, "%s%s",
                    (level > LOG_LVL_USER) ? log_strings[level + 1] : "", string);
        }
    }
    else
    {
        /* Empty strings are sent to log callbacks to keep e.g. gdbserver alive, here we do
		 *nothing. */
    }

    fflush(log_output);
}

/* return allocated string w/printf() result */
char *alloc_vprintf(const char *fmt, va_list ap)
{
    va_list ap_copy;
    int len;
    char *string;

    /* determine the length of the buffer needed */
    va_copy(ap_copy, ap);
    len = vsnprintf(NULL, 0, fmt, ap_copy);
    va_end(ap_copy);

    /* allocate and make room for terminating zero. */
    /* FIXME: The old version always allocated at least one byte extra and
	 * other code depend on that. They should be probably be fixed, but for
	 * now reserve the extra byte. */
    string = (char *)malloc(len + 2);
    if (string == NULL)
        return NULL;

    /* do the real work */
    vsnprintf(string, len + 1, fmt, ap);

    return string;
}

void log_printf(enum log_levels level,
                const char *file,
                unsigned line,
                const char *function,
                const char *format,
                ...)
{
    char *string;
    va_list ap;

    count++;
    if (level > debug_level)
        return;

    va_start(ap, format);

    string = alloc_vprintf(format, ap);
    if (string != NULL)
    {
        log_puts(level, file, line, function, string);
        free(string);
    }

    va_end(ap);
}

void log_vprintf_lf(enum log_levels level, const char *file, unsigned line,
                    const char *function, const char *format, va_list args)
{
    char *tmp;

    count++;

    if (level > debug_level)
        return;

    tmp = alloc_vprintf(format, args);

    if (!tmp)
        return;

    /*
	 * Note: alloc_vprintf() guarantees that the buffer is at least one
	 * character longer.
	 */
    strcat(tmp, "\n");
    log_puts(level, file, line, function, tmp);
    free(tmp);
}

void log_printf_lf(enum log_levels level,
                   const char *file,
                   unsigned line,
                   const char *function,
                   const char *format,
                   ...)
{
    va_list ap;

    va_start(ap, format);
    log_vprintf_lf(level, file, line, function, format, ap);
    va_end(ap);
}

// https://stackoverflow.com/questions/28667132/is-there-a-way-to-use-the-c-standard-library-to-convert-string-int-and-keep/
int strtoi(const char *data, char **endptr, int base)
{
    int old_errno = errno;
    errno = 0;
    long lval = strtol(data, endptr, base);
    if (lval > INT_MAX)
    {
        errno = ERANGE;
        lval = INT_MAX;
    }
    else if (lval < INT_MIN)
    {
        errno = ERANGE;
        lval = INT_MIN;
    }
    if (errno == 0)
        errno = old_errno;
    return (int)lval;
}

// https://stackoverflow.com/questions/28667132/is-there-a-way-to-use-the-c-standard-library-to-convert-string-int-and-keep/
long parse_int(char *cp, int *i)
{
    char *end;
    *i = strtoi(cp, &end, 10); // You want decimal
    return (end - cp);
}

void log_init()
{
    /* set defaults for daemon configuration,
	 * if not set by cmdline or cfgfile */
    if (debug_level == -1)
        debug_level = LOG_LVL_INFO;

    char *debug_env = getenv("OPENOCD_DEBUG_LEVEL");
    if (NULL != debug_env)
    {
        int value;
        int retval = parse_int(debug_env, &value);
        if (ERROR_OK == retval &&
            debug_level >= LOG_LVL_SILENT &&
            debug_level <= LOG_LVL_DEBUG_IO)
            debug_level = value;
    }

    if (log_output == NULL)
        log_output = stderr;

    start = last_time = timeval_ms();
}

int set_log_output(FILE *output)
{
    log_output = output;
    return ERROR_OK;
}