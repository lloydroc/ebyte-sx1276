#include "error.h"

#define BUF_SIZE 1024

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

static void
output(int priority, const char *format, va_list ap)
{
  char buf[BUF_SIZE];

  vsnprintf(buf, BUF_SIZE, format, ap);

  if(use_syslog)
    syslog(priority, buf);
  else if(priority > LOG_WARNING)
    fputs(buf, stdout);
  else
    fputs(buf, stderr);
}

void
info_output(const char *format, ...)
{
  va_list argList;
  va_start(argList, format);
  output(LOG_DEBUG, format, argList);
  va_end(argList);
}

void
debug_output(const char *format, ...)
{
  va_list argList;
  va_start(argList, format);
  output(LOG_DEBUG, format, argList);
  va_end(argList);
}

void
warn_output(const char *format, ...)
{
  va_list argList;
  va_start(argList, format);
  output(LOG_WARNING, format, argList);
  va_end(argList);
}

void
err_output(const char *format, ...)
{
  va_list argList;
  va_start(argList, format);
  output(LOG_ERR, format, argList);
  va_end(argList);
}

static void
output_errno(int err, const char *format, va_list ap)
{
  char buf[BUF_SIZE], userMsg[BUF_SIZE], errText[BUF_SIZE];

  vsnprintf(userMsg, BUF_SIZE, format, ap);

  snprintf(errText, BUF_SIZE, " [%s %s]",
          (err > 0 && err <= MAX_ENAME) ?
          ename[err] : "?UNKNOWN?", strerror(err));

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
  snprintf(buf, BUF_SIZE, "ERROR%s %s\n", errText, userMsg);
#pragma GCC diagnostic pop

  if(use_syslog)
  {
    syslog(LOG_ERR, buf);
  }
  else
  {
    fflush(stdout);     /* Flush any pending stdout */
    fputs(buf, stderr);
    fflush(stderr);     /* In case stderr is not line-buffered */
  }
}

void
errno_output(const char *format, ...)
{
  va_list argList;
  va_start(argList, format);
  output_errno(errno, format, argList);
  va_end(argList);
}
