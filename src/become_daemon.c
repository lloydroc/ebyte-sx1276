
#include "become_daemon.h"

int // returns 0 on success non-zero on error
become_daemon()
{
  int fd;
  switch(fork())                    // become background process
  {
    case -1: return 1;
    case 0: break;                  // child falls through
    default: _exit(EXIT_SUCCESS);   // parent terminates
  }

  if(setsid() == -1)                // become leader of new session
    return 2;

  switch(fork())
  {
    case -1: return 3;
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
    return 4;
  if(dup2(STDIN_FILENO, STDOUT_FILENO) != STDOUT_FILENO)
    return 5;
  if(dup2(STDIN_FILENO, STDERR_FILENO) != STDERR_FILENO)
    return 6;

  return 0;
}

int
write_pidfile(char *file)
{
  pid_t pid;
  FILE *fp;
  fp = fopen(file, "w");
  if(fp == NULL)
  {
    return 1;
  }
  pid = getpid();
  fprintf(fp, "%d\n", pid);
  fclose(fp);
  return 0;
}
