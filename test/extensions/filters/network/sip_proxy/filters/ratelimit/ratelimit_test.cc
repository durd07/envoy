#include <memory>
#include <string>
#include <vector>

#include "envoy/extensions/filters/network/sip_proxy/filters/ratelimit/v3/rate_limit.pb.h"

#include "common/buffer/buffer_impl.h"
#include "common/common/empty_string.h"
#include "common/http/headers.h"

#include "extensions/filters/network/sip_proxy/app_exception_impl.h"
#include "extensions/filters/network/sip_proxy/filters/ratelimit/ratelimit.h"
#include "extensions/filters/network/sip_proxy/filters/well_known_names.h"

#include "test/extensions/filters/common/ratelimit/mocks.h"
#include "test/extensions/filters/network/sip_proxy/mocks.h"
#include "test/mocks/local_info/mocks.h"
#include "test/mocks/ratelimit/mocks.h"
#include "test/mocks/runtime/mocks.h"
#include "test/mocks/tracing/mocks.h"
#include "test/mocks/upstream/cluster_manager.h"
#include "test/test_common/printers.h"
#include "test/test_common/utility.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

using testing::_;
using testing::InSequence;
using testing::Invoke;
using testing::NiceMock;
using testing::Return;
using testing::SetArgReferee;
using testing::WithArgs;

namespace Envoy {
namespace Extensions {
namespace SipFilters {
namespace RateLimitFilter {

using namespace Envoy::Extensions::NetworkFilters;

class SipRateLimitFilterTest : public testing::Test {
public:
  SipRateLimitFilterTest() {
    ON_CALL(runtime_.snapshot_, featureEnabled("ratelimit.sip_filter_enabled", 100))
        .WillByDefault(Return(true));
    ON_CALL(runtime_.snapshot_, featureEnabled("ratelimit.sip_filter_enforcing", 100))
        .WillByDefault(Return(true));
    ON_CALL(runtime_.snapshot_, featureEnabled("ratelimit.test_key.sip_filter_enabled", 100))
        .WillByDefault(Return(true));
    cm_.initializeThreadLocalClusters({"fake_cluster"});
  }

  void setupTest(const std::string& yaml) {
    envoy::extensions::filters::network::sip_proxy::filters::ratelimit::v3::RateLimit
        proto_config{};
    TestUtility::loadFromYaml(yaml, proto_config, false, true);

    config_ = std::make_shared<Config>(proto_config, local_info_, stats_store_, runtime_, cm_);

    request_metadata_ = std::make_shared<SipProxy::MessageMetadata>();

    client_ = new Filters::Common::RateLimit::MockClient();
    filter_ = std::make_unique<Filter>(config_, Filters::Common::RateLimit::ClientPtr{client_});
    filter_->setDecoderFilterCallbacks(filter_callbacks_);
    filter_callbacks_.route_->route_entry_.rate_limit_policy_.rate_limit_policy_entry_.clear();
    filter_callbacks_.route_->route_entry_.rate_limit_policy_.rate_limit_policy_entry_.emplace_back(
        route_rate_limit_);
  }

  const std::string fail_close_config_ = R"EOF(
  domain: foo
  failure_mode_deny: true
  )EOF";

  const std::string filter_config_ = R"EOF(
  domain: foo
  )EOF";

