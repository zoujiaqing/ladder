#ifndef LADDER_CALLBACKS_H
#define LADDER_CALLBACKS_H

#include <functional>
#include <memory>
#include <string>

#ifdef _MSC_VER
#ifdef _WINDLL
#define LADDER_API __declspec(dllexport)
#else
#define LADDER_API __declspec(dllimport)
#endif
#else
#define LADDER_API
#endif

namespace ladder {

class Connection;
class IBuffer;
class Buffer;
class Channel;
class EventLoop;
class EventLoopThreadPool;
class Acceptor;
class ThreadPool;

using ChannelPtr = std::shared_ptr<Channel>;
using EventLoopPtr = std::shared_ptr<EventLoop>;
using EventLoopThreadPoolPtr = std::shared_ptr<EventLoopThreadPool>;
using ThreadPoolPtr = std::shared_ptr<ThreadPool>;
using AcceptorPtr = std::unique_ptr<Acceptor>;
using ConnectionPtr = std::shared_ptr<Connection>;

using ReadEvtCallback = std::function<void(const ConnectionPtr&, Buffer*)>;
using WriteEvtCallback = std::function<void(IBuffer*)>;
using ConnectCloseCallback = std::function<void()>;
using ConnectionEvtCallback = std::function<void(const ConnectionPtr&)>;

class LADDER_API IBuffer {
 public:
  virtual ~IBuffer();
  virtual void Write(const std::string& buf) = 0;
  virtual void Write(const char* src, size_t len) = 0;
#ifndef _MSC_VER
  virtual int WriteBufferToFd(int fd) = 0;
#endif
  virtual uint32_t Peek(char* dst, size_t len) = 0;
  virtual bool Empty() const = 0;
  virtual void HaveRead(size_t) = 0;
};

}  // namespace ladder

#endif
