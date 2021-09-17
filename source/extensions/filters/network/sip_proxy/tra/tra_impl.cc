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

void GrpcClientImpl::setRequestCallbacks(RequestCallbacks& callbacks)
{
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

void GrpcClientImpl::createLskpmc(const std::string lskpmc, Tracing::Span& parent_span,
                                  const StreamInfo::StreamInfo& stream_info) {

  envoy::extensions::filters::network::sip_proxy::tra::v3::TraServiceRequest request;
  //request.mutable_create_lskpmc_request()->mutable_lskpmcs()->insert(lskpmc.substr(0, lskpmc.find('=')),  lskpmc.substr(lskpmc.find('=') + 1));
  auto key = lskpmc.substr(0, lskpmc.find('='));
  auto val = lskpmc.substr(lskpmc.find('=') + 1);
  (*request.mutable_create_lskpmc_request()->mutable_lskpmcs())[key] = val;

  const auto& service_method =
      Grpc::VersionedMethods("envoy.extensions.filters.network.sip_proxy.tra.v3.TraService."
                             "CreateLskpmc",
                             "envoy.extensions.filters.network.sip_proxy.tra.v2.TraService."
                             "CreateLskpmc")
          .getMethodDescriptorForVersion(transport_api_version_);

  request_ =
      async_client_->send(service_method, request, *this, parent_span,
                          Http::AsyncClient::RequestOptions().setTimeout(timeout_).setParentContext(
                              Http::AsyncClient::ParentContext{&stream_info}),
                          transport_api_version_);
}

void GrpcClientImpl::updateLskpmc(const std::pair<std::string &&, std::string &&> && lskpmc, Tracing::Span& parent_span,
                                  const StreamInfo::StreamInfo& stream_info) {

  envoy::extensions::filters::network::sip_proxy::tra::v3::TraServiceRequest request;
  //request.mutable_update_lskpmc_request()->mutable_lskpmcs()->insert(lskpmc.substr(0, lskpmc.find('=')),  lskpmc.substr(lskpmc.find('=') + 1));
  auto key = lskpmc.first;
  auto val = lskpmc.second;
  (*request.mutable_create_lskpmc_request()->mutable_lskpmcs())[key] = val;

  const auto& service_method =
      Grpc::VersionedMethods("envoy.extensions.filters.network.sip_proxy.tra.v3.TraService."
                             "UpdateLskpmc",
                             "envoy.extensions.filters.network.sip_proxy.tra.v2.TraService."
                             "UpdateLskpmc")
          .getMethodDescriptorForVersion(transport_api_version_);

  request_ =
      async_client_->send(service_method, request, *this, parent_span,
                          Http::AsyncClient::RequestOptions().setTimeout(timeout_).setParentContext(
                              Http::AsyncClient::ParentContext{&stream_info}),
                          transport_api_version_);
}

void GrpcClientImpl::retrieveLskpmc(const std::string lskpmc, Tracing::Span& parent_span,
                                     const StreamInfo::StreamInfo& stream_info) {

  envoy::extensions::filters::network::sip_proxy::tra::v3::TraServiceRequest request;
  request.mutable_retrieve_lskpmc_request()->set_lskpmc(lskpmc);

  const auto& service_method =
      Grpc::VersionedMethods("envoy.extensions.filters.network.sip_proxy.tra.v3.TraService."
                             "RetrieveLskpmc",
                             "envoy.extensions.filters.network.sip_proxy.tra.v2.TraService."
                             "RetrieveLskpmc")
          .getMethodDescriptorForVersion(transport_api_version_);

  request_ =
      async_client_->send(service_method, request, *this, parent_span,
                          Http::AsyncClient::RequestOptions().setTimeout(timeout_).setParentContext(
                              Http::AsyncClient::ParentContext{&stream_info}),
                          transport_api_version_);
}

void GrpcClientImpl::deleteLskpmc(const std::string lskpmc, Tracing::Span& parent_span,
                                     const StreamInfo::StreamInfo& stream_info) {

  envoy::extensions::filters::network::sip_proxy::tra::v3::TraServiceRequest request;
  request.mutable_delete_lskpmc_request()->set_lskpmc(lskpmc);

  const auto& service_method =
      Grpc::VersionedMethods("envoy.extensions.filters.network.sip_proxy.tra.v3.TraService."
                             "DeleteLskpmc",
                             "envoy.extensions.filters.network.sip_proxy.tra.v2.TraService."
                             "DeleteLskpmc")
          .getMethodDescriptorForVersion(transport_api_version_);

  request_ =
      async_client_->send(service_method, request, *this, parent_span,
                          Http::AsyncClient::RequestOptions().setTimeout(timeout_).setParentContext(
                              Http::AsyncClient::ParentContext{&stream_info}),
                          transport_api_version_);
}

void GrpcClientImpl::subscribeLskpmc(const std::string lskpmc, Tracing::Span& parent_span,
                               const StreamInfo::StreamInfo& stream_info) {
  envoy::extensions::filters::network::sip_proxy::tra::v3::TraServiceRequest request;

  UNREFERENCED_PARAMETER(lskpmc);
  UNREFERENCED_PARAMETER(parent_span);
  request.mutable_subscribe_lskpmc_request();

  const auto& service_method =
      Grpc::VersionedMethods("envoy.extensions.filters.network.sip_proxy.tra.v3.TraService."
                             "SubscribeLskpmc",
                             "envoy.extensions.filters.network.sip_proxy.tra.v2.TraService."
                             "SubscribeLskpmc")
          .getMethodDescriptorForVersion(transport_api_version_);
  stream_ = async_client_->start(service_method, *this,
                       //Http::AsyncClient::StreamOptions().setTimeout(timeout_).setParentContext(
                       Http::AsyncClient::StreamOptions().setParentContext(
                           Http::AsyncClient::ParentContext{&stream_info}));
  stream_.sendMessage(request, false);
}

void GrpcClientImpl::createXafi(const std::string xafi, Tracing::Span& parent_span,
                                  const StreamInfo::StreamInfo& stream_info) {

  envoy::extensions::filters::network::sip_proxy::tra::v3::TraServiceRequest request;
  //request.mutable_create_xafi_request()->mutable_xafis()->insert(xafi.substr(0, xafi.find('=')),  xafi.substr(xafi.find('=') + 1));
  auto key = xafi.substr(0, xafi.find('='));
  auto val = xafi.substr(xafi.find('=') + 1);
  (*request.mutable_create_xafi_request()->mutable_xafis())[key] = val;

  const auto& service_method =
      Grpc::VersionedMethods("envoy.extensions.filters.network.sip_proxy.tra.v3.TraService."
                             "CreateXafi",
                             "envoy.extensions.filters.network.sip_proxy.tra.v2.TraService."
                             "CreateXafi")
          .getMethodDescriptorForVersion(transport_api_version_);

  request_ =
      async_client_->send(service_method, request, *this, parent_span,
                          Http::AsyncClient::RequestOptions().setTimeout(timeout_).setParentContext(
                              Http::AsyncClient::ParentContext{&stream_info}),
                          transport_api_version_);
}

void GrpcClientImpl::updateXafi(const std::string xafi, Tracing::Span& parent_span,
                                  const StreamInfo::StreamInfo& stream_info) {

  envoy::extensions::filters::network::sip_proxy::tra::v3::TraServiceRequest request;
  //request.mutable_update_xafi_request()->mutable_xafis()->insert(xafi.substr(0, xafi.find('=')),  xafi.substr(xafi.find('=') + 1));
  auto key = xafi.substr(0, xafi.find('='));
  auto val = xafi.substr(xafi.find('=') + 1);
  (*request.mutable_create_xafi_request()->mutable_xafis())[key] = val;

  const auto& service_method =
      Grpc::VersionedMethods("envoy.extensions.filters.network.sip_proxy.tra.v3.TraService."
                             "UpdateXafi",
                             "envoy.extensions.filters.network.sip_proxy.tra.v2.TraService."
                             "UpdateXafi")
          .getMethodDescriptorForVersion(transport_api_version_);

  request_ =
      async_client_->send(service_method, request, *this, parent_span,
                          Http::AsyncClient::RequestOptions().setTimeout(timeout_).setParentContext(
                              Http::AsyncClient::ParentContext{&stream_info}),
                          transport_api_version_);
}

void GrpcClientImpl::retrieveXafi(const std::string xafi, Tracing::Span& parent_span,
                                     const StreamInfo::StreamInfo& stream_info) {

  envoy::extensions::filters::network::sip_proxy::tra::v3::TraServiceRequest request;
  request.mutable_retrieve_xafi_request()->set_xafi(xafi);

  const auto& service_method =
      Grpc::VersionedMethods("envoy.extensions.filters.network.sip_proxy.tra.v3.TraService."
                             "RetrieveXafi",
                             "envoy.extensions.filters.network.sip_proxy.tra.v2.TraService."
                             "RetrieveXafi")
          .getMethodDescriptorForVersion(transport_api_version_);

  request_ =
      async_client_->send(service_method, request, *this, parent_span,
                          Http::AsyncClient::RequestOptions().setTimeout(timeout_).setParentContext(
                              Http::AsyncClient::ParentContext{&stream_info}),
                          transport_api_version_);
}

void GrpcClientImpl::deleteXafi(const std::string xafi, Tracing::Span& parent_span,
                                     const StreamInfo::StreamInfo& stream_info) {

  envoy::extensions::filters::network::sip_proxy::tra::v3::TraServiceRequest request;
  request.mutable_delete_xafi_request()->set_xafi(xafi);

  const auto& service_method =
      Grpc::VersionedMethods("envoy.extensions.filters.network.sip_proxy.tra.v3.TraService."
                             "DeleteXafi",
                             "envoy.extensions.filters.network.sip_proxy.tra.v2.TraService."
                             "DeleteXafi")
          .getMethodDescriptorForVersion(transport_api_version_);

  request_ =
      async_client_->send(service_method, request, *this, parent_span,
                          Http::AsyncClient::RequestOptions().setTimeout(timeout_).setParentContext(
                              Http::AsyncClient::ParentContext{&stream_info}),
                          transport_api_version_);
}

void GrpcClientImpl::subscribeXafi(const std::string xafi, Tracing::Span& parent_span,
                               const StreamInfo::StreamInfo& stream_info) {
  envoy::extensions::filters::network::sip_proxy::tra::v3::TraServiceRequest request;

  UNREFERENCED_PARAMETER(xafi);
  UNREFERENCED_PARAMETER(parent_span);
  request.mutable_subscribe_xafi_request();

  const auto& service_method =
      Grpc::VersionedMethods("envoy.extensions.filters.network.sip_proxy.tra.v3.TraService."
                             "SubscribeXafi",
                             "envoy.extensions.filters.network.sip_proxy.tra.v2.TraService."
                             "SubscribeXafi")
          .getMethodDescriptorForVersion(transport_api_version_);
  stream_ = async_client_->start(service_method, *this,
                       //Http::AsyncClient::StreamOptions().setTimeout(timeout_).setParentContext(
                       Http::AsyncClient::StreamOptions().setParentContext(
                           Http::AsyncClient::ParentContext{&stream_info}));
  stream_.sendMessage(request, false);
}

void GrpcClientImpl::onSuccess(
    std::unique_ptr<envoy::extensions::filters::network::sip_proxy::tra::v3::TraServiceResponse>&&
        response,
    Tracing::Span& span) {

  UNREFERENCED_PARAMETER(span);
  if (response->has_create_lskpmc_response()) {
    callbacks_->complete(ResponseType::CreateLskpmcResp, response->create_lskpmc_response());
  } else if (response->has_update_lskpmc_response()) {
    callbacks_->complete(ResponseType::UpdateLskpmcResp, response->update_lskpmc_response());
  } else if (response->has_retrieve_lskpmc_response()) {
    callbacks_->complete(ResponseType::RetrieveLskpmcResp, response->retrieve_lskpmc_response());
  } else if (response->has_delete_lskpmc_response()) {
    callbacks_->complete(ResponseType::DeleteLskpmcResp, response->delete_lskpmc_response());
  } else if (response->has_subscribe_lskpmc_response()) {
    callbacks_->complete(ResponseType::SubscribeLskpmcResp, response->subscribe_lskpmc_response());
  } else if (response->has_create_xafi_response()) {
    callbacks_->complete(ResponseType::CreateXafiResp, response->create_xafi_response());
  } else if (response->has_update_xafi_response()) {
    callbacks_->complete(ResponseType::UpdateXafiResp, response->update_xafi_response());
  } else if (response->has_retrieve_xafi_response()) {
    callbacks_->complete(ResponseType::RetrieveXafiResp, response->retrieve_xafi_response());
  } else if (response->has_delete_xafi_response()) {
    callbacks_->complete(ResponseType::DeleteXafiResp, response->delete_xafi_response());
  } else if (response->has_subscribe_xafi_response()) {
    callbacks_->complete(ResponseType::SubscribeXafiResp, response->subscribe_xafi_response());
  }
  // callbacks_ = nullptr;
}

void GrpcClientImpl::onFailure(Grpc::Status::GrpcStatus status, const std::string&,
                               Tracing::Span&) {
  ASSERT(status != Grpc::Status::WellKnownGrpcStatus::Ok);
  callbacks_->complete(ResponseType::FailureResp, status);
  // callbacks_ = nullptr;
}

void GrpcClientImpl::onReceiveMessage(
    std::unique_ptr<envoy::extensions::filters::network::sip_proxy::tra::v3::TraServiceResponse>&&
        message) {
  callbacks_->complete(ResponseType::SubscribeLskpmcResp, message->subscribe_lskpmc_response());
  callbacks_->complete(ResponseType::SubscribeXafiResp, message->subscribe_xafi_response());
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
