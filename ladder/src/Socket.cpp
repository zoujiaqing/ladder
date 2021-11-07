#ifdef __unix__
#include <unistd.h>
#endif
#ifdef __linux__
#include <sys/sendfile.h>
#endif
#ifdef _MSC_VER
#pragma comment(lib, "mswsock.lib")
#endif

#include <mutex>

#include <Socket.h>
#include <utils.h>

namespace ladder {

SocketAddr::SocketAddr(bool ipv6) : ipv6_(ipv6) { ; }

SocketAddr::SocketAddr(sockaddr_t* addr, bool ipv6) : ipv6_(ipv6) {
  char buf[50];
  if (ipv6) {
    if (inet_ntop(AF_INET6, &(addr->addr6_.sin6_addr), buf, sizeof(buf)) ==
        NULL) {
      EXIT("inet_pton");
    }
    ip_ = std::string(buf);
    port_ = ntohs(addr->addr6_.sin6_port);
  } else {
    if (inet_ntop(AF_INET, &(addr->addr_.sin_addr), buf, sizeof(buf)) == NULL) {
      EXIT("inet_pton");
    }
    ip_ = std::string(buf);
    port_ = ntohs(addr->addr_.sin_port);
  }
}

SocketAddr::SocketAddr(const std::string& ip, uint16_t port, bool ipv6)
    : ip_(ip), port_(port), ipv6_(ipv6) {
  if (ipv6) {
    sa_.addr6_.sin6_family = AF_INET6;
    if (inet_pton(AF_INET6, ip.c_str(), &sa_.addr6_.sin6_addr) < 0) {
      EXIT("inet_pton");
    }
    sa_.addr6_.sin6_port = htons(port);
  } else {
    sa_.addr_.sin_family = AF_INET;
    if (inet_pton(AF_INET, ip.c_str(), &sa_.addr_.sin_addr) < 0) {
      EXIT("inet_pton");
    }
    sa_.addr_.sin_port = htons(port);
  }
}

void SocketAddr::Bind(int fd) {
  socklen_t sa_len = ipv6_ ? sizeof(sa_.addr6_) : sizeof(sa_.addr_);
  int ret = ::bind(fd, (struct sockaddr*)&sa_, sa_len);
  if (ret < 0) {
    EXIT("bind");
  }
}

std::string SocketAddr::ip() const { return ip_; }

uint16_t SocketAddr::port() const { return port_; }

bool SocketAddr::ipv6() const { return ipv6_; }

const sockaddr_t* SocketAddr::addr() const { return &sa_; }

namespace socket {

int socket(bool tcp, bool ipv6) {
#ifdef __unix__
  int fd = ::socket(ipv6 ? AF_INET6 : AF_INET,
                    (tcp ? SOCK_STREAM : SOCK_DGRAM) | SOCK_NONBLOCK, 0);
  if (fd < 0) {
#endif
#ifdef _MSC_VER
    int fd = WSASocket(ipv6 ? AF_INET6 : AF_INET,
                       tcp ? SOCK_STREAM : SOCK_DGRAM,
                       IPPROTO_IP,
                       NULL,
                       0,
                       WSA_FLAG_OVERLAPPED);
  if (fd == INVALID_SOCKET) {
#endif
    EXIT("socket error: %d", WSAGetLastError());
  }
#ifdef __unix__
  int enable = kEnableOption;
  if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable))) {
#elif defined _MSC_VER
  int zero = 0;
  if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (char*)&zero, sizeof(zero)) == SOCKET_ERROR) {
#endif
    EXIT("setsockopt");
  }
  return fd;
}

int listen(int fd) {
#ifdef _MSC_VER
  int ret = ::listen(fd, SOMAXCONN);
  if (ret < 0) {
    EXIT("listen error: %d", WSAGetLastError());
  }
#else
  int ret = ::listen(fd, 0);
  if (ret < 0) {
    EXIT("listen");
  }
#endif
  return ret;
}

