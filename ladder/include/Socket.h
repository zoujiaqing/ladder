#ifndef LADDER_SOCKET_H
#define LADDER_SOCKET_H

#include <stdint.h>

#include <compat.h>

#ifdef LADDER_OS_UNIX
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#ifdef LADDER_OS_LINUX
#include <sys/epoll.h>
#endif
#endif
#ifdef LADDER_OS_WINDOWS
#include <winsock2.h>
#include <ws2ipdef.h>
#include <ws2tcpip.h>
// mswsock.h should be included after winsock2.h
#include <mswsock.h>
#endif
#include <fcntl.h>

#include <mutex>
#include <string>

#include <openssl/ssl.h>

#include <Base.h>

namespace ladder {

enum kPollEvent : uint32_t {
#ifdef LADDER_OS_LINUX
  kPollIn = EPOLLIN,
  kPollOut = EPOLLOUT,
  kPollPri = EPOLLPRI,
  kPollRdHup = EPOLLRDHUP,
  kPollHup = EPOLLHUP,
  kPollErr = EPOLLERR,
  kPollEt = EPOLLET,
#else
  kPollIn = 1 << 0,
  kPollOut = 1 << 2,
  kPollErr = 1 << 3,
#endif
};

#ifdef LADDER_OS_WINDOWS
struct SocketIocpStatus {
  WSAOVERLAPPED overlapped_;
  int status_;
  int ref_count_;
  std::mutex mutex_ref_count_;

  SocketIocpStatus(int status = 0) : status_(status), ref_count_(0) { Reset(); };
  void Reset() { memset(&overlapped_, 0, sizeof(overlapped_)); }
  void UpdateRefCount(bool increment = true) {
    std::lock_guard<std::mutex> lock(mutex_ref_count_);
    ref_count_ += increment ? 1 : (-1);
  }
  bool FreeOfRef() {
    std::lock_guard<std::mutex> lock(mutex_ref_count_);
    return ref_count_ == 0;
  }
};
#endif

typedef union {
  struct sockaddr_in addr_;
  struct sockaddr_in6 addr6_;
} sockaddr_t;

class LADDER_API SocketAddr {
 public:
  SocketAddr(bool ipv6 = true);
  SocketAddr(const sockaddr_t* addr, bool ipv6 = true);
  SocketAddr(const std::string& ip, uint16_t port, bool ipv6 = true);
  void Bind(int fd) const;
  std::string ip() const;
  uint16_t port() const;
  bool ipv6() const;
  const sockaddr_t* addr() const;

 private:
  std::string ip_;
  uint16_t port_;
  sockaddr_t sa_;
  bool ipv6_;
};

namespace socket {

int socket(bool tcp = true, bool ipv6 = true);
int listen(int fd);
#ifdef LADDER_OS_WINDOWS
int accept(int fd, char* buffer, LPFN_ACCEPTEX fn_acceptex,
           SocketIocpStatus* status, bool ipv6 = false);
int connect(int fd, const sockaddr_t* addr, socklen_t addr_len,
            LPFN_CONNECTEX fn_connectex,
            SocketIocpStatus* status, bool ipv6);
int write(int fd, LPWSABUF buf, SocketIocpStatus* status);
int read(int fd, LPWSABUF buf, SocketIocpStatus* status);
int sendfile(int out_fd, HANDLE in_fd, off_t* offset, size_t count);
#else
int accept(int fd, sockaddr_t* addr, socklen_t* addr_len);
int connect(int fd, const sockaddr_t* addr, socklen_t addr_len);
int write(int fd, const void* buf, size_t len);
int read(int fd, void* buf, size_t len);
#ifdef LADDER_OS_UNIX
int sendfile(int out_fd, int in_fd, off_t* offset, size_t count);
#endif
#endif

int shutdown_write(int fd);
int shutdown_read(int fd);
int close(int fd);
int getsockname(int fd, sockaddr_t* addr, socklen_t* addr_len);
int getpeername(int fd, sockaddr_t* addr, socklen_t* addr_len);
int getsockerropt(int fd);

}  // namespace socket

}  // namespace ladder

#endif
