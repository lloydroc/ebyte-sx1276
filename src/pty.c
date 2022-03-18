#include "pty.h"

int
ttySetRaw(int fd, struct termios *prevTermios)
{
    struct termios t;

    if (tcgetattr(fd, &t) == -1)
        return -1;

    if (prevTermios != NULL)
        *prevTermios = t;

    /* Noncanonical mode, disable signals, extended
       input processing, and echoing */
    t.c_lflag &= ~(ICANON | ISIG | IEXTEN | ECHO);

    /* Disable special handling of CR, NL, and BREAK.
       No 8th-bit stripping or parity error handling.
       Disable START/STOP output flow control. */
    t.c_iflag &= ~(BRKINT | ICRNL | IGNBRK | IGNCR | INLCR |
                      INPCK | ISTRIP | IXON | PARMRK);

    /* Disable all output processing */
    t.c_oflag &= ~OPOST;

    /* Character-at-a-time input with blocking */
    t.c_cc[VMIN] = 1;
    t.c_cc[VTIME] = 0;

    if (tcsetattr(fd, TCSAFLUSH, &t) == -1)
        return -1;

    return 0;
}

/*
 * Opens a slave terminal and stores the name in slaveName.
 * Caller must create a buffer with size snLen bigger than slaveName.
 *
 * returns the master pty file descriptor or -1 on error
 */
static int
ptyMasterOpen(char *slaveName, size_t snLen)
{
  int masterFd, savedErrno;
  char *p;

  masterFd = posix_openpt(O_RDWR | O_NOCTTY);
  if(masterFd == -1)
  {
    return -1;
  }

  if(grantpt(masterFd) == -1) goto ptyMasterOpen_fail;
  if(unlockpt(masterFd) == -1) goto ptyMasterOpen_fail;

  p = ptsname(masterFd);
  if(p == NULL) goto ptyMasterOpen_fail;

  if(strlen(p) < snLen)
  {
    strncpy(slaveName,p,snLen);
  }
  else
  {
    close(masterFd);
    errno = EOVERFLOW;
    return -1;
  }

  return masterFd;
ptyMasterOpen_fail:
    savedErrno = errno;
    close(masterFd);
    errno = savedErrno;
    return -1;
}

pid_t
ptyFork(
    int *masterFd,
    char *slaveName,
    size_t snLen,
    const struct termios *slaveTermios,
    const struct winsize *slaveWS)
{
  int mfd, slaveFd, savedErrno;
  pid_t childPid;
  char slname[MAX_SNAME];

  mfd = ptyMasterOpen(slname,MAX_SNAME);
  if(mfd == -1) return -1;

  /* TODO can have packet mode
  if(ioctl(masterFd, TIOCPKT, 1 == enable, 0 == disable)
   */

  // TODO snLen strange here
  strncpy(slaveName,slname,snLen);

  childPid = fork();
  if(childPid == -1) /* fork failed */
  {
    savedErrno = errno;
    close(mfd);
    errno = savedErrno;
    return -1;
  }

  if(childPid != 0) /* parent */
  {
    *masterFd = mfd;
    return childPid;
  }

  /* child falls through to here */
  if(setsid() == -1) /* start a new session */
  {
    err_exit("ptyFork:setsid");
  }

  close(mfd); /* not needed in child */

  slaveFd = open(slname, O_RDWR); /* becomes controlling tty */
  if(slaveFd == -1)
  {
    err_exit("ptyFork:open-slave");
  }

#ifdef TIOCSCTTY /* acquire controller tty on BSD */
  if(ioctl(slaveFd, TIOCSCTTY, 0) == -1)
  {
    err_exit("ptyFork:ioctl-TIOCSCTTY");
  }
#endif

  if(slaveTermios != NULL) /* set slave tty attributes */
  {
    if(tcsetattr(slaveFd, TCSANOW, slaveTermios) == -1)
      err_exit("ptyFork:tcsetattr-TCSANOW");
  }

  if(slaveWS != NULL) /* set the slave window size */
  {
    if(ioctl(slaveFd, TIOCSWINSZ, slaveWS) == -1)
      err_exit("ptyFork:ioctl-TIOCSWINSZ");
  }

  /* duplicate pty slave to be the childs stdin, stdout and stderr */
  if(dup2(slaveFd, STDIN_FILENO) != STDIN_FILENO)
    err_exit("ptyFork:dup2-STDIN");

  if(dup2(slaveFd, STDOUT_FILENO) != STDOUT_FILENO)
    err_exit("ptyFork:dup2-STDOUT");

  if(dup2(slaveFd, STDERR_FILENO) != STDERR_FILENO)
    err_exit("ptyFork:dup2-STDERR");

  if(slaveFd > STDERR_FILENO) /* safety check */
    close(slaveFd);

  return 0;
}

int
pty_create(int *fd_pty_master)
{
  char slaveName[MAX_SNAME];
  char *shell;
  struct winsize ws;
  pid_t childPid;

  // TODO this comes from the other end?
  struct termios ttyOrig;

  if(tcgetattr(STDIN_FILENO, &ttyOrig) == -1)
    err_exit("tcgetattr tty original");
  if(ioctl(STDIN_FILENO, TIOCGWINSZ, &ws) < 0)
    err_exit("ioctl-TIOCGWINSZ");

  childPid = ptyFork(fd_pty_master, slaveName, MAX_SNAME, &ttyOrig, &ws);
  if(childPid < 0)
  {
    err_output("ptyFork");
    return 3;
  }

  if(childPid == 0)
  {
    shell = getenv("SHELL");
    if(shell == NULL || *shell == '\0')
      shell = "/bin/sh";

    /* child code should end here */
    execlp(shell, shell, (char *) NULL);
    err_exit("execlp"); /* if we got here something went wrong */
  }

  debug_output("child pid: %d\n", childPid);
  debug_output("slave name: %s\n", slaveName);
  debug_output("fileno stdout: %d\n",STDOUT_FILENO);

  return 0;
}