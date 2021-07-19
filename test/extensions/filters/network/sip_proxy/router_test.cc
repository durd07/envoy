#include <memory>

#include "envoy/tcp/conn_pool.h"

#include "source/common/buffer/buffer_impl.h"

#include "source/extensions/filters/network/sip_proxy/app_exception_impl.h"
#include "source/extensions/filters/network/sip_proxy/config.h"
#include "source/extensions/filters/network/sip_proxy/router/config.h"
#include "source/extensions/filters/network/sip_proxy/router/router_impl.h"
#include "source/extensions/filters/network/sip_proxy/sip.h"

#include "test/extensions/filters/network/sip_proxy/mocks.h"
#include "test/extensions/filters/network/sip_proxy/utility.h"
#include "test/mocks/network/mocks.h"
#include "test/mocks/server/factory_context.h"
#include "test/mocks/upstream/host.h"
#include "test/test_common/printers.h"
#include "test/test_common/registry.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

using testing::_;
using testing::AtLeast;
using testing::Combine;
using testing::ContainsRegex;
using testing::Eq;
using testing::Invoke;
using testing::NiceMock;
using testing::Ref;
using testing::Return;
using testing::ReturnRef;
using ::testing::TestParamInfo;
using testing::Values;

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace SipProxy {
namespace Router {

class SipRouterTestBase {
public:
  SipRouterTestBase() {}
  void initializeTrans(bool hasOption = true) {
    if (hasOption == true) {
      const std::string yaml = R"EOF(
session_affinity: true
registration_affinity: true
)EOF";

      envoy::extensions::filters::network::sip_proxy::v3::SipProtocolOptions config;
      TestUtility::loadFromYaml(yaml, config);

      const auto options = std::make_shared<ProtocolOptionsConfigImpl>(config);
      EXPECT_CALL(*context_.cluster_manager_.thread_local_cluster_.cluster_.info_,
                  extensionProtocolOptions(_))
          .WillRepeatedly(Return(options));
    }

    transaction_infos_ = std::make_shared<TransactionInfos>();
    context_.cluster_manager_.initializeThreadLocalClusters({"cluster"});
  }

  void initializeRouter() {
    route_ = new NiceMock<MockRoute>();
    route_ptr_.reset(route_);

    router_ = std::make_unique<Router>(context_.clusterManager(), "test", context_.scope());

    EXPECT_EQ(nullptr, router_->downstreamConnection());

    EXPECT_CALL(callbacks_, transactionInfos()).WillOnce(Return(transaction_infos_));
    router_->setDecoderFilterCallbacks(callbacks_);
  }

  void initializeRouter_callback() {
    route_ = new NiceMock<MockRoute>();
    route_ptr_.reset(route_);

    router_ = std::make_unique<Router>(context_.clusterManager(), "test", context_.scope());

    EXPECT_CALL(callbacks_, transactionInfos()).WillOnce(Return(transaction_infos_));
    router_->setDecoderFilterCallbacks(callbacks_);

    EXPECT_EQ(nullptr, router_->downstreamConnection());
  }
  /*
    //void initializeFilter(const std::string& yaml) {
    void initializeFilter() {
    const std::string yaml = R"EOF(
  stat_prefix: egress
  route_config:
    name: local_route
    routes:
    - match:
        domain: "test"
      route:
        cluster: "test"
  settings:
    transaction_timeout: 32s
  )EOF";

      // Destroy any existing filter first.
      filter_ = nullptr;

      for (const auto& counter : store_.counters()) {
        counter->reset();
      }

      if (yaml.empty()) {
        proto_config_.set_stat_prefix("test");
      } else {
        TestUtility::loadFromYaml(yaml, proto_config_);
        TestUtility::validate(proto_config_);
      }

      proto_config_.set_stat_prefix("test");

      decoder_filter_ = std::make_shared<NiceMock<SipFilters::MockDecoderFilter>>();

      config_ = std::make_unique<TestConfigImpl>(proto_config_, context_, decoder_filter_, stats_);
      if (custom_filter_) {
        config_->custom_filter_ = custom_filter_;
      }

      ON_CALL(random_, random()).WillByDefault(Return(42));
      filter_ = std::make_unique<ConnectionManager>(
          *config_, random_, filter_callbacks_.connection_.dispatcher_.timeSource(),
          transaction_infos_);
      filter_->initializeReadFilterCallbacks(filter_callbacks_);
      filter_->onNewConnection();

      // NOP currently.
      filter_->onAboveWriteBufferHighWatermark();
      filter_->onBelowWriteBufferLowWatermark();
    }
  */

  void initializeMetadata(MsgType msg_type, MethodType method = MethodType::Invite,
                          bool set_destination = true) {

    metadata_ = std::make_shared<MessageMetadata>();
    metadata_->setMethodType(method);
    metadata_->setMsgType(msg_type);
    metadata_->setTransactionId("test");
    if (set_destination) {
      metadata_->setDestination("127.0.0.1");
    }
  }

  void initializeTransaction() {
    auto transaction_info_ptr = std::make_shared<TransactionInfo>(
        "test", thread_local_, static_cast<std::chrono::milliseconds>(4));
    transaction_info_ptr->init();
    transaction_infos_->emplace(cluster_name_, transaction_info_ptr);
  }

  void startRequest(MsgType msg_type, MethodType method = MethodType::Invite) {
    // const bool strip_service_name = false)
    initializeMetadata(msg_type, method);
    EXPECT_EQ(FilterStatus::Continue, router_->transportBegin(metadata_));

    EXPECT_CALL(callbacks_, route()).WillOnce(Return(route_ptr_));
    EXPECT_CALL(*route_, routeEntry()).WillOnce(Return(&route_entry_));
    EXPECT_CALL(route_entry_, clusterName()).WillRepeatedly(ReturnRef(cluster_name_));

    EXPECT_EQ(FilterStatus::Continue, router_->messageBegin(metadata_));
  }

  void connectUpstream() {
    EXPECT_CALL(*context_.cluster_manager_.thread_local_cluster_.tcp_conn_pool_.connection_data_,
                addUpstreamCallbacks(_))
        .WillOnce(Invoke([&](Tcp::ConnectionPool::UpstreamCallbacks& cb) -> void {
          upstream_callbacks_ = &cb;
        }));

    conn_state_.reset();
    EXPECT_CALL(*context_.cluster_manager_.thread_local_cluster_.tcp_conn_pool_.connection_data_,
                connectionState())
        .WillRepeatedly(
            Invoke([&]() -> Tcp::ConnectionPool::ConnectionState* { return conn_state_.get(); }));

    context_.cluster_manager_.thread_local_cluster_.tcp_conn_pool_.poolReady(upstream_connection_);

    EXPECT_NE(nullptr, upstream_callbacks_);
  }

  void startRequestWithExistingConnection(MsgType msg_type,
                                          MethodType method = MethodType::Invite) {
    initializeMetadata(msg_type, method);
    EXPECT_EQ(FilterStatus::Continue, router_->transportBegin({}));

    EXPECT_CALL(route_entry_, clusterName()).WillRepeatedly(ReturnRef(cluster_name_));

    EXPECT_EQ(FilterStatus::Continue, router_->messageBegin(metadata_));
    EXPECT_NE(nullptr, upstream_callbacks_);
  }

  void completeRequest() {
    EXPECT_EQ(FilterStatus::Continue, router_->messageEnd());
    EXPECT_EQ(FilterStatus::Continue, router_->transportEnd());
  }

  void returnResponse(MsgType msg_type = MsgType::Response, bool is_success = true) {
    Buffer::OwnedImpl buffer;

    initializeMetadata(msg_type, MethodType::Ok200, false);

    ON_CALL(callbacks_, responseSuccess()).WillByDefault(Return(is_success));

    upstream_callbacks_->onUpstreamData(buffer, false);
  }

  void destroyRouter() {
    router_->onDestroy();
    router_.reset();
  }

  NiceMock<Server::Configuration::MockFactoryContext> context_;
  NiceMock<Network::MockClientConnection> connection_;
  NiceMock<Event::MockDispatcher> dispatcher_;
  NiceMock<MockTimeSystem> time_source_;
  NiceMock<SipFilters::MockDecoderFilterCallbacks> callbacks_;
  NiceMock<MockRoute>* route_{};
  NiceMock<MockRouteEntry> route_entry_;
  NiceMock<Upstream::MockHostDescription>* host_{};
  Tcp::ConnectionPool::ConnectionStatePtr conn_state_;
  Buffer::OwnedImpl buffer_;
  NiceMock<ThreadLocal::MockInstance> thread_local_;

  std::shared_ptr<TransactionInfos> transaction_infos_;

  RouteConstSharedPtr route_ptr_;
  std::unique_ptr<Router> router_;

  std::string cluster_name_{"cluster"};

  MsgType msg_type_{MsgType::Request};
  MessageMetadataSharedPtr metadata_;

  Tcp::ConnectionPool::UpstreamCallbacks* upstream_callbacks_{};
  NiceMock<Network::MockClientConnection> upstream_connection_;
};

class SipRouterTest : public testing::Test, public SipRouterTestBase {
public:
};

TEST_F(SipRouterTest, Call) {
  initializeTrans();
  initializeRouter();
  initializeTransaction();
  startRequest(MsgType::Request);
  connectUpstream();
  completeRequest();
  returnResponse();
  EXPECT_CALL(callbacks_, transactionId()).WillRepeatedly(Return("test"));
  destroyRouter();
}
/*TODO
TEST_F(SipRouterTest, Audit) {
  initializeTrans();
  initializeRouter();
  initializeTransaction();
  startRequest(MsgType::Request);
  connectUpstream();
  completeRequest();

  std::chrono::seconds TIMEOUT{5};
  std::this_thread::sleep_for(TIMEOUT);

} */

TEST_F(SipRouterTest, CallWithNotify) {
  initializeTrans();
  initializeRouter();
  initializeTransaction();

  initializeMetadata(MsgType::Request, MethodType::Notify);
  metadata_->setEP("10.0.0.1");
  EXPECT_CALL(callbacks_, route()).WillOnce(Return(route_ptr_));
  EXPECT_CALL(*route_, routeEntry()).WillOnce(Return(&route_entry_));
  EXPECT_CALL(route_entry_, clusterName()).WillRepeatedly(ReturnRef(cluster_name_));
  EXPECT_EQ(FilterStatus::Continue, router_->messageBegin(metadata_));

  auto& transation_info_ptr = (*transaction_infos_)[cluster_name_];
  EXPECT_NE(nullptr, transation_info_ptr);
  std::shared_ptr<UpstreamRequest> upstream_request_ptr =
      transation_info_ptr->getUpstreamRequest("10.0.0.1");
  EXPECT_NE(nullptr, upstream_request_ptr);
  upstream_request_ptr->resetStream();

  transation_info_ptr->deleteUpstreamRequest("10.0.0.1");
  upstream_request_ptr = transation_info_ptr->getUpstreamRequest("10.0.0.1");
  EXPECT_EQ(nullptr, upstream_request_ptr);
}

TEST_F(SipRouterTest, DiffRouter) {
  initializeTrans(false);
  initializeRouter();
  router_->metadataMatchCriteria();
  EXPECT_EQ(nullptr, router_->metadataMatchCriteria());
  initializeTransaction();
  EXPECT_EQ(router_->metadataMatchCriteria(), route_entry_.metadataMatchCriteria());
  startRequest(MsgType::Request);

  initializeRouter();
  startRequest(MsgType::Request);
}

TEST_F(SipRouterTest, DiffRouterDiffTrans) {
  initializeTrans();
  initializeRouter();
  initializeTransaction();
  startRequest(MsgType::Request);

  initializeRouter();

  initializeMetadata(MsgType::Request, MethodType::Invite);
  EXPECT_EQ(FilterStatus::Continue, router_->transportBegin(metadata_));

  EXPECT_CALL(callbacks_, route()).WillOnce(Return(route_ptr_));
  EXPECT_CALL(*route_, routeEntry()).WillOnce(Return(&route_entry_));
  EXPECT_CALL(route_entry_, clusterName()).WillRepeatedly(ReturnRef(cluster_name_));

  metadata_->setTransactionId("test1");
  EXPECT_EQ(FilterStatus::Continue, router_->messageBegin(metadata_));
}

TEST_F(SipRouterTest, DiffDestination) {
  initializeTrans();
  initializeRouter();
  initializeTransaction();
  initializeMetadata(MsgType::Request, MethodType::Ack);
  metadata_->setEP("10.0.0.1");
  EXPECT_EQ(FilterStatus::StopIteration, router_->messageBegin(metadata_));

  initializeRouter();
  initializeMetadata(MsgType::Request, MethodType::Ack);
  metadata_->setEP("10.0.0.1");
  metadata_->setDestination("10.0.0.1");

  EXPECT_CALL(callbacks_, route()).WillOnce(Return(route_ptr_));
  EXPECT_CALL(*route_, routeEntry()).WillOnce(Return(&route_entry_));
  EXPECT_CALL(route_entry_, clusterName()).WillRepeatedly(ReturnRef(cluster_name_));

  EXPECT_EQ(FilterStatus::Continue, router_->messageBegin(metadata_));
}

TEST_F(SipRouterTest, DiffDestinationDiffTrans) {
  initializeTrans();
  initializeRouter();
  initializeTransaction();
  startRequest(MsgType::Request);

  initializeRouter();
  initializeMetadata(MsgType::Request, MethodType::Ack);
  metadata_->setDestination("10.0.0.1");

  EXPECT_CALL(callbacks_, route()).WillOnce(Return(route_ptr_));
  EXPECT_CALL(*route_, routeEntry()).WillOnce(Return(&route_entry_));
  EXPECT_CALL(route_entry_, clusterName()).WillRepeatedly(ReturnRef(cluster_name_));

  metadata_->setTransactionId("test1");
  EXPECT_EQ(FilterStatus::Continue, router_->messageBegin(metadata_));
}

TEST_F(SipRouterTest, NoDestination) {
  initializeTrans();
  initializeRouter();
  initializeTransaction();

  initializeMetadata(MsgType::Request, MethodType::Invite, false);
  EXPECT_CALL(callbacks_, route()).WillOnce(Return(route_ptr_));
  EXPECT_CALL(*route_, routeEntry()).WillOnce(Return(&route_entry_));
  EXPECT_CALL(route_entry_, clusterName()).WillRepeatedly(ReturnRef(cluster_name_));

  EXPECT_EQ(FilterStatus::Continue, router_->messageBegin(metadata_));
}

TEST_F(SipRouterTest, CallNoRouter) {
  initializeTrans();
  initializeRouter();
  initializeTransaction();
  initializeMetadata(MsgType::Request);

  EXPECT_CALL(callbacks_, route()).WillOnce(Return(nullptr));
  EXPECT_CALL(callbacks_, sendLocalReply(_, _))
      .WillOnce(Invoke([&](const DirectResponse& response, bool end_stream) -> void {
        auto& app_ex = dynamic_cast<const AppException&>(response);
        EXPECT_EQ(AppExceptionType::UnknownMethod, app_ex.type_);
        EXPECT_THAT(app_ex.what(), ContainsRegex(".*no route.*"));
        EXPECT_TRUE(end_stream);
      }));
  EXPECT_EQ(FilterStatus::StopIteration, router_->messageBegin(metadata_));
  EXPECT_EQ(1U, context_.scope().counterFromString("test.route_missing").value());

  destroyRouter();
}

TEST_F(SipRouterTest, CallNoCluster) {
  initializeTrans();
  initializeRouter();
  initializeTransaction();
  initializeMetadata(MsgType::Request);

  EXPECT_CALL(callbacks_, route()).WillOnce(Return(route_ptr_));
  EXPECT_CALL(*route_, routeEntry()).WillOnce(Return(&route_entry_));
  EXPECT_CALL(route_entry_, clusterName()).WillRepeatedly(ReturnRef(cluster_name_));
  EXPECT_CALL(context_.cluster_manager_, getThreadLocalCluster(Eq(cluster_name_)))
      .WillOnce(Return(nullptr));
  EXPECT_CALL(callbacks_, sendLocalReply(_, _))
      .WillOnce(Invoke([&](const DirectResponse& response, bool end_stream) -> void {
        auto& app_ex = dynamic_cast<const AppException&>(response);
        EXPECT_EQ(AppExceptionType::InternalError, app_ex.type_);
        EXPECT_THAT(app_ex.what(), ContainsRegex(".*unknown cluster.*"));
        EXPECT_TRUE(end_stream);
      }));
  EXPECT_EQ(FilterStatus::StopIteration, router_->messageBegin(metadata_));
  EXPECT_EQ(1U, context_.scope().counterFromString("test.unknown_cluster").value());

  destroyRouter();
}

TEST_F(SipRouterTest, ClusterMaintenanceMode) {
  initializeTrans();
  initializeRouter();
  initializeTransaction();
  initializeMetadata(MsgType::Request);

  EXPECT_CALL(callbacks_, route()).WillOnce(Return(route_ptr_));
  EXPECT_CALL(*route_, routeEntry()).WillOnce(Return(&route_entry_));
  EXPECT_CALL(route_entry_, clusterName()).WillRepeatedly(ReturnRef(cluster_name_));
  EXPECT_CALL(*context_.cluster_manager_.thread_local_cluster_.cluster_.info_, maintenanceMode())
      .WillOnce(Return(true));

  EXPECT_CALL(callbacks_, sendLocalReply(_, _))
      .WillOnce(Invoke([&](const DirectResponse& response, bool end_stream) -> void {
        auto& app_ex = dynamic_cast<const AppException&>(response);
        EXPECT_EQ(AppExceptionType::InternalError, app_ex.type_);
        EXPECT_THAT(app_ex.what(), ContainsRegex(".*maintenance mode.*"));
        EXPECT_TRUE(end_stream);
      }));
  EXPECT_EQ(FilterStatus::StopIteration, router_->messageBegin(metadata_));
  EXPECT_EQ(1U, context_.scope().counterFromString("test.upstream_rq_maintenance_mode").value());
  destroyRouter();
}

TEST_F(SipRouterTest, NoHealthyHosts) {
  initializeTrans();
  initializeRouter();
  initializeTransaction();
  initializeMetadata(MsgType::Request);

  EXPECT_CALL(callbacks_, route()).WillOnce(Return(route_ptr_));
  EXPECT_CALL(*route_, routeEntry()).WillOnce(Return(&route_entry_));
  EXPECT_CALL(route_entry_, clusterName()).WillOnce(ReturnRef(cluster_name_));
  EXPECT_CALL(context_.cluster_manager_.thread_local_cluster_, tcpConnPool(_, _))
      .WillOnce(Return(absl::nullopt));

  EXPECT_CALL(callbacks_, sendLocalReply(_, _))
      .WillOnce(Invoke([&](const DirectResponse& response, bool end_stream) -> void {
        auto& app_ex = dynamic_cast<const AppException&>(response);
        EXPECT_EQ(AppExceptionType::InternalError, app_ex.type_);
        EXPECT_THAT(app_ex.what(), ContainsRegex(".*no healthy upstream.*"));
        EXPECT_TRUE(end_stream);
      }));
  EXPECT_EQ(FilterStatus::Continue, router_->messageBegin(metadata_));
  EXPECT_EQ(1U, context_.scope().counterFromString("test.no_healthy_upstream").value());
  destroyRouter();
}

TEST_F(SipRouterTest, NoHost) {
  initializeTrans();
  initializeRouter();
  initializeTransaction();
  initializeMetadata(MsgType::Request);

  EXPECT_CALL(callbacks_, route()).WillOnce(Return(route_ptr_));
  EXPECT_CALL(*route_, routeEntry()).WillOnce(Return(&route_entry_));
  EXPECT_CALL(route_entry_, clusterName()).WillOnce(ReturnRef(cluster_name_));

  EXPECT_CALL(context_.cluster_manager_.thread_local_cluster_.tcp_conn_pool_, host())
      .WillOnce(Return(nullptr));
  EXPECT_EQ(FilterStatus::Continue, router_->messageBegin(metadata_));
  destroyRouter();
}

TEST_F(SipRouterTest, NoNewConnection) {
  initializeTrans();
  initializeRouter();
  initializeTransaction();
  initializeMetadata(MsgType::Request);

  EXPECT_CALL(callbacks_, route()).WillOnce(Return(route_ptr_));
  EXPECT_CALL(*route_, routeEntry()).WillOnce(Return(&route_entry_));
  EXPECT_CALL(route_entry_, clusterName()).WillOnce(ReturnRef(cluster_name_));

  EXPECT_CALL(context_.cluster_manager_.thread_local_cluster_.tcp_conn_pool_, newConnection(_))
      .WillOnce(Return(nullptr));

  EXPECT_EQ(FilterStatus::Continue, router_->messageBegin(metadata_));
  destroyRouter();
}

TEST_F(SipRouterTest, CallWithExistingConnection) {
  initializeTrans();
  initializeRouter();
  initializeTransaction();
  startRequest(MsgType::Request);
  connectUpstream();
  completeRequest();
  returnResponse();
  EXPECT_CALL(callbacks_, transactionId()).WillRepeatedly(Return("test"));
  startRequestWithExistingConnection(MsgType::Request);
  destroyRouter();
}

TEST_F(SipRouterTest, PoolFailure) {
  initializeTrans();
  initializeRouter_callback();
  initializeTransaction();
  startRequest(MsgType::Request);
  // connectUpstream();
  context_.cluster_manager_.thread_local_cluster_.tcp_conn_pool_.poolFailure(
      ConnectionPool::PoolFailureReason::RemoteConnectionFailure);
  completeRequest();

  // auto& transaction_info_ptr = (*transaction_infos_)[cluster_name_];
  // EXPECT_NE(nullptr, transaction_info_ptr);
  // std::shared_ptr<UpstreamRequest> upstream_request_ptr =
  //     transaction_info_ptr->getUpstreamRequest("10.0.0.1");
  // EXPECT_NE(nullptr, upstream_request_ptr);

  // EXPECT_CALL(context_.cluster_manager_.thread_local_cluster_.tcp_conn_pool_, newConnection(_))
  //     .WillOnce(Return(nullptr));
  // upstream_request_ptr->start();
}

TEST_F(SipRouterTest, UpstreamCloseMidResponse) {
  initializeTrans();
  initializeRouter();
  initializeTransaction();
  startRequest(MsgType::Request);
  connectUpstream();

  upstream_callbacks_->onEvent(Network::ConnectionEvent::LocalClose);
  upstream_callbacks_->onEvent(Network::ConnectionEvent::RemoteClose);
  // Panic: NOT_REACHED_GCOVR_EXCL_LINE
  // upstream_callbacks_->onEvent(static_cast<Network::ConnectionEvent>(9999));
}

TEST_F(SipRouterTest, RouteEntryImplBase) {
  // initializeMetadata(MsgType::Request);
  const envoy::extensions::filters::network::sip_proxy::v3::Route route;
  GeneralRouteEntryImpl* base = new GeneralRouteEntryImpl(route);
  EXPECT_EQ("", base->clusterName());
  EXPECT_EQ(base, base->routeEntry());
}

envoy::extensions::filters::network::sip_proxy::v3::RouteConfiguration
parseConfigFromYaml(const std::string& yaml) {
  envoy::extensions::filters::network::sip_proxy::v3::RouteConfiguration route;
  TestUtility::loadFromYaml(yaml, route);
  return route;
}

TEST_F(SipRouterTest, RouteMatcher) {

  const std::string yaml = R"EOF(
  name: local_route
  routes:
    match:
      domain: A
    route:
      cluster: A
)EOF";

  envoy::extensions::filters::network::sip_proxy::v3::RouteConfiguration config;
  TestUtility::loadFromYaml(yaml, config);

  initializeMetadata(MsgType::Request);
  auto matcher_ptr = std::make_shared<RouteMatcher>(config);

  // Match domain
  metadata_->setDomain("A");
  matcher_ptr->route(*metadata_);

  // Not match domain
  metadata_->setDomain("B");
  matcher_ptr->route(*metadata_);
}

