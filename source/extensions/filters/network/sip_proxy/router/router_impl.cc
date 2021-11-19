#include "extensions/filters/network/sip_proxy/router/router_impl.h"

#include "envoy/extensions/filters/network/sip_proxy/v3/route.pb.h"
#include "envoy/upstream/cluster_manager.h"
#include "common/tracing/http_tracer_impl.h"

#include "common/common/logger.h"
#include "common/common/utility.h"
#include "common/network/address_impl.h"
#include "common/router/metadatamatchcriteria_impl.h"

#include "extensions/filters/network/sip_proxy/app_exception_impl.h"
#include "extensions/filters/network/sip_proxy/encoder.h"
#include "extensions/filters/network/well_known_names.h"

#include "absl/strings/match.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace SipProxy {
namespace Router {

RouteEntryImplBase::RouteEntryImplBase(
    const envoy::extensions::filters::network::sip_proxy::v3::Route& route)
    : cluster_name_(route.route().cluster()) {}

const std::string& RouteEntryImplBase::clusterName() const { return cluster_name_; }

const RouteEntry* RouteEntryImplBase::routeEntry() const { return this; }

RouteConstSharedPtr RouteEntryImplBase::clusterEntry(const MessageMetadata& metadata) const {
  UNREFERENCED_PARAMETER(metadata);
  return shared_from_this();
}

GeneralRouteEntryImpl::GeneralRouteEntryImpl(
    const envoy::extensions::filters::network::sip_proxy::v3::Route& route)
    : RouteEntryImplBase(route), domain_(route.match().domain()) {}

RouteConstSharedPtr GeneralRouteEntryImpl::matches(MessageMetadata& metadata) const {
  bool matches = metadata.domain().value() == domain_ || domain_ == "*";

  if (matches) {
    return clusterEntry(metadata);
  }

  return nullptr;
}

RouteMatcher::RouteMatcher(
    const envoy::extensions::filters::network::sip_proxy::v3::RouteConfiguration& config) {
  using envoy::extensions::filters::network::sip_proxy::v3::RouteMatch;

  for (const auto& route : config.routes()) {
    switch (route.match().match_specifier_case()) {
    case RouteMatch::MatchSpecifierCase::kDomain:
      routes_.emplace_back(new GeneralRouteEntryImpl(route));
      break;
    default:
      NOT_REACHED_GCOVR_EXCL_LINE;
    }
  }
}

RouteConstSharedPtr RouteMatcher::route(MessageMetadata& metadata) const {
  for (const auto& route : routes_) {
    RouteConstSharedPtr route_entry = route->matches(metadata);
    if (nullptr != route_entry) {
      return route_entry;
    }
  }

  return nullptr;
}

void Router::onDestroy() {
  if (!callbacks_->transactionId().empty()) {
    for (auto& kv : *transaction_infos_) {
      auto transaction_info = kv.second;
      try {
        transaction_info->getTransaction(callbacks_->transactionId());
        transaction_info->deleteTransaction(callbacks_->transactionId());
      } catch (std::out_of_range const&) {
      }
    }
  }
}

void Router::setDecoderFilterCallbacks(SipFilters::DecoderFilterCallbacks& callbacks) {
  callbacks_ = &callbacks;
  transaction_infos_ = callbacks_->transactionInfos();
  settings_ = callbacks_->settings();
}

FilterStatus Router::handleAffinity() {
  auto& metadata = metadata_;

  if (metadata->pCookieIpMap().has_value()) {
    auto [key, val] = metadata->pCookieIpMap().value();
    if ((*callbacks_->pCookieIPMap())[key] != val) {
      callbacks_->pCookieIPMap()->emplace(metadata->pCookieIpMap().value());
//      callbacks_->traClient()->updateLskpmc(metadata->pCookieIpMap().value(),
//                                            Tracing::NullSpan::instance(),
//                                            callbacks_->streamInfo());
    }
  }

  if (metadata->xafiIpMap().has_value()) {
    auto [key, val] = metadata->xafiIpMap().value();
    if ((*callbacks_->xafiIPMap())[key] != val) {
      callbacks_->xafiIPMap()->emplace(metadata->xafiIpMap().value());
//      callbacks_->traClient()->updateXafi(metadata->xafiIpMap().value(),
//                                            Tracing::NullSpan::instance(),
//                                            callbacks_->streamInfo());
//    }
  }

  const std::shared_ptr<const ProtocolOptionsConfig> options =
      cluster_->extensionProtocolOptionsTyped<ProtocolOptionsConfig>(
          NetworkFilterNames::get().SipProxy);

  if (options == nullptr || metadata->msgType() == MsgType::Response) {
    return FilterStatus::Continue;
  }

  if ((options->registrationAffinity() || options->sessionAffinity()) && metadata->destinationMap().empty()) {
    std::cout << "DDD DestinationMap: \n";
    for (const auto& aff: options->CustomizedAffinityList()) {
      for (auto [param, value] : metadata->paramMap()) {
        if (param == aff.name()) {
          std::cout << param << " = " << value << std::endl;
	  metadata->addDestination(param, value);
	}
      }
    }
    metadata->destIter = metadata->destinationMap().begin();
  }

  // No destinationMap selected
  if (metadata->destinationMap().empty()) {
    return FilterStatus::StopIteration;
  }

  return FilterStatus::Continue;
}

FilterStatus Router::transportBegin(MessageMetadataSharedPtr metadata) {
  metadata_ = metadata;

  if (upstream_request_ != nullptr) {
    return FilterStatus::Continue;
  }

  route_ = callbacks_->route();
  if (!route_) {
    ENVOY_STREAM_LOG(debug, "no route match domain {}", *callbacks_, metadata->domain().value());
    stats_.route_missing_.inc();
    callbacks_->sendLocalReply(AppException(AppExceptionType::UnknownMethod, "no route for method"),
                               true);
    return FilterStatus::StopIteration;
  }

  route_entry_ = route_->routeEntry();
  const std::string& cluster_name = route_entry_->clusterName();

  Upstream::ThreadLocalCluster* cluster = cluster_manager_.getThreadLocalCluster(cluster_name);
  thread_local_cluster_ = cluster;
  if (!cluster) {
    ENVOY_STREAM_LOG(debug, "unknown cluster '{}'", *callbacks_, cluster_name);
    stats_.unknown_cluster_.inc();
    callbacks_->sendLocalReply(AppException(AppExceptionType::InternalError,
                                            fmt::format("unknown cluster '{}'", cluster_name)),
                               true);
    return FilterStatus::StopIteration;
  }

  cluster_ = cluster->info();
  ENVOY_STREAM_LOG(debug, "cluster '{}' match domain {}", *callbacks_, cluster_name,
                   std::string(metadata->domain().value()));

  if (cluster_->maintenanceMode()) {
    stats_.upstream_rq_maintenance_mode_.inc();
    callbacks_->sendLocalReply(
        AppException(AppExceptionType::InternalError,
                     fmt::format("maintenance mode for cluster '{}'", cluster_name)),
        true);
    return FilterStatus::StopIteration;
  }

  auto ret = handleAffinity();
  if (ret == FilterStatus::StopIteration) {
    return ret;
  }

  return FilterStatus::Continue;
}

FilterStatus Router::transportEnd() { return FilterStatus::Continue; }

FilterStatus Router::messageHandlerWithLoadbalancer(std::shared_ptr<TransactionInfo> transaction_info,
		MessageMetadataSharedPtr metadata) {
    Tcp::ConnectionPool::Instance* conn_pool =
        thread_local_cluster_->tcpConnPool(Upstream::ResourcePriority::Default, this);
    if (!conn_pool) {
      stats_.no_healthy_upstream_.inc();
      callbacks_->sendLocalReply(
          AppException(AppExceptionType::InternalError,
                       fmt::format("no healthy upstream for '{}'", cluster_->name())),
          true);
      return FilterStatus::StopIteration;
    }

    Upstream::HostDescriptionConstSharedPtr host = conn_pool->host();
    if (!host) {
      return FilterStatus::StopIteration;
    }

    if (auto upstream_request =
            transaction_info->getUpstreamRequest(host->address()->ip()->addressAsString());
        upstream_request != nullptr) {
      // There is action connection, reuse it.
      upstream_request_ = upstream_request;
      upstream_request_->setDecoderFilterCallbacks(*callbacks_);
      ENVOY_STREAM_LOG(debug, "reuse upstream request for {}", *callbacks_,
                       host->address()->ip()->addressAsString());
      try {
        transaction_info->getTransaction(std::string(metadata->transactionId().value()));
      } catch (std::out_of_range const&) {
        transaction_info->insertTransaction(std::string(metadata->transactionId().value()),
                                            callbacks_, upstream_request_);
      }
    } else {
      upstream_request_ = std::make_shared<UpstreamRequest>(*conn_pool, transaction_info);
      upstream_request_->setDecoderFilterCallbacks(*callbacks_);
      transaction_info->insertUpstreamRequest(host->address()->ip()->addressAsString(),
                                              upstream_request_);
      ENVOY_STREAM_LOG(debug, "create new upstream request {}", *callbacks_,
                       host->address()->ip()->addressAsString());

      try {
        transaction_info->getTransaction(std::string(metadata->transactionId().value()));
      } catch (std::out_of_range const&) {
        transaction_info->insertTransaction(std::string(metadata->transactionId().value()),
                                            callbacks_, upstream_request_);
      }
    }
    return upstream_request_->start();
  }

FilterStatus Router::messageBegin(MessageMetadataSharedPtr metadata) {
  if (upstream_request_ != nullptr) {
    return FilterStatus::Continue;
  }

  auto& transaction_info = (*transaction_infos_)[cluster_->name()];

  while (metadata->destIter != metadata->destinationMap().end()) {
    std::string host;
    if (FilterStatus::Continue == (handle_param_map_[metadata->destIter->second](metadata->destIter->first, metadata))) {
      host = metadata->destination().value();
      metadata->destIter ++;
    } else {
      return FilterStatus::StopIteration;
    }

    if (auto upstream_request = transaction_info->getUpstreamRequest(std::string(host));
        upstream_request != nullptr) {
      // There is action connection, reuse it.
      ENVOY_STREAM_LOG(trace, "reuse upstream request from {}", *callbacks_, host);
      upstream_request_ = upstream_request;
      upstream_request_->setDecoderFilterCallbacks(*callbacks_);

      try {
        transaction_info->getTransaction(std::string(metadata->transactionId().value()));
      } catch (std::out_of_range const&) {
        transaction_info->insertTransaction(std::string(metadata->transactionId().value()),
                                            callbacks_, upstream_request_);
      }
      return upstream_request_->start();
    }
    ENVOY_STREAM_LOG(trace, "no destination preset select with load balancer.", *callbacks_);

    if (FilterStatus::Continue == messageHandlerWithLoadbalancer(transaction_info, metadata)) {
      return FilterStatus::Continue;
    }
  }

  ENVOY_STREAM_LOG(debug, "no destination.", *callbacks_);
  messageHandlerWithLoadbalancer(transaction_info, metadata);

  return FilterStatus::Continue;
}

FilterStatus Router::messageEnd() {
  Buffer::OwnedImpl transport_buffer;

  // set EP/Opaque, used in upstream
  metadata_->setEP(upstream_request_->localAddress());

  std::shared_ptr<Encoder> encoder = std::make_shared<EncoderImpl>();
  encoder->encode(metadata_, transport_buffer);

  ENVOY_STREAM_LOG(info, "send buffer : {} bytes\n{}", *callbacks_, transport_buffer.length(),
                   transport_buffer.toString());

  upstream_request_->write(transport_buffer, false);
  return FilterStatus::Continue;
}

const Network::Connection* Router::downstreamConnection() const {
  if (callbacks_ != nullptr) {
    return callbacks_->connection();
  }

  return nullptr;
}

// Not used
// void Router::cleanup() { /*upstream_request_.reset();*/
//}

UpstreamRequest::UpstreamRequest(Tcp::ConnectionPool::Instance& pool,
                                 std::shared_ptr<TransactionInfo> transaction_info)
    : conn_pool_(pool), transaction_info_(transaction_info), /*request_complete_(false),*/
      /*response_started_(false),*/ response_complete_(false) {}

UpstreamRequest::~UpstreamRequest() {
  if (conn_pool_handle_) {
    conn_pool_handle_->cancel(Tcp::ConnectionPool::CancelPolicy::Default);
  }
}

FilterStatus UpstreamRequest::start() {
  if (conn_state_ == ConnectionState::Connecting) {
    return FilterStatus::StopIteration;
  } else if (conn_state_ == ConnectionState::Connected) {
    return FilterStatus::Continue;
  }

  ENVOY_LOG(trace, "start connecting {}", conn_pool_.host()->address()->asString());
  conn_state_ = ConnectionState::Connecting;

  Tcp::ConnectionPool::Cancellable* handle = conn_pool_.newConnection(*this);
  if (handle) {
    // Pause while we wait for a connection.
    conn_pool_handle_ = handle;
    return FilterStatus::StopIteration;
  }

  if (upstream_host_ == nullptr) {
    return FilterStatus::StopIteration;
  }

  return FilterStatus::Continue;
}

void UpstreamRequest::releaseConnection(const bool close) {
  if (conn_pool_handle_) {
    conn_pool_handle_->cancel(Tcp::ConnectionPool::CancelPolicy::Default);
    conn_pool_handle_ = nullptr;
  }

  conn_state_ = ConnectionState::NotConnected;

  // The event triggered by close will also release this connection so clear conn_data_ before
  // closing.
  auto conn_data = std::move(conn_data_);
  if (close && conn_data != nullptr) {
    conn_data->connection().close(Network::ConnectionCloseType::NoFlush);
  }
}

void UpstreamRequest::resetStream() { releaseConnection(true); }

void UpstreamRequest::onPoolFailure(ConnectionPool::PoolFailureReason reason,
                                    Upstream::HostDescriptionConstSharedPtr host) {
  ENVOY_LOG(info, "on pool failure");
  conn_state_ = ConnectionState::NotConnected;
  conn_pool_handle_ = nullptr;

  // Mimic an upstream reset.
  onUpstreamHostSelected(host);
  UNREFERENCED_PARAMETER(reason);
}

void UpstreamRequest::onPoolReady(Tcp::ConnectionPool::ConnectionDataPtr&& conn_data,
                                  Upstream::HostDescriptionConstSharedPtr host) {
  ENVOY_STREAM_LOG(trace, "onPoolReady", *callbacks_);
  bool continue_handling = conn_pool_handle_ != nullptr;

  conn_data_ = std::move(conn_data);

  onUpstreamHostSelected(host);
  conn_data_->addUpstreamCallbacks(*this);
  conn_pool_handle_ = nullptr;

  conn_state_ = ConnectionState::Connected;

  onRequestStart(continue_handling);
}

void UpstreamRequest::onRequestStart(bool continue_handling) {
  if (continue_handling) {
    callbacks_->continueHanding();
  }
}

void UpstreamRequest::onUpstreamHostSelected(Upstream::HostDescriptionConstSharedPtr host) {
  upstream_host_ = host;
}

void UpstreamRequest::onResetStream(ConnectionPool::PoolFailureReason reason) {
  switch (reason) {
  case ConnectionPool::PoolFailureReason::Overflow:
    callbacks_->sendLocalReply(
        AppException(AppExceptionType::InternalError, "sip upstream request: too many connections"),
        true);
    break;
  case ConnectionPool::PoolFailureReason::LocalConnectionFailure:
    // Should only happen if we closed the connection, due to an error condition, in which case
    // we've already handled any possible downstream response.
    callbacks_->resetDownstreamConnection();
    break;
  case ConnectionPool::PoolFailureReason::RemoteConnectionFailure:
  case ConnectionPool::PoolFailureReason::Timeout:
    // TODO(zuercher): distinguish between these cases where appropriate (particularly timeout)
    // if (!response_started_) {
    //  callbacks_->sendLocalReply(
    //      AppException(
    //          AppExceptionType::InternalError,
    //          fmt::format("connection failure '{}'", (upstream_host_ != nullptr)
    //                                                     ? upstream_host_->address()->asString()
    //                                                     : "to upstream")),
    //      true);
    //  return;
    //}

    // Error occurred after a partial response, propagate the reset to the downstream.
    callbacks_->resetDownstreamConnection();
    break;
  default:
    NOT_REACHED_GCOVR_EXCL_LINE;
  }
}

SipFilters::DecoderFilterCallbacks* UpstreamRequest::getTransaction(std::string&& transaction_id) {
  try {
    return transaction_info_->getTransaction(std::move(transaction_id)).activeTrans();
  } catch (std::out_of_range const&) {
    return nullptr;
  }
}

// Tcp::ConnectionPool::UpstreamCallbacks
void UpstreamRequest::onUpstreamData(Buffer::Instance& data, bool end_stream) {
  UNREFERENCED_PARAMETER(end_stream);
  upstream_buffer_.move(data);
  auto response_decoder_ = std::make_unique<ResponseDecoder>(*this);
  response_decoder_->onData(upstream_buffer_);
}

void UpstreamRequest::onEvent(Network::ConnectionEvent event) {
  ENVOY_LOG(info, "received upstream event {}", event);
  switch (event) {
  case Network::ConnectionEvent::RemoteClose:
    ENVOY_LOG(debug, "upstream remote close");
    break;
  case Network::ConnectionEvent::LocalClose:
    ENVOY_LOG(debug, "upstream local close");
    break;
  default:
    ENVOY_LOG(debug, "connected is consumed by the connection pool");
    // Connected is consumed by the connection pool.
    return;
  }

  releaseConnection(false);
  transaction_info_->deleteUpstreamRequest(conn_pool_.host()->address()->ip()->addressAsString());
}

void UpstreamRequest::setDecoderFilterCallbacks(SipFilters::DecoderFilterCallbacks& callbacks) {
  callbacks_ = &callbacks;
}

bool ResponseDecoder::onData(Buffer::Instance& data) {
  decoder_->onData(data);
  return true;
}

FilterStatus ResponseDecoder::transportBegin(MessageMetadataSharedPtr metadata) {
  if (metadata->transactionId().has_value()) {
    auto transaction_id = metadata->transactionId().value();

    auto active_trans = parent_.getTransaction(std::string(transaction_id));
    if (active_trans) {
      // p_cookie_ip_map_ = active_trans->pCookieIPMap();

      if (metadata->pCookieIpMap().has_value()) {
        ENVOY_LOG(trace, "update p-cookie-ip-map {}={}", metadata->pCookieIpMap().value().first,
                  metadata->pCookieIpMap().value().second);
        auto [key, val] = metadata->pCookieIpMap().value();
        if ((*active_trans->pCookieIPMap())[key] != val) {
          active_trans->pCookieIPMap()->emplace(metadata->pCookieIpMap().value());
//          active_trans->traClient()->updateLskpmc(metadata->pCookieIpMap().value(),
//                                                  Tracing::NullSpan::instance(),
//                                                  active_trans->streamInfo());
//        }
      }

      if (metadata->xafiIpMap().has_value()) {
        ENVOY_LOG(trace, "update xafi-ip-map {}={}", metadata->xafiIpMap().value().first,
                  metadata->xafiIpMap().value().second);
        auto [key, val] = metadata->xafiIpMap().value();
        if ((*active_trans->xafiIPMap())[key] != val) {
          active_trans->xafiIPMap()->emplace(metadata->xafiIpMap().value());
//          active_trans->traClient()->updateXafi(metadata->xafiIpMap().value(),
//                                                  Tracing::NullSpan::instance(),
//                                                  active_trans->streamInfo());
//        }
      }

      active_trans->startUpstreamResponse();
      active_trans->upstreamData(metadata);
    } else {
      ENVOY_LOG(debug, "no active trans selected {}\n{}", transaction_id, metadata->rawMsg());
      return FilterStatus::StopIteration;
    }
  } else {
    ENVOY_LOG(debug, "no active trans selected \n{}", metadata->rawMsg());
    return FilterStatus::StopIteration;
  }

  return FilterStatus::Continue;
}

absl::string_view ResponseDecoder::getLocalIp() { return parent_.localAddress(); }

std::string ResponseDecoder::getOwnDomain() { return parent_.transactionInfo()->getOwnDomain(); }

std::string ResponseDecoder::getDomainMatchParamName() {
  return parent_.transactionInfo()->getDomainMatchParamName();
}

} // namespace Router
} // namespace SipProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
