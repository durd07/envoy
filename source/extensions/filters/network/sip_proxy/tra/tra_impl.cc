#include "extensions/filters/network/sip_proxy/tra/tra_impl.h"

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

#include "envoy/config/core/v3/grpc_service.pb.h"
#include "envoy/stats/scope.h"

#include "common/common/assert.h"
#include "common/http/header_map_impl.h"
#include "common/http/headers.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace SipProxy {
namespace TrafficRoutingAssistant {

GrpcClientImpl::GrpcClientImpl(Grpc::RawAsyncClientPtr&& async_client,
                               const absl::optional<std::chrono::milliseconds>& timeout,
                               envoy::config::core::v3::ApiVersion transport_api_version)
    : async_client_(std::move(async_client)), timeout_(timeout), transport_api_version_(transport_api_version) {}

GrpcClientImpl::~GrpcClientImpl() { ASSERT(!callbacks_); }

void GrpcClientImpl::cancel() {
  ASSERT(callbacks_ != nullptr);
  request_->cancel();
  callbacks_ = nullptr;
}

void GrpcClientImpl::updateLskpmc(RequestCallbacks& callbacks, const std::string lskpmc,
                                  Tracing::Span& parent_span,
                                  const StreamInfo::StreamInfo& stream_info) {
  ASSERT(callbacks_ == nullptr);
  callbacks_ = &callbacks;

  envoy::extensions::filters::network::sip_proxy::tra::v3::TraServiceRequest request;
  request.mutable_update_lskpmc_request()->mutable_lskpmc()->set_key(lskpmc.substr(0, lskpmc.find('=')));
  request.mutable_update_lskpmc_request()->mutable_lskpmc()->set_val(lskpmc.substr(lskpmc.find('=') + 1));

  const auto & service_method = Grpc::VersionedMethods("envoy.extensions.filters.network.sip_proxy.tra.v3.TraService."
			 "UpdateLskpmc",
			 "envoy.extensions.filters.network.sip_proxy.tra.v2.TraService."
			 "UpdateLskpmc")
      .getMethodDescriptorForVersion(transport_api_version_);

  request_ = async_client_->send(service_method, request, *this, parent_span,
                          Http::AsyncClient::RequestOptions().setTimeout(timeout_).setParentContext(
                              Http::AsyncClient::ParentContext{&stream_info}),
                          transport_api_version_);
}

void GrpcClientImpl::getIpFromLskpmc(RequestCallbacks& callbacks, const std::string key,
                                     Tracing::Span& parent_span,
                                     const StreamInfo::StreamInfo& stream_info) {
  ASSERT(callbacks_ == nullptr);
  callbacks_ = &callbacks;

  envoy::extensions::filters::network::sip_proxy::tra::v3::TraServiceRequest request;
  request.mutable_get_ip_from_lskpmc_request()->set_key(key);

  const auto & service_method = Grpc::VersionedMethods("envoy.extensions.filters.network.sip_proxy.tra.v3.TraService."
			 "GetIpFromLskpmc",
			 "envoy.extensions.filters.network.sip_proxy.tra.v2.TraService."
			 "GetIpFromLskpmc")
      .getMethodDescriptorForVersion(transport_api_version_);

  request_ =
      async_client_->send(service_method, request, *this, parent_span,
                          Http::AsyncClient::RequestOptions().setTimeout(timeout_).setParentContext(
                              Http::AsyncClient::ParentContext{&stream_info}),
                          transport_api_version_);
}

void GrpcClientImpl::subscribe(RequestCallbacks& callbacks, const std::string lskpmc,
                               Tracing::Span& parent_span,
                               const StreamInfo::StreamInfo& stream_info) {
  ASSERT(callbacks_ == nullptr);
  callbacks_ = &callbacks;

  envoy::extensions::filters::network::sip_proxy::tra::v3::TraServiceRequest request;

  UNREFERENCED_PARAMETER(lskpmc);
  UNREFERENCED_PARAMETER(parent_span);
  UNREFERENCED_PARAMETER(stream_info);

  const auto & service_method = Grpc::VersionedMethods("envoy.extensions.filters.network.sip_proxy.tra.v3.TraService."
			 "Subscribe",
			 "envoy.extensions.filters.network.sip_proxy.tra.v2.TraService."
			 "Subscribe")
      .getMethodDescriptorForVersion(transport_api_version_);
      async_client_->start(service_method, *this,
                          Http::AsyncClient::StreamOptions().setTimeout(timeout_).setParentContext(
                              Http::AsyncClient::ParentContext{&stream_info}));
}

void GrpcClientImpl::onSuccess(std::unique_ptr<envoy::extensions::filters::network::sip_proxy::tra::v3::TraServiceResponse>&& response, Tracing::Span& span) {

  UNREFERENCED_PARAMETER(response);
  UNREFERENCED_PARAMETER(span);
  callbacks_->complete();
  callbacks_ = nullptr;
}

void GrpcClientImpl::onFailure(Grpc::Status::GrpcStatus status, const std::string&,
                               Tracing::Span&) {
  ASSERT(status != Grpc::Status::WellKnownGrpcStatus::Ok);
  callbacks_->complete();
  callbacks_ = nullptr;
}

void GrpcClientImpl::onReceiveMessage(std::unique_ptr<envoy::extensions::filters::network::sip_proxy::tra::v3::TraServiceResponse>&& message) {
	UNREFERENCED_PARAMETER(message);
	callbacks_->complete();
  	callbacks_ = nullptr;
}

ClientPtr traClient(Server::Configuration::FactoryContext& context,
                    const envoy::config::core::v3::GrpcService& grpc_service,
                    const std::chrono::milliseconds timeout,
                    envoy::config::core::v3::ApiVersion transport_api_version) {
  // TODO(ramaraochavali): register client to singleton when GrpcClientImpl supports concurrent
  // requests.
  const auto async_client_factory =
      context.clusterManager().grpcAsyncClientManager().factoryForGrpcService(
          grpc_service, context.scope(), true);
  return std::make_unique<GrpcClientImpl>(async_client_factory->create(), timeout,
                                          transport_api_version);
}

} // namespace TrafficRoutingAssistant
} // namespace SipProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
