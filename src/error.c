#include "error.h"

static char *ename[] = {
    /*   0 */ "",
    /*   1 */ "EPERM", "ENOENT", "ESRCH", "EINTR", "EIO", "ENXIO",
    /*   7 */ "E2BIG", "ENOEXEC", "EBADF", "ECHILD", "EDEADLK",
    /*  12 */ "ENOMEM", "EACCES", "EFAULT", "ENOTBLK", "EBUSY",
    /*  17 */ "EEXIST", "EXDEV", "ENODEV", "ENOTDIR", "EISDIR",
    /*  22 */ "EINVAL", "ENFILE", "EMFILE", "ENOTTY", "ETXTBSY",
    /*  27 */ "EFBIG", "ENOSPC", "ESPIPE", "EROFS", "EMLINK", "EPIPE",
    /*  33 */ "EDOM", "ERANGE", "EAGAIN/EWOULDBLOCK", "EINPROGRESS",
    /*  37 */ "EALREADY", "ENOTSOCK", "EDESTADDRREQ", "EMSGSIZE",
    /*  41 */ "EPROTOTYPE", "ENOPROTOOPT", "EPROTONOSUPPORT",
    /*  44 */ "ESOCKTNOSUPPORT", "ENOTSUP", "EPFNOSUPPORT",
    /*  47 */ "EAFNOSUPPORT", "EADDRINUSE", "EADDRNOTAVAIL",
    /*  50 */ "ENETDOWN", "ENETUNREACH", "ENETRESET", "ECONNABORTED",
    /*  54 */ "ECONNRESET", "ENOBUFS", "EISCONN", "ENOTCONN",
    /*  58 */ "ESHUTDOWN", "ETOOMANYREFS", "ETIMEDOUT", "ECONNREFUSED",
    /*  62 */ "ELOOP", "ENAMETOOLONG", "EHOSTDOWN", "EHOSTUNREACH",
    /*  66 */ "ENOTEMPTY", "EPROCLIM", "EUSERS", "EDQUOT", "ESTALE",
    /*  71 */ "EREMOTE", "EBADRPC", "ERPCMISMATCH", "EPROGUNAVAIL",
    /*  75 */ "EPROGMISMATCH", "EPROCUNAVAIL", "ENOLCK", "ENOSYS",
    /*  79 */ "EFTYPE", "EAUTH", "ENEEDAUTH", "EPWROFF", "EDEVERR",
    /*  84 */ "EOVERFLOW", "EBADEXEC", "EBADARCH", "ESHLIBVERS",
    /*  88 */ "EBADMACHO", "ECANCELED", "EIDRM", "ENOMSG", "EILSEQ",
    /*  93 */ "ENOATTR", "EBADMSG", "EMULTIHOP", "ENODATA", "ENOLINK",
    /*  98 */ "ENOSR", "ENOSTR", "EPROTO", "ETIME", "EOPNOTSUPP",
    /* 103 */ "ENOPOLICY", "ENOTRECOVERABLE", "EOWNERDEAD", "ELAST",
    /* 106 */ "EQFULL"
};


/*static void
output_error(bool useErr, int err, bool flushStdout, const char *format, va_list ap);

static void
terminate(bool useExit3);
*/

static void
output_error(bool useErr, int err, bool flushStdout, const char *format, va_list ap)
{
#define BUF_SIZE 1024
    char buf[BUF_SIZE], userMsg[BUF_SIZE], errText[BUF_SIZE];

    vsnprintf(userMsg, BUF_SIZE, format, ap);

    if (useErr)
        snprintf(errText, BUF_SIZE, " [%s %s]",
                (err > 0 && err <= MAX_ENAME) ?
                ename[err] : "?UNKNOWN?", strerror(err));
    else
        snprintf(errText, BUF_SIZE, ":");

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
    snprintf(buf, BUF_SIZE, "ERROR%s %s\n", errText, userMsg);
#pragma GCC diagnostic pop

    if (flushStdout)
        fflush(stdout);       /* Flush any pending stdout */
    fputs(buf, stderr);
    fflush(stderr);           /* In case stderr is not line-buffered */
}

static void
terminate(bool useExit3)
{
    char *s;

    /* Dump core if EF_DUMPCORE environment variable is defined and
       is a nonempty string; otherwise call exit(3) or _exit(2),
       depending on the value of 'useExit3'. */

    s = getenv("EF_DUMPCORE");

    if (s != NULL && *s != '\0')
        abort();
    else if (useExit3)
        exit(EXIT_FAILURE);
    else
        _exit(EXIT_FAILURE);
}

void
err_output(const char *format, ...)
{

  va_list argList;

  va_start(argList, format);
  output_error(true, errno, true, format, argList);
  va_end(argList);
}

void
err_exit(const char *format, ...)
{

  va_list argList;

  va_start(argList, format);
  err_output(format, argList);
  va_end(argList);

  terminate(false);
}
