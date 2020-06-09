#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include "become_daemon.h"

int // returns 0 on success -1 on error
become_daemon()
{
  int fd;
  switch(fork())                    // become background process
  {
    case -1: return -1;
    case 0: break;                  // child falls through
    default: _exit(EXIT_SUCCESS);   // parent terminates
  }

  if(setsid() == -1)                // become leader of new session
    return -1;

  switch(fork())
  {
    case -1: return -1;
    case 0: break;
    default: _exit(EXIT_SUCCESS);
  }

  umask(0);                       // clear file creation mode mask
  chdir("/");                     // change to root directory

  close(STDIN_FILENO);
  close(STDOUT_FILENO);
  close(STDERR_FILENO);

  fd = open("/dev/null", O_RDWR);
  if(fd != STDIN_FILENO)
    return -1;
  if(dup2(STDIN_FILENO, STDOUT_FILENO) != STDOUT_FILENO)
    return -2;
  if(dup2(STDIN_FILENO, STDERR_FILENO) != STDERR_FILENO)
    return -3;

  return 0;
}
