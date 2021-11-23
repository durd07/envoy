#include "extensions/filters/network/sip_proxy/tra/tra_impl.h"

#include <chrono>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "envoy/config/core/v3/grpc_service.pb.h"
#include "envoy/stats/scope.h"

#include "common/common/assert.h"
#include "common/http/header_map_impl.h"
#include "common/http/headers.h"
#include "common/protobuf/protobuf.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace SipProxy {
namespace TrafficRoutingAssistant {

GrpcClientImpl::GrpcClientImpl(Grpc::RawAsyncClientPtr&& async_client,
                               const absl::optional<std::chrono::milliseconds>& timeout,
                               envoy::config::core::v3::ApiVersion transport_api_version)
    : async_client_(std::move(async_client)), timeout_(timeout),
      transport_api_version_(transport_api_version) {}

void GrpcClientImpl::setRequestCallbacks(RequestCallbacks& callbacks) {
  // ASSERT(callbacks_ == nullptr);
  callbacks_ = &callbacks;
}

void GrpcClientImpl::cancel() {
  ASSERT(callbacks_ != nullptr);
  request_->cancel();
  // callbacks_ = nullptr;
}

void GrpcClientImpl::closeStream() {
  ASSERT(callbacks_ != nullptr);
  stream_.closeStream();
}

void GrpcClientImpl::createTrafficRoutingAssistant(
    const std::string& type, const absl::flat_hash_map<std::string, std::string>& data,
    Tracing::Span& parent_span, const StreamInfo::StreamInfo& stream_info) {

  envoy::extensions::filters::network::sip_proxy::tra::v3::TraServiceRequest request;
  request.set_type(type);

  for (auto& item : data) {
    (*request.mutable_create_request()->mutable_data())[item.first] = item.second;
  }

  const auto& service_method =
      Grpc::VersionedMethods("envoy.extensions.filters.network.sip_proxy.tra.v3.TraService."
                             "Create",
                             "envoy.extensions.filters.network.sip_proxy.tra.v2.TraService."
                             "Create")
          .getMethodDescriptorForVersion(transport_api_version_);

  request_ =
      async_client_->send(service_method, request, *this, parent_span,
                          Http::AsyncClient::RequestOptions().setTimeout(timeout_).setParentContext(
                              Http::AsyncClient::ParentContext{&stream_info}),
                          transport_api_version_);
}

void GrpcClientImpl::updateTrafficRoutingAssistant(
    const std::string& type, const absl::flat_hash_map<std::string, std::string>& data,
    Tracing::Span& parent_span, const StreamInfo::StreamInfo& stream_info) {
  envoy::extensions::filters::network::sip_proxy::tra::v3::TraServiceRequest request;
  request.set_type(type);

  for (auto& item : data) {
    (*request.mutable_update_request()->mutable_data())[item.first] = item.second;
  }

  const auto& service_method =
      Grpc::VersionedMethods("envoy.extensions.filters.network.sip_proxy.tra.v3.TraService."
                             "Update",
                             "envoy.extensions.filters.network.sip_proxy.tra.v2.TraService."
                             "Update")
          .getMethodDescriptorForVersion(transport_api_version_);

  request_ =
      async_client_->send(service_method, request, *this, parent_span,
                          Http::AsyncClient::RequestOptions().setTimeout(timeout_).setParentContext(
                              Http::AsyncClient::ParentContext{&stream_info}),
                          transport_api_version_);
}

void GrpcClientImpl::retrieveTrafficRoutingAssistant(const std::string& type,
                                                     const std::string& key,
                                                     Tracing::Span& parent_span,
                                                     const StreamInfo::StreamInfo& stream_info) {

  envoy::extensions::filters::network::sip_proxy::tra::v3::TraServiceRequest request;
  request.set_type(type);
  request.mutable_retrieve_request()->set_key(key);

  const auto& service_method =
      Grpc::VersionedMethods("envoy.extensions.filters.network.sip_proxy.tra.v3.TraService."
                             "Retrieve",
                             "envoy.extensions.filters.network.sip_proxy.tra.v2.TraService."
                             "Retrieve")
          .getMethodDescriptorForVersion(transport_api_version_);

  request_ =
      async_client_->send(service_method, request, *this, parent_span,
                          Http::AsyncClient::RequestOptions().setTimeout(timeout_).setParentContext(
                              Http::AsyncClient::ParentContext{&stream_info}),
                          transport_api_version_);
}

void GrpcClientImpl::deleteTrafficRoutingAssistant(const std::string& type, const std::string& key,
                                                   Tracing::Span& parent_span,
                                                   const StreamInfo::StreamInfo& stream_info) {

  envoy::extensions::filters::network::sip_proxy::tra::v3::TraServiceRequest request;
  request.set_type(type);
  request.mutable_delete_request()->set_key(key);

  const auto& service_method =
      Grpc::VersionedMethods("envoy.extensions.filters.network.sip_proxy.tra.v3.TraService."
                             "Delete",
                             "envoy.extensions.filters.network.sip_proxy.tra.v2.TraService."
                             "Delete")
          .getMethodDescriptorForVersion(transport_api_version_);

  request_ =
      async_client_->send(service_method, request, *this, parent_span,
                          Http::AsyncClient::RequestOptions().setTimeout(timeout_).setParentContext(
                              Http::AsyncClient::ParentContext{&stream_info}),
                          transport_api_version_);
}

void GrpcClientImpl::subscribeTrafficRoutingAssistant(const std::string& type,
                                                      Tracing::Span& parent_span,
                                                      const StreamInfo::StreamInfo& stream_info) {
  envoy::extensions::filters::network::sip_proxy::tra::v3::TraServiceRequest request;

  UNREFERENCED_PARAMETER(parent_span);
  request.set_type(type);
  request.mutable_subscribe_request();

  const auto& service_method =
      Grpc::VersionedMethods("envoy.extensions.filters.network.sip_proxy.tra.v3.TraService."
                             "Subscribe",
                             "envoy.extensions.filters.network.sip_proxy.tra.v2.TraService."
                             "Subscribe")
          .getMethodDescriptorForVersion(transport_api_version_);
  stream_ = async_client_->start(service_method, *this,
                                 Http::AsyncClient::StreamOptions().setParentContext(
                                     Http::AsyncClient::ParentContext{&stream_info}));
  stream_.sendMessage(request, false);
}

void GrpcClientImpl::onSuccess(std::unique_ptr<envoy::extensions::filters::network::sip_proxy::tra::v3::TraServiceResponse>&& response, Tracing::Span& span) {

  UNREFERENCED_PARAMETER(span);
  if (response->has_create_response()) {
    callbacks_->complete(ResponseType::CreateResp, response->type(), response->create_response());
  } else if (response->has_update_response()) {
    callbacks_->complete(ResponseType::UpdateResp, response->type(), response->update_response());
  } else if (response->has_retrieve_response()) {
    callbacks_->complete(ResponseType::RetrieveResp, response->type(), response->retrieve_response());
  } else if (response->has_delete_response()) {
    callbacks_->complete(ResponseType::DeleteResp, response->type(), response->delete_response());
  }
  // callbacks_ = nullptr;
}

void GrpcClientImpl::onFailure(Grpc::Status::GrpcStatus status, const std::string& message, Tracing::Span&) {
  ASSERT(status != Grpc::Status::WellKnownGrpcStatus::Ok);
  ENVOY_LOG(error, "GrpcClientImpl Failure {} {}", message, status);
  // callbacks_->complete(ResponseType::FailureResp, status);
  // callbacks_ = nullptr;
}

void GrpcClientImpl::onReceiveMessage(std::unique_ptr<envoy::extensions::filters::network::sip_proxy::tra::v3::TraServiceResponse>&& message) {
  callbacks_->complete(ResponseType::SubscribeResp, message->type(), message->subscribe_response());
  // callbacks_ = nullptr;
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
