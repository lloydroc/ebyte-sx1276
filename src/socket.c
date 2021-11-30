#include "socket.h"

int
socket_create_unix(char *filename, struct sockaddr_un *sock)
{
  memset(sock, 0, sizeof(struct sockaddr_un));
  sock->sun_family = AF_UNIX;
  strncpy(sock->sun_path, filename, sizeof(sock->sun_path)-1);

  return 0;
}

int
socket_unix_bind(char *filename, int *fd, struct sockaddr_un *sock)
{
  char path[1024];
  char *sockdir;

  sockdir = strdup(filename);
  sockdir = dirname(sockdir);

  if(access(sockdir, F_OK))
  {
      if(mkdir(sockdir, S_IRUSR | S_IXUSR | S_IWUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH))
      {
          errno_output("creating directory %s\n", sockdir);
      }
  }

  free(sockdir);

  memset(sock, 0, sizeof(struct sockaddr_un));
  sock->sun_family = AF_UNIX;

  *fd = socket(AF_UNIX, SOCK_DGRAM, 0);
  if(*fd == -1)
  {
    errno_output("socket_unix_bind: error opening socket\n");
    return 1;
  }

  if(strlen(filename) > sizeof(sock->sun_path)-1)
  {
    err_output("socket_unix_bind: socket path too long must be %d chars\n", sizeof(sock->sun_path)-1);
    return 3;
  }

  if(remove(filename) == -1 && errno != ENOENT)
  {
    errno_output("socket_unix_bind: error removing socket\n");
    return 4;
  }

  strncpy(sock->sun_path, filename, sizeof(sock->sun_path)-1);

  if(bind(*fd, (struct sockaddr*) sock, sizeof(struct sockaddr_un)) == -1)
  {
    errno_output("socket_unix_bind: error binding to socket %s\n", filename);
    return 5;
  }

  if(realpath(filename, path) == NULL)
  {
    errno_output("socket_create_unix: socket full path too long, must be %d chars\n", sizeof(sock->sun_path)-1);
    return 2;
  }

  /* update the socket path so the the destination sockets have the full path */
  strncpy(sock->sun_path, path, sizeof(sock->sun_path)-1);

  return 0;
}

int
socket_unix_receive(int fd_sock, uint8_t *buf, size_t buf_len, ssize_t *received_bytes, struct sockaddr_un *sock_source)
{
    int flags = 0;
    socklen_t addrlen;
    addrlen = sizeof(struct sockaddr_un);

    *received_bytes = recvfrom(fd_sock, buf, buf_len, flags, (struct sockaddr*) sock_source, &addrlen);
    if(*received_bytes == -1)
    {
        errno_output("socket_unix_send_with_ack: error receiving from unix domain socket");
        return 1;
    }

    return 0;
}

int
socket_unix_send(int fd_sock, struct sockaddr_un *sock_dest, uint8_t *data, size_t len)
{
    size_t sent_bytes;
    int flags = 0;
    socklen_t addrlen;
    addrlen = sizeof(struct sockaddr_un);

    /* send the data to the destination socket from the source */
    sent_bytes = sendto(fd_sock, data, len, flags, (struct sockaddr*) sock_dest, addrlen);
    if(sent_bytes == -1)
    {
      errno_output("socket_unix_send: unable to send %d bytes from socket fd %d to socket %s\n", len, fd_sock, sock_dest->sun_path);
      return 1;
    }
    return 0;
}

int
socket_unix_send_with_ack(int fd_sock, struct sockaddr_un sock_dest, uint8_t *data, size_t len, int timeout_ms)
{
    struct sockaddr_un sock_source;
    uint8_t buffer[1]; // we expect back a status after sendto
    size_t sent_bytes, received_bytes;
    int flags = 0;

    socklen_t addrlen;
    addrlen = sizeof(struct sockaddr_un);

    /* send the data to the destination socket from the source */
    sent_bytes = sendto(fd_sock, data, len, flags, (struct sockaddr*) &sock_dest, addrlen);
    if(sent_bytes == -1)
    {
      errno_output("socket_unix_send_with_ack: unable to send %d bytes from socket fd %d to socket %s\n", len, fd_sock, sock_dest.sun_path);
      return 1;
    }

    int ret = socket_unix_poll_source_read_ready(fd_sock, timeout_ms);
    if (ret == 1)
    {
        return 2;
    }
    else if(ret)
    {
        errno_output("socket_unix_send_with_ack: poll");
        return 3;
    }

    /* receive back the ack from the socket by polling the source */
    received_bytes = recvfrom(fd_sock, &buffer, 1, flags, (struct sockaddr*) &sock_source, &addrlen);

    if(received_bytes == -1)
    {
        errno_output("socket_unix_send_with_ack: error receiving from unix domain socket");
        return 4;
    }
    else if(received_bytes != 1)
    {
        errno_output("socket_unix_send_with_ack: error receiving from unix domain socket");
        return 5;
    }
    else if(strncmp(sock_source.sun_path, sock_dest.sun_path, addrlen))
    {
        errno_output("socket_unix_send_with_ack: destination socket and source socket to acknowledge are not the same");
        return 6;
    }

    return buffer[0];
}

int
socket_unix_poll_source_read_ready(int fd_sock, int timeout_ms)
{
    struct pollfd pfd;
    pfd.fd = fd_sock;
    pfd.events = POLLIN;
    int ret = poll(&pfd, 1, timeout_ms);
    if (ret == 0)
    {
        err_output("socket_unix_poll_source_read_ready: poll timed out from fd %d after %d ms\n", fd_sock, timeout_ms);
        return 1;
    }
    else if(ret < 0)
    {
        errno_output("socket_unix_poll_source_read_ready: poll fd %d" ,fd_sock);
        return 2;
    }

    return 0;
}
