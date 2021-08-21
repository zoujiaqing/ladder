#ifndef LADDER_CHANNEL_H
#define LADDER_CHANNEL_H

#include <stdint.h>

#include <functional>

namespace ladder {

class Channel {
public:
  Channel(int fd);
  int fd() const;
  void SetReadCallback(const std::function<void()>& callback);
  void SetWriteCallback(const std::function<void()>& callback);
  void SetEvents(uint32_t events);
  void HandleEvents();

private:
  int fd_;
  uint32_t events_;
  std::function<void()> read_callback_;
  std::function<void()> write_callback_;
};

} // namespace ladder

#endif
