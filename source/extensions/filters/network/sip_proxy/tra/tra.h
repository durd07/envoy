#pragma once

#include <chrono>
#include <memory>
#include <string>
#include <vector>

#include "envoy/common/pure.h"
#include "envoy/singleton/manager.h"
#include "envoy/stream_info/stream_info.h"
#include "envoy/tracing/http_tracer.h"

#include "absl/types/optional.h"
#include "absl/types/any.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace SipProxy {
namespace TrafficRoutingAssistant {

enum class ResponseType {
	UpdateLskpmcResp,
	GetIpFromLskpmcResp,
	SubscribeResp,
	FailureResp,
};

/**
 * Async callbacks used during grpc calls.
 */
class RequestCallbacks {
public:
  virtual ~RequestCallbacks() = default;

  virtual void complete(ResponseType type, absl::any resp) PURE;
};

/**
 * A client used to query a centralized rate limit service.
 */
class Client {
public:
  virtual ~Client() = default;

  /**
   * Cancel an inflight limit request.
   */
  virtual void cancel() PURE;

  virtual void updateLskpmc(RequestCallbacks& callbacks, const std::string lskpmc,
		  Tracing::Span& parent_span, const StreamInfo::StreamInfo& stream_info) PURE;

  virtual void getIpFromLskpmc(RequestCallbacks& callbacks, const std::string key,
		  Tracing::Span& parent_span, const StreamInfo::StreamInfo& stream_info) PURE;

  virtual void subscribe(RequestCallbacks& callbacks, const std::string lskpmc,
		  Tracing::Span& parent_span, const StreamInfo::StreamInfo& stream_info) PURE;
};

using ClientPtr = std::unique_ptr<Client>;

} // namespace TrafficRoutingAssistant
} // namespace SipProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
