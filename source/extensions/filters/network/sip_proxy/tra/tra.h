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
  CreateLskpmcResp,
	UpdateLskpmcResp,
	RetrieveLskpmcResp,
	DeleteLskpmcResp,
	SubscribeLskpmcResp,
	UpdateXafiResp,
	RetrieveXafiResp,
	DeleteXafiResp,
	SubscribeXafiesp,
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

  virtual void setRequestCallbacks(RequestCallbacks& callbacks) PURE;

  virtual void cancel() PURE;

  virtual void closeStream() PURE;

  virtual void createLskpmc(const std::string lskpmc, Tracing::Span& parent_span, const StreamInfo::StreamInfo& stream_info) PURE;

  virtual void updateLskpmc(const std::pair<std::string &&, std::string &&> && lskpmc, Tracing::Span& parent_span, const StreamInfo::StreamInfo& stream_info) PURE;

  virtual void retrieveLskpmc(const std::string lskpmc, Tracing::Span& parent_span, const StreamInfo::StreamInfo& stream_info) PURE;

  virtual void deleteLskpmc(const std::string lskpmc, Tracing::Span& parent_span, const StreamInfo::StreamInfo& stream_info) PURE;

  virtual void subscribeLskpmc(const std::string lskpmc, Tracing::Span& parent_span, const StreamInfo::StreamInfo& stream_info) PURE;

  virtual void createXafi(const std::string xafi, Tracing::Span& parent_span, const StreamInfo::StreamInfo& stream_info) PURE;

  virtual void updateXafi(const std::string xafi, Tracing::Span& parent_span, const StreamInfo::StreamInfo& stream_info) PURE;

  virtual void retrieveXafi(const std::string xafi, Tracing::Span& parent_span, const StreamInfo::StreamInfo& stream_info) PURE;

  virtual void deleteXafi(const std::string xafi, Tracing::Span& parent_span, const StreamInfo::StreamInfo& stream_info) PURE;

  virtual void subscribeXafi(const std::string xafi, Tracing::Span& parent_span, const StreamInfo::StreamInfo& stream_info) PURE;
};

using ClientPtr = std::unique_ptr<Client>;

} // namespace TrafficRoutingAssistant
} // namespace SipProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