TEST_F(SipRouterTest, HandlePendingRequest) {
  initializeTrans();
  initializeRouter();
  initializeTransaction();
  startRequest(MsgType::Request);
  connectUpstream();
  completeRequest();

  auto& transation_info_ptr = (*transaction_infos_)[cluster_name_];
  EXPECT_NE(nullptr, transation_info_ptr);
  std::shared_ptr<UpstreamRequest> upstream_request_ptr =
      transation_info_ptr->getUpstreamRequest("10.0.0.1");
  EXPECT_NE(nullptr, upstream_request_ptr);
  upstream_request_ptr->addIntoPendingRequest(metadata_);
  upstream_request_ptr->onRequestStart();

  upstream_request_ptr->resetStream();
}

TEST_F(SipRouterTest, ResponseDecoderTransportBegin) {
  initializeTrans();
  initializeRouter();
  initializeTransaction();
  startRequest(MsgType::Request);

  initializeMetadata(MsgType::Request);
  auto& transation_info_ptr = (*transaction_infos_)[cluster_name_];
  EXPECT_NE(nullptr, transation_info_ptr);
  std::shared_ptr<UpstreamRequest> upstream_request_ptr =
      transation_info_ptr->getUpstreamRequest("10.0.0.1");
  EXPECT_NE(nullptr, upstream_request_ptr);
  std::shared_ptr<ResponseDecoder> response_decoder_ptr =
      std::make_shared<ResponseDecoder>(*upstream_request_ptr);
  response_decoder_ptr->transportBegin(metadata_);

  // No active trans
  metadata_->setTransactionId(nullptr);
  response_decoder_ptr->transportBegin(metadata_);
  // No trans id
  metadata_->resetTransactionId();
  response_decoder_ptr->transportBegin(metadata_);
}

} // namespace Router
} // namespace SipProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