  NiceMock<Stats::MockIsolatedStatsStore> stats_store_;
  ConfigSharedPtr config_;
  Filters::Common::RateLimit::MockClient* client_;
  std::unique_ptr<Filter> filter_;
  NiceMock<SipProxy::SipFilters::MockDecoderFilterCallbacks> filter_callbacks_;
  Filters::Common::RateLimit::RequestCallbacks* request_callbacks_{};
  SipProxy::MessageMetadataSharedPtr request_metadata_;
  Http::TestResponseHeaderMapImpl response_headers_;
  Buffer::OwnedImpl data_;
  Buffer::OwnedImpl response_data_;
  NiceMock<Runtime::MockLoader> runtime_;
  NiceMock<Upstream::MockClusterManager> cm_;
  NiceMock<SipProxy::Router::MockRateLimitPolicyEntry> route_rate_limit_;
  std::vector<RateLimit::Descriptor> descriptor_{{{{"descriptor_key", "descriptor_value"}}}};
  NiceMock<LocalInfo::MockLocalInfo> local_info_;
};

TEST_F(SipRateLimitFilterTest, NoRoute) {
  setupTest(filter_config_);

  EXPECT_CALL(*filter_callbacks_.route_, routeEntry()).WillOnce(Return(nullptr));

  EXPECT_EQ(SipProxy::FilterStatus::Continue, filter_->transportBegin(request_metadata_));
  EXPECT_EQ(SipProxy::FilterStatus::Continue, filter_->messageBegin(request_metadata_));
  {
    std::string dummy_str = "dummy";
    EXPECT_EQ(SipProxy::FilterStatus::Continue, filter_->structBegin(dummy_str));
  }
  {
    std::string dummy_str = "dummy";
    SipProxy::FieldType dummy_ft{SipProxy::FieldType::I32};
    int16_t dummy_id{1};
    EXPECT_EQ(SipProxy::FilterStatus::Continue, filter_->fieldBegin(dummy_str, dummy_ft, dummy_id));
  }
  {
    bool dummy_val{false};
    EXPECT_EQ(SipProxy::FilterStatus::Continue, filter_->boolValue(dummy_val));
  }
  {
    uint8_t dummy_val{0};
    EXPECT_EQ(SipProxy::FilterStatus::Continue, filter_->byteValue(dummy_val));
  }
  {
    int16_t dummy_val{0};
    EXPECT_EQ(SipProxy::FilterStatus::Continue, filter_->int16Value(dummy_val));
  }
  {
    int32_t dummy_val{0};
    EXPECT_EQ(SipProxy::FilterStatus::Continue, filter_->int32Value(dummy_val));
  }
  {
    int64_t dummy_val{0};
    EXPECT_EQ(SipProxy::FilterStatus::Continue, filter_->int64Value(dummy_val));
  }
  {
    double dummy_val{0.0};
    EXPECT_EQ(SipProxy::FilterStatus::Continue, filter_->doubleValue(dummy_val));
  }
  {
    std::string dummy_str = "dummy";
    EXPECT_EQ(SipProxy::FilterStatus::Continue, filter_->stringValue(dummy_str));
  }
  {
    SipProxy::FieldType dummy_ft = SipProxy::FieldType::I32;
    uint32_t dummy_size{1};
    EXPECT_EQ(SipProxy::FilterStatus::Continue, filter_->mapBegin(dummy_ft, dummy_ft, dummy_size));
    EXPECT_EQ(SipProxy::FilterStatus::Continue, filter_->mapEnd());
  }
  {
    SipProxy::FieldType dummy_ft = SipProxy::FieldType::I32;
    uint32_t dummy_size{1};
    EXPECT_EQ(SipProxy::FilterStatus::Continue, filter_->listBegin(dummy_ft, dummy_size));
    EXPECT_EQ(SipProxy::FilterStatus::Continue, filter_->listEnd());
  }
  {
    SipProxy::FieldType dummy_ft = SipProxy::FieldType::I32;
    uint32_t dummy_size{1};
    EXPECT_EQ(SipProxy::FilterStatus::Continue, filter_->setBegin(dummy_ft, dummy_size));
    EXPECT_EQ(SipProxy::FilterStatus::Continue, filter_->setEnd());
  }
  EXPECT_EQ(SipProxy::FilterStatus::Continue, filter_->structEnd());
  EXPECT_EQ(SipProxy::FilterStatus::Continue, filter_->fieldEnd());
  EXPECT_EQ(SipProxy::FilterStatus::Continue, filter_->messageEnd());
  EXPECT_EQ(SipProxy::FilterStatus::Continue, filter_->transportEnd());
}

TEST_F(SipRateLimitFilterTest, NoCluster) {
  setupTest(filter_config_);

  ON_CALL(cm_, getThreadLocalCluster(_)).WillByDefault(Return(nullptr));

  EXPECT_EQ(SipProxy::FilterStatus::Continue, filter_->messageBegin(request_metadata_));
}

TEST_F(SipRateLimitFilterTest, NoApplicableRateLimit) {
  setupTest(filter_config_);

  filter_callbacks_.route_->route_entry_.rate_limit_policy_.rate_limit_policy_entry_.clear();
  EXPECT_CALL(*client_, limit(_, _, _, _, _)).Times(0);

  EXPECT_EQ(SipProxy::FilterStatus::Continue, filter_->messageBegin(request_metadata_));
}

TEST_F(SipRateLimitFilterTest, NoDescriptor) {
  setupTest(filter_config_);

  EXPECT_CALL(route_rate_limit_, populateDescriptors(_, _, _, _, _));
  EXPECT_CALL(*client_, limit(_, _, _, _, _)).Times(0);

  EXPECT_EQ(SipProxy::FilterStatus::Continue, filter_->messageBegin(request_metadata_));
}

TEST_F(SipRateLimitFilterTest, RuntimeDisabled) {
  setupTest(filter_config_);

  EXPECT_CALL(runtime_.snapshot_, featureEnabled("ratelimit.sip_filter_enabled", 100))
      .WillOnce(Return(false));

  EXPECT_EQ(SipProxy::FilterStatus::Continue, filter_->messageBegin(request_metadata_));
}

TEST_F(SipRateLimitFilterTest, OkResponse) {
  setupTest(filter_config_);
  InSequence s;

  EXPECT_CALL(filter_callbacks_.route_->route_entry_.rate_limit_policy_, getApplicableRateLimit(0));

  EXPECT_CALL(route_rate_limit_, populateDescriptors(_, _, _, _, _))
      .WillOnce(SetArgReferee<1>(descriptor_));

  EXPECT_CALL(*client_, limit(_, "foo",
                              testing::ContainerEq(std::vector<RateLimit::Descriptor>{
                                  {{{"descriptor_key", "descriptor_value"}}}}),
                              _, _))
      .WillOnce(
          WithArgs<0>(Invoke([&](Filters::Common::RateLimit::RequestCallbacks& callbacks) -> void {
            request_callbacks_ = &callbacks;
          })));

  request_metadata_->headers().addCopy(SipProxy::Headers::get().ClientId, "clientid");

  EXPECT_EQ(SipProxy::FilterStatus::StopIteration, filter_->messageBegin(request_metadata_));

  EXPECT_CALL(filter_callbacks_, continueDecoding());
  EXPECT_CALL(filter_callbacks_.stream_info_,
              setResponseFlag(StreamInfo::ResponseFlag::RateLimited))
      .Times(0);
  request_callbacks_->complete(Filters::Common::RateLimit::LimitStatus::OK, nullptr, nullptr,
                               nullptr, "", nullptr);

  EXPECT_EQ(1U,
            cm_.thread_local_cluster_.cluster_.info_->stats_store_.counter("ratelimit.ok").value());
}

TEST_F(SipRateLimitFilterTest, ImmediateOkResponse) {
  setupTest(filter_config_);
  InSequence s;

  EXPECT_CALL(route_rate_limit_, populateDescriptors(_, _, _, _, _))
      .WillOnce(SetArgReferee<1>(descriptor_));

  EXPECT_CALL(*client_, limit(_, "foo",
                              testing::ContainerEq(std::vector<RateLimit::Descriptor>{
                                  {{{"descriptor_key", "descriptor_value"}}}}),
                              _, _))
      .WillOnce(
          WithArgs<0>(Invoke([&](Filters::Common::RateLimit::RequestCallbacks& callbacks) -> void {
            callbacks.complete(Filters::Common::RateLimit::LimitStatus::OK, nullptr, nullptr,
                               nullptr, "", nullptr);
          })));

  EXPECT_CALL(filter_callbacks_, continueDecoding()).Times(0);
  EXPECT_EQ(SipProxy::FilterStatus::Continue, filter_->messageBegin(request_metadata_));

  EXPECT_EQ(1U,
            cm_.thread_local_cluster_.cluster_.info_->stats_store_.counter("ratelimit.ok").value());
}

TEST_F(SipRateLimitFilterTest, ImmediateErrorResponse) {
  setupTest(filter_config_);
  InSequence s;

  EXPECT_CALL(route_rate_limit_, populateDescriptors(_, _, _, _, _))
      .WillOnce(SetArgReferee<1>(descriptor_));

  EXPECT_CALL(*client_, limit(_, "foo",
                              testing::ContainerEq(std::vector<RateLimit::Descriptor>{
                                  {{{"descriptor_key", "descriptor_value"}}}}),
                              _, _))
      .WillOnce(
          WithArgs<0>(Invoke([&](Filters::Common::RateLimit::RequestCallbacks& callbacks) -> void {
            callbacks.complete(Filters::Common::RateLimit::LimitStatus::Error, nullptr, nullptr,
                               nullptr, "", nullptr);
          })));

  EXPECT_CALL(filter_callbacks_, continueDecoding()).Times(0);
  EXPECT_EQ(SipProxy::FilterStatus::Continue, filter_->messageBegin(request_metadata_));

  EXPECT_EQ(
      1U,
      cm_.thread_local_cluster_.cluster_.info_->stats_store_.counter("ratelimit.error").value());
  EXPECT_EQ(1U, cm_.thread_local_cluster_.cluster_.info_->stats_store_
                    .counter("ratelimit.failure_mode_allowed")
                    .value());
}

TEST_F(SipRateLimitFilterTest, ErrorResponse) {
  setupTest(filter_config_);
  InSequence s;

  EXPECT_CALL(route_rate_limit_, populateDescriptors(_, _, _, _, _))
      .WillOnce(SetArgReferee<1>(descriptor_));
  EXPECT_CALL(*client_, limit(_, _, _, _, _))
      .WillOnce(
          WithArgs<0>(Invoke([&](Filters::Common::RateLimit::RequestCallbacks& callbacks) -> void {
            request_callbacks_ = &callbacks;
          })));

  EXPECT_EQ(SipProxy::FilterStatus::StopIteration, filter_->messageBegin(request_metadata_));

  EXPECT_CALL(filter_callbacks_, continueDecoding());
  request_callbacks_->complete(Filters::Common::RateLimit::LimitStatus::Error, nullptr, nullptr,
                               nullptr, "", nullptr);

  EXPECT_EQ(SipProxy::FilterStatus::Continue, filter_->messageEnd());
  EXPECT_CALL(filter_callbacks_.stream_info_,
              setResponseFlag(StreamInfo::ResponseFlag::RateLimited))
      .Times(0);

  EXPECT_EQ(
      1U,
      cm_.thread_local_cluster_.cluster_.info_->stats_store_.counter("ratelimit.error").value());
  EXPECT_EQ(1U, cm_.thread_local_cluster_.cluster_.info_->stats_store_
                    .counter("ratelimit.failure_mode_allowed")
                    .value());
}

TEST_F(SipRateLimitFilterTest, ErrorResponseWithDynamicMetadata) {
  setupTest(filter_config_);
  InSequence s;

  EXPECT_CALL(route_rate_limit_, populateDescriptors(_, _, _, _, _))
      .WillOnce(SetArgReferee<1>(descriptor_));
  EXPECT_CALL(*client_, limit(_, _, _, _, _))
      .WillOnce(
          WithArgs<0>(Invoke([&](Filters::Common::RateLimit::RequestCallbacks& callbacks) -> void {
            request_callbacks_ = &callbacks;
          })));

  EXPECT_EQ(SipProxy::FilterStatus::StopIteration, filter_->messageBegin(request_metadata_));

  Filters::Common::RateLimit::DynamicMetadataPtr dynamic_metadata =
      std::make_unique<ProtobufWkt::Struct>();
  auto* fields = dynamic_metadata->mutable_fields();
  (*fields)["name"] = ValueUtil::stringValue("my-limit");
  (*fields)["x"] = ValueUtil::numberValue(3);
  EXPECT_CALL(filter_callbacks_.stream_info_, setDynamicMetadata(_, _))
      .WillOnce(Invoke([&dynamic_metadata](const std::string& ns,
                                           const ProtobufWkt::Struct& returned_dynamic_metadata) {
        EXPECT_EQ(ns, SipProxy::SipFilters::SipFilterNames::get().RATE_LIMIT);
        EXPECT_TRUE(TestUtility::protoEqual(returned_dynamic_metadata, *dynamic_metadata));
      }));

  EXPECT_CALL(filter_callbacks_, continueDecoding());
  request_callbacks_->complete(Filters::Common::RateLimit::LimitStatus::Error, nullptr, nullptr,
                               nullptr, "", std::move(dynamic_metadata));

  EXPECT_EQ(SipProxy::FilterStatus::Continue, filter_->messageEnd());
  EXPECT_CALL(filter_callbacks_.stream_info_,
              setResponseFlag(StreamInfo::ResponseFlag::RateLimited))
      .Times(0);

  EXPECT_EQ(
      1U,
      cm_.thread_local_cluster_.cluster_.info_->stats_store_.counter("ratelimit.error").value());
  EXPECT_EQ(1U, cm_.thread_local_cluster_.cluster_.info_->stats_store_
                    .counter("ratelimit.failure_mode_allowed")
                    .value());
}

TEST_F(SipRateLimitFilterTest, ErrorResponseWithFailureModeAllowOff) {
  setupTest(fail_close_config_);
  InSequence s;

  EXPECT_CALL(route_rate_limit_, populateDescriptors(_, _, _, _, _))
      .WillOnce(SetArgReferee<1>(descriptor_));
  EXPECT_CALL(*client_, limit(_, _, _, _, _))
      .WillOnce(
          WithArgs<0>(Invoke([&](Filters::Common::RateLimit::RequestCallbacks& callbacks) -> void {
            request_callbacks_ = &callbacks;
          })));

  EXPECT_EQ(SipProxy::FilterStatus::StopIteration, filter_->messageBegin(request_metadata_));

  EXPECT_CALL(filter_callbacks_, sendLocalReply(_, false))
      .WillOnce(Invoke([&](const SipProxy::DirectResponse& response, bool) -> void {
        const auto& app_ex = dynamic_cast<const SipProxy::AppException&>(response);
        EXPECT_STREQ("limiter error", app_ex.what());
        EXPECT_EQ(SipProxy::AppExceptionType::InternalError, app_ex.type_);
      }));
  EXPECT_CALL(filter_callbacks_.stream_info_,
              setResponseFlag(StreamInfo::ResponseFlag::RateLimitServiceError));
  request_callbacks_->complete(Filters::Common::RateLimit::LimitStatus::Error, nullptr, nullptr,
                               nullptr, "", nullptr);

  EXPECT_EQ(
      1U,
      cm_.thread_local_cluster_.cluster_.info_->stats_store_.counter("ratelimit.error").value());
  EXPECT_EQ(0U, cm_.thread_local_cluster_.cluster_.info_->stats_store_
                    .counter("ratelimit.failure_mode_allowed")
                    .value());
}

TEST_F(SipRateLimitFilterTest, LimitResponse) {
  setupTest(filter_config_);
  InSequence s;

  EXPECT_CALL(route_rate_limit_, populateDescriptors(_, _, _, _, _))
      .WillOnce(SetArgReferee<1>(descriptor_));
  EXPECT_CALL(*client_, limit(_, _, _, _, _))
      .WillOnce(
          WithArgs<0>(Invoke([&](Filters::Common::RateLimit::RequestCallbacks& callbacks) -> void {
            request_callbacks_ = &callbacks;
          })));

  EXPECT_EQ(SipProxy::FilterStatus::StopIteration, filter_->messageBegin(request_metadata_));

  EXPECT_CALL(filter_callbacks_, sendLocalReply(_, false))
      .WillOnce(Invoke([&](const SipProxy::DirectResponse& response, bool) -> void {
        const auto& app_ex = dynamic_cast<const SipProxy::AppException&>(response);
        EXPECT_STREQ("over limit", app_ex.what());
        EXPECT_EQ(SipProxy::AppExceptionType::InternalError, app_ex.type_);
      }));
  EXPECT_CALL(filter_callbacks_, continueDecoding()).Times(0);
  EXPECT_CALL(filter_callbacks_.stream_info_,
              setResponseFlag(StreamInfo::ResponseFlag::RateLimited));
  request_callbacks_->complete(Filters::Common::RateLimit::LimitStatus::OverLimit, nullptr, nullptr,
                               nullptr, "", nullptr);

  EXPECT_EQ(1U,
            cm_.thread_local_cluster_.cluster_.info_->stats_store_.counter("ratelimit.over_limit")
                .value());
}

TEST_F(SipRateLimitFilterTest, LimitResponseWithHeaders) {
  setupTest(filter_config_);
  InSequence s;

  EXPECT_CALL(route_rate_limit_, populateDescriptors(_, _, _, _, _))
      .WillOnce(SetArgReferee<1>(descriptor_));
  EXPECT_CALL(*client_, limit(_, _, _, _, _))
      .WillOnce(
          WithArgs<0>(Invoke([&](Filters::Common::RateLimit::RequestCallbacks& callbacks) -> void {
            request_callbacks_ = &callbacks;
          })));

  EXPECT_EQ(SipProxy::FilterStatus::StopIteration, filter_->messageBegin(request_metadata_));

  Http::HeaderMapPtr rl_headers{new Http::TestRequestHeaderMapImpl{
      {"x-ratelimit-limit", "1000"}, {"x-ratelimit-remaining", "0"}, {"retry-after", "33"}}};

  EXPECT_CALL(filter_callbacks_, continueDecoding()).Times(0);

  // TODO(zuercher): Headers are currently ignored, but sendLocalReply is the place to pass them.
  EXPECT_CALL(filter_callbacks_, sendLocalReply(_, false));
  EXPECT_CALL(filter_callbacks_.stream_info_,
              setResponseFlag(StreamInfo::ResponseFlag::RateLimited));

  Http::ResponseHeaderMapPtr h{new Http::TestResponseHeaderMapImpl(*rl_headers)};
  request_callbacks_->complete(Filters::Common::RateLimit::LimitStatus::OverLimit, nullptr,
                               std::move(h), nullptr, "", nullptr);

  EXPECT_EQ(1U,
            cm_.thread_local_cluster_.cluster_.info_->stats_store_.counter("ratelimit.over_limit")
                .value());
}

TEST_F(SipRateLimitFilterTest, LimitResponseRuntimeDisabled) {
  setupTest(filter_config_);
  InSequence s;

  EXPECT_CALL(route_rate_limit_, populateDescriptors(_, _, _, _, _))
      .WillOnce(SetArgReferee<1>(descriptor_));
  EXPECT_CALL(*client_, limit(_, _, _, _, _))
      .WillOnce(
          WithArgs<0>(Invoke([&](Filters::Common::RateLimit::RequestCallbacks& callbacks) -> void {
            request_callbacks_ = &callbacks;
          })));

  EXPECT_EQ(SipProxy::FilterStatus::StopIteration, filter_->messageBegin(request_metadata_));

  EXPECT_CALL(runtime_.snapshot_, featureEnabled("ratelimit.sip_filter_enforcing", 100))
      .WillOnce(Return(false));
  EXPECT_CALL(filter_callbacks_, continueDecoding());
  request_callbacks_->complete(Filters::Common::RateLimit::LimitStatus::OverLimit, nullptr, nullptr,
                               nullptr, "", nullptr);

  EXPECT_EQ(1U,
            cm_.thread_local_cluster_.cluster_.info_->stats_store_.counter("ratelimit.over_limit")
                .value());
}

TEST_F(SipRateLimitFilterTest, ResetDuringCall) {
  setupTest(filter_config_);
  InSequence s;

  EXPECT_CALL(route_rate_limit_, populateDescriptors(_, _, _, _, _))
      .WillOnce(SetArgReferee<1>(descriptor_));
  EXPECT_CALL(*client_, limit(_, _, _, _, _))
      .WillOnce(
          WithArgs<0>(Invoke([&](Filters::Common::RateLimit::RequestCallbacks& callbacks) -> void {
            request_callbacks_ = &callbacks;
          })));

  EXPECT_EQ(SipProxy::FilterStatus::StopIteration, filter_->messageBegin(request_metadata_));

  EXPECT_CALL(*client_, cancel());
  filter_->onDestroy();
}

TEST_F(SipRateLimitFilterTest, RouteRateLimitDisabledForRouteKey) {
  route_rate_limit_.disable_key_ = "test_key";
  setupTest(filter_config_);

  ON_CALL(runtime_.snapshot_, featureEnabled("ratelimit.test_key.sip_filter_enabled", 100))
      .WillByDefault(Return(false));

  EXPECT_CALL(route_rate_limit_, populateDescriptors(_, _, _, _, _)).Times(0);
  EXPECT_CALL(*client_, limit(_, _, _, _, _)).Times(0);

  EXPECT_EQ(SipProxy::FilterStatus::Continue, filter_->messageBegin(request_metadata_));
}

TEST_F(SipRateLimitFilterTest, ConfigValueTest) {
  std::string stage_filter_config = R"EOF(
  {
    "domain": "foo",
    "stage": 5,
  }
  )EOF";

  setupTest(stage_filter_config);

  EXPECT_EQ(5UL, config_->stage());
  EXPECT_EQ("foo", config_->domain());
}

TEST_F(SipRateLimitFilterTest, DefaultConfigValueTest) {
  std::string stage_filter_config = R"EOF(
  {
    "domain": "foo"
  }
  )EOF";

  setupTest(stage_filter_config);

  EXPECT_EQ(0UL, config_->stage());
  EXPECT_EQ("foo", config_->domain());
}

} // namespace RateLimitFilter
} // namespace SipFilters
} // namespace Extensions
} // namespace Envoy
