#ifndef LADDER_CODEC_H
#define LADDER_CODEC_H

#include <Base.h>

namespace ladder {

class LADDER_API Codec {
 public:
  virtual ~Codec();
  void Send(const ConnectionPtr& conn, const void* message) const;
  void OnMessage(const ConnectionPtr& conn, Buffer* buffer);

  void Encapsulate(const void* message, std::string& packet) const;
  bool Decapsulate(std::string& raw_data, Buffer* buffer) const;

  virtual void ComposeMessage(const void* message, std::string& buf) const = 0;
  virtual bool ParseMessage(const std::string& packet,
                            void*& message) const = 0;
  virtual bool HandleMessage(const ConnectionPtr& conn,
                             void* message) const = 0;

  void ParseAndHandleMessage(const ConnectionPtr& conn,
                             const std::string& packet) const;

  static const uint32_t kHeaderLength;
};

}  // namespace ladder

#endif
