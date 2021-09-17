#pragma once

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

#include "common/common/macros.h"
#include "envoy/config/core/v3/grpc_service.pb.h"
#include "envoy/grpc/async_client.h"
#include "envoy/grpc/async_client_manager.h"
#include "envoy/server/filter_config.h"
#include "envoy/stats/scope.h"
#include "envoy/tracing/http_tracer.h"
#include "envoy/upstream/cluster_manager.h"

#include "common/common/logger.h"
#include "common/grpc/typed_async_client.h"
#include "common/singleton/const_singleton.h"

#include "extensions/filters/network/sip_proxy/tra/tra.h"
#include "envoy/extensions/filters/network/sip_proxy/tra/v3/tra.pb.h"
#include "envoy/extensions/filters/network/sip_proxy/tra/v3/tra.pb.validate.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace SipProxy {
namespace TrafficRoutingAssistant {

using TrafficRoutingAssistantAsyncRequestCallbacks = Grpc::AsyncRequestCallbacks<envoy::extensions::filters::network::sip_proxy::tra::v3::TraServiceResponse>;
using TrafficRoutingAssistantAsyncStreamCallbacks = Grpc::AsyncStreamCallbacks<envoy::extensions::filters::network::sip_proxy::tra::v3::TraServiceResponse>;

// TODO(htuch): We should have only one client per thread, but today we create one per filter stack.
// This will require support for more than one outstanding request per client (limit() assumes only
// one today).
class GrpcClientImpl : public Client,
                       public TrafficRoutingAssistantAsyncRequestCallbacks,
                       public TrafficRoutingAssistantAsyncStreamCallbacks,
                       public Logger::Loggable<Logger::Id::sip> {
public:
  GrpcClientImpl(Grpc::RawAsyncClientPtr&& async_client,
                 const absl::optional<std::chrono::milliseconds>& timeout,
                 envoy::config::core::v3::ApiVersion transport_api_version);
  ~GrpcClientImpl() override = default;

  // Extensions::NetworkFilters::SipProxy::TrafficRoutingAssistant::Client
  void setRequestCallbacks(RequestCallbacks& callbacks) override;
  void cancel() override;

  void closeStream() override;

  void createLskpmc(const std::string lskpmc, Tracing::Span& parent_span, const StreamInfo::StreamInfo& stream_info) override;

  void updateLskpmc(const std::string lskpmc, Tracing::Span& parent_span, const StreamInfo::StreamInfo& stream_info) override;

  void retrieveLskpmc(const std::string lskpmc, Tracing::Span& parent_span, const StreamInfo::StreamInfo& stream_info) override;

  void deleteLskpmc(const std::string lskpmc, Tracing::Span& parent_span, const StreamInfo::StreamInfo& stream_info) override;

  void subscribeLskpmc(const std::string lskpmc, Tracing::Span& parent_span, const StreamInfo::StreamInfo& stream_info) override;

  // Grpc::AsyncRequestCallbacks
  void onCreateInitialMetadata(Http::RequestHeaderMap&) override {}
  void onSuccess(std::unique_ptr<envoy::extensions::filters::network::sip_proxy::tra::v3::TraServiceResponse>&& response, Tracing::Span& span) override;
  void onFailure(Grpc::Status::GrpcStatus status, const std::string& message, Tracing::Span& span) override;

  // Grpc::AsyncStreamCallbacks
  void onReceiveMessage(std::unique_ptr<envoy::extensions::filters::network::sip_proxy::tra::v3::TraServiceResponse>&& message) override;
  void onReceiveInitialMetadata(Http::ResponseHeaderMapPtr&& metadata) override {
	  UNREFERENCED_PARAMETER(metadata);
  }
  void onReceiveTrailingMetadata(Http::ResponseTrailerMapPtr&& metadata) override {
	  UNREFERENCED_PARAMETER(metadata);
  };
  void onRemoteClose(Grpc::Status::GrpcStatus status, const std::string& message) override {
	  UNREFERENCED_PARAMETER(status);
	  UNREFERENCED_PARAMETER(message);
  };

private:
  RequestCallbacks* callbacks_{};
  Grpc::AsyncClient<envoy::extensions::filters::network::sip_proxy::tra::v3::TraServiceRequest, envoy::extensions::filters::network::sip_proxy::tra::v3::TraServiceResponse> async_client_;
  Grpc::AsyncRequest* request_{};
  Grpc::AsyncStream<envoy::extensions::filters::network::sip_proxy::tra::v3::TraServiceRequest> stream_{};
  absl::optional<std::chrono::milliseconds> timeout_;
  const envoy::config::core::v3::ApiVersion transport_api_version_;
};

/**
 * Builds the tra client.
 */
ClientPtr traClient(Server::Configuration::FactoryContext& context,
                    const envoy::config::core::v3::GrpcService& grpc_service,
                    const std::chrono::milliseconds timeout,
                    envoy::config::core::v3::ApiVersion transport_api_version);

} // namespace TrafficRoutingAssistant
} // namespace SipProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
