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
#include "absl/container/flat_hash_map.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace SipProxy {
namespace TrafficRoutingAssistant {

enum class ResponseType {
  CreateResp,
  UpdateResp,
  RetrieveResp,
  DeleteResp,
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

  virtual void setRequestCallbacks(RequestCallbacks& callbacks) PURE;

  virtual void cancel() PURE;

  virtual void closeStream() PURE;

  virtual void createTrafficRoutingAssistant(std::string& type,
                                             absl::flat_hash_map<std::string, std::string>& data,
                                             Tracing::Span& parent_span,
                                             StreamInfo::StreamInfo& stream_info) const PURE;
  virtual void updateTrafficRoutingAssistant(std::string& type,
                                             absl::flat_hash_map<std::string, std::string>& data,
                                             Tracing::Span& parent_span,
                                             StreamInfo::StreamInfo& stream_info) const PURE;
  virtual void retrieveTrafficRoutingAssistant(std::string& type, std::string& key,
                                               Tracing::Span& parent_span,
                                               StreamInfo::StreamInfo& stream_info) const PURE;
  virtual void deleteTrafficRoutingAssistant(std::string& type, std::string& key,
                                             Tracing::Span& parent_span,
                                             StreamInfo::StreamInfo& stream_info) const PURE;
  virtual void subscribeTrafficRoutingAssistant(std::string& type, Tracing::Span& parent_span,
                                                StreamInfo::StreamInfo& stream_info) const PURE;
};

using ClientPtr = std::unique_ptr<Client>;

} // namespace TrafficRoutingAssistant
} // namespace SipProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