#ifdef _MSC_VER
int accept(int fd, char* buffer, LPFN_ACCEPTEX fn_acceptex,
           SocketIocpStatus* status, bool ipv6) {
  int ret;
  int acceptfd = socket(true, ipv6);
  if (acceptfd == -1) return -1;

  DWORD bytes_recved = 0;

  socklen_t sa_len = ipv6 ? sizeof(sockaddr_in6) : sizeof(sockaddr_in);
  ret = fn_acceptex(fd, acceptfd, (LPVOID)buffer,
                    kMaxIocpRecvSize - 2 * (sa_len + 16),
                    sa_len + 16, sa_len + 16,
                    &bytes_recved, (LPOVERLAPPED)status);
  if (ret == SOCKET_ERROR && ERROR_IO_PENDING != WSAGetLastError()) {
    EXIT("AcceptEx error: %d", WSAGetLastError());
  }

  return acceptfd;
}
#else
int accept(int fd, sockaddr_t* addr, socklen_t* addr_len) {
#ifdef __linux
  int accepted = ::accept4(fd, (struct sockaddr*)addr, addr_len, SOCK_NONBLOCK);
#endif
#ifdef _MSC_VER
  int accepted = ::accept(fd, (struct sockaddr*)addr, addr_len);
#endif
  if (accepted < 0) {
    EXIT("accept");
  }
#ifdef _MSC_VER
  u_long enabled = static_cast<u_long>(kEnableOption);
  if (ioctlsocket(accepted, FIONBIO, &enabled) != NO_ERROR) {
    EXIT("ioctlsocket");
  }
#endif
  return accepted;
}
#endif

int connect(int fd, const sockaddr_t* addr, socklen_t addr_len) {
  int ret = ::connect(fd, (const struct sockaddr*)addr, addr_len);
  return ret;
}

#ifdef _MSC_VER
int write(int fd, LPWSABUF buf, SocketIocpStatus* status) {
  DWORD bytes_sent = 0;
  
  int ret = WSASend(fd, buf, 1, &bytes_sent, 0, (LPWSAOVERLAPPED)status, NULL);

  if (ret == SOCKET_ERROR &&
      ERROR_IO_PENDING != WSAGetLastError()) {
    EXIT("WSASend");
  }

  return ret;
}
#else
int write(int fd, const void* buf, size_t len) {
  return ::write(fd, buf, len);
}
#endif

#ifdef _MSC_VER
int read(int fd, LPWSABUF buf, SocketIocpStatus* status) {
  DWORD bytes_recved = 0;
  DWORD flags = 0;
  buf->len = kMaxIocpRecvSize;

  int ret =
      WSARecv(fd, buf, 1, &bytes_recved, &flags, (LPWSAOVERLAPPED)status, NULL);

  if (ret == SOCKET_ERROR && (ERROR_IO_PENDING != WSAGetLastError())) {
    EXIT("WSARecv error: %d", WSAGetLastError());
  }

  return ret;
}
#else
int read(int fd, void* buf, size_t len) {
  return ::read(fd, buf, len);
}
#endif

#ifdef __unix__
int sendfile(int out_fd, int in_fd, off_t* offset, size_t count) {
#ifdef __linux__
  int ret = ::sendfile(out_fd, in_fd, offset, count);
#elif defined(__FreeBSD__)
  off_t bytes_sent;
  int success = ::sendfile(in_fd, out_fd, *offset, count, NULL, &bytes_sent, 0);
  int ret = (success == 0) ? static_cast<int>(bytes_sent) : 0;
#endif
  return ret;
}
#elif defined(_MSC_VER)
int sendfile(int out_fd, HANDLE in_fd, off_t* offset, size_t count) {
  // TODO: asynchronous send with iocp
  // bool success = TransmitFile(out_fd, in_fd, 0, 0, 0, NULL, 0);
  return -1;
}
#endif

int shutdown_write(int fd) {
#ifdef __unix__
  int ret = ::shutdown(fd, SHUT_WR);
#endif
#ifdef _MSC_VER
  int ret = ::shutdown(fd, SD_SEND);
#endif
  if (ret < 0) {
    switch (errno) {
      case ENOTCONN:
        break;
      default:
        // EXIT("shutdown SHUT_WR");
        break;
    }
  }
  return ret;
}

int shutdown_read(int fd) {
#ifdef __unix__
  int ret = ::shutdown(fd, SHUT_RD);
#endif
#ifdef _MSC_VER
  int ret = ::shutdown(fd, SD_RECEIVE);
#endif
  
  if (ret < 0) {
    EXIT("shutdown SHUT_RD");
  }
  return ret;
}

int close(int fd) {
#ifdef __unix__
  int ret = ::close(fd);
  if (ret < 0) {
#endif
#ifdef _MSC_VER
  int ret = ::closesocket(fd);
  if (ret == SOCKET_ERROR) {
#endif
    EXIT("close");
  }
  return ret;
}

int getsockname(int fd, sockaddr_t* addr, socklen_t* addr_len) {
  int ret = ::getpeername(fd, (struct sockaddr*)addr, addr_len);
  if (ret < 0) {
#ifdef _MSC_VER
    EXIT("getsockname error: %d", WSAGetLastError());
#else
    EXIT("getsockname");
#endif
  }
  return ret;
}

int getsockerropt(int fd) {
  int err;
  socklen_t len = sizeof(int);
  getsockopt(fd, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&err), &len);
  return err;
}

}  // namespace socket

}  // namespace ladder
