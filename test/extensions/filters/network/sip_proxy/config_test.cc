#include "envoy/extensions/filters/network/sip_proxy/v3/sip_proxy.pb.h"
#include "envoy/extensions/filters/network/sip_proxy/v3/sip_proxy.pb.validate.h"

#include "extensions/filters/network/sip_proxy/config.h"
#include "extensions/filters/network/sip_proxy/filters/factory_base.h"

#include "test/extensions/filters/network/sip_proxy/mocks.h"
#include "test/mocks/server/factory_context.h"
#include "test/test_common/registry.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

using testing::_;

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace SipProxy {

namespace {

envoy::extensions::filters::network::sip_proxy::v3::SipProxy
parseSipProxyFromYaml(const std::string& yaml) {
  envoy::extensions::filters::network::sip_proxy::v3::SipProxy sip_proxy;
  TestUtility::loadFromYaml(yaml, sip_proxy);
  return sip_proxy;
}
} // namespace

class SipFilterConfigTestBase {
public:
  void testConfig(envoy::extensions::filters::network::sip_proxy::v3::SipProxy& config) {
    Network::FilterFactoryCb cb;
    EXPECT_NO_THROW({ cb = factory_.createFilterFactoryFromProto(config, context_); });
    EXPECT_TRUE(factory_.isTerminalFilterByProto(config, context_));

    Network::MockConnection connection;
    EXPECT_CALL(connection, addReadFilter(_));
    cb(connection);
  }

  NiceMock<Server::Configuration::MockFactoryContext> context_;
  SipProxyFilterConfigFactory factory_;
};

class SipFilterConfigTest : public testing::Test, public SipFilterConfigTestBase {};

TEST_F(SipFilterConfigTest, ValidateFail) {
  EXPECT_THROW(factory_.createFilterFactoryFromProto(
                   envoy::extensions::filters::network::sip_proxy::v3::SipProxy(), context_),
               ProtoValidationException);
}

TEST_F(SipFilterConfigTest, ValidProtoConfiguration) {
  envoy::extensions::filters::network::sip_proxy::v3::SipProxy config{};
  config.set_stat_prefix("my_stat_prefix");

  testConfig(config);
}

// TEST_P(SipFilterTransportConfigTest, ValidProtoConfiguration) {
//  envoy::extensions::filters::network::sip_proxy::v3::SipProxy config{};
//  config.set_stat_prefix("my_stat_prefix");
//  config.set_transport(GetParam());
//  testConfig(config);
//}
//
// TEST_P(SipFilterProtocolConfigTest, ValidProtoConfiguration) {
//  envoy::extensions::filters::network::sip_proxy::v3::SipProxy config{};
//  config.set_stat_prefix("my_stat_prefix");
//  config.set_protocol(GetParam());
//  testConfig(config);
//}

TEST_F(SipFilterConfigTest, SipProxyWithEmptyProto) {
  envoy::extensions::filters::network::sip_proxy::v3::SipProxy config =
      *dynamic_cast<envoy::extensions::filters::network::sip_proxy::v3::SipProxy*>(
          factory_.createEmptyConfigProto().get());
  config.set_stat_prefix("my_stat_prefix");

  testConfig(config);
}

// Test config with an invalid cluster_header.
TEST_F(SipFilterConfigTest, RouterConfigWithInvalidClusterHeader) {
  const std::string yaml = R"EOF(
stat_prefix: sip
route_config:
  name: local_route
  routes:
    match:
      domain: A
    route:
      cluster: A
sip_filters:
  - name: envoy.filters.sip.router
)EOF";

  envoy::extensions::filters::network::sip_proxy::v3::SipProxy config = parseSipProxyFromYaml(yaml);
  std::string cluster = "A";
  cluster.push_back('\000'); // Add an invalid character for http header.
  config.mutable_route_config()->mutable_routes()->at(0).mutable_route()->set_cluster(cluster);
  EXPECT_THROW(factory_.createFilterFactoryFromProto(config, context_), ProtoValidationException);
}

// Test config with an explicitly defined router filter.
TEST_F(SipFilterConfigTest, SipProxyWithExplicitRouterConfig) {
  const std::string yaml = R"EOF(
stat_prefix: sip
route_config:
  name: local_route
sip_filters:
  - name: envoy.filters.sip.router
)EOF";

  envoy::extensions::filters::network::sip_proxy::v3::SipProxy config = parseSipProxyFromYaml(yaml);
  testConfig(config);
}

// Test config with an unknown filter.
TEST_F(SipFilterConfigTest, SipProxyWithUnknownFilter) {
  const std::string yaml = R"EOF(
stat_prefix: sip
route_config:
  name: local_route
sip_filters:
  - name: no_such_filter
  - name: envoy.filters.sip.router
)EOF";

  envoy::extensions::filters::network::sip_proxy::v3::SipProxy config = parseSipProxyFromYaml(yaml);

  EXPECT_THROW_WITH_REGEX(factory_.createFilterFactoryFromProto(config, context_), EnvoyException,
                          "no_such_filter");
}

// Test config with multiple filters.
TEST_F(SipFilterConfigTest, SipProxyWithMultipleFilters) {
  const std::string yaml = R"EOF(
stat_prefix: ingress
route_config:
  name: local_route
sip_filters:
  - name: envoy.filters.sip.mock_filter
    typed_config:
      "@type": type.googleapis.com/google.protobuf.Struct
      value:
        key: value
  - name: envoy.filters.sip.router
)EOF";

  SipFilters::MockFilterConfigFactory factory;
  Registry::InjectFactory<SipFilters::NamedSipFilterConfigFactory> registry(factory);

  envoy::extensions::filters::network::sip_proxy::v3::SipProxy config = parseSipProxyFromYaml(yaml);
  testConfig(config);

  EXPECT_EQ(1, factory.config_struct_.fields_size());
  EXPECT_EQ("value", factory.config_struct_.fields().at("key").string_value());
  EXPECT_EQ("sip.ingress.", factory.config_stat_prefix_);
}

} // namespace SipProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
