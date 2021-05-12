#pragma once

#include "envoy/buffer/buffer.h"
#include "extensions/filters/network/sip_proxy/conn_state.h"
#include "extensions/filters/network/sip_proxy/metadata.h"
namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace SipProxy {

class SipSettings {
public:
  SipSettings(std::chrono::milliseconds transaction_timeout, bool session_stickness)
      : transaction_timeout_(transaction_timeout), session_stickness_(session_stickness) {}
  std::chrono::milliseconds transactionTimeout() { return transaction_timeout_; }
  bool sessionStickness() { return session_stickness_; }

private:
  std::chrono::milliseconds transaction_timeout_;
  bool session_stickness_;
};

/**
 * A DirectResponse manipulates a Protocol to directly create a Sip response message.
 */
class DirectResponse {
public:
  virtual ~DirectResponse() = default;

  enum class ResponseType {
    // DirectResponse encodes MessageType::Reply with success payload
    SuccessReply,

    // DirectResponse encodes MessageType::Reply with an exception payload
    ErrorReply,

    // DirectResponse encodes MessageType::Exception
    Exception,
  };

  /**
   * Encodes the response via the given Protocol.
   * @param metadata the MessageMetadata for the request that generated this response
   * @param proto the Protocol to be used for message encoding
   * @param buffer the Buffer into which the message should be encoded
   * @return ResponseType indicating whether the message is a successful or error reply or an
   *         exception
   */
  virtual ResponseType encode(MessageMetadata& metadata, Buffer::Instance& buffer) const PURE;
};
} // namespace SipProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
