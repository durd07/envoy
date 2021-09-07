#pragma once

#include <string>

#include "envoy/buffer/buffer.h"
#include "envoy/common/exception.h"

#include "source/common/common/assert.h"
#include "source/common/common/logger.h"
#include "source/common/common/base64.h"

#include "source/extensions/filters/network/sip_proxy/metadata.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace SipProxy {
class Encoder : public Logger::Loggable<Logger::Id::sip> {
public:
  virtual ~Encoder() = default;
  virtual void encode(const MessageMetadataSharedPtr& metadata, Buffer::Instance& out) PURE;
};

class EncoderImpl : public Encoder {
public:
  void encode(const MessageMetadataSharedPtr& metadata, Buffer::Instance& out) override;
};

} // namespace SipProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
