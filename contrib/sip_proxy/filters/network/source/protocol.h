#pragma once

#include "envoy/buffer/buffer.h"

#include "contrib/envoy/extensions/filters/network/sip_proxy/tra/v3alpha/tra.pb.h"
#include "contrib/envoy/extensions/filters/network/sip_proxy/tra/v3alpha/tra.pb.validate.h"
#include "contrib/envoy/extensions/filters/network/sip_proxy/v3alpha/sip_proxy.pb.h"
#include "contrib/envoy/extensions/filters/network/sip_proxy/v3alpha/sip_proxy.pb.validate.h"
#include "contrib/sip_proxy/filters/network/source/conn_state.h"
#include "contrib/sip_proxy/filters/network/source/metadata.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace SipProxy {

class SipSettings {
public:
  SipSettings(
      std::chrono::milliseconds transaction_timeout,
      const Protobuf::RepeatedPtrField<
          envoy::extensions::filters::network::sip_proxy::v3alpha::LocalService>& local_services,
      const envoy::extensions::filters::network::sip_proxy::tra::v3alpha::TraServiceConfig&
          tra_service_config,
      bool operate_via)
      : transaction_timeout_(transaction_timeout), tra_service_config_(tra_service_config),
        operate_via_(operate_via) {
    UNREFERENCED_PARAMETER(operate_via_);

    for (auto service : local_services) {
      local_services_.emplace_back(service);
    }
  }
  std::chrono::milliseconds transactionTimeout() { return transaction_timeout_; }
  std::vector<envoy::extensions::filters::network::sip_proxy::v3alpha::LocalService>
  localServices() {
    return local_services_;
  }
  envoy::extensions::filters::network::sip_proxy::tra::v3alpha::TraServiceConfig&
  traServiceConfig() {
    return tra_service_config_;
  }

private:
  std::chrono::milliseconds transaction_timeout_;

  std::vector<envoy::extensions::filters::network::sip_proxy::v3alpha::LocalService>
      local_services_;
  envoy::extensions::filters::network::sip_proxy::tra::v3alpha::TraServiceConfig
      tra_service_config_;
  bool operate_via_;
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
