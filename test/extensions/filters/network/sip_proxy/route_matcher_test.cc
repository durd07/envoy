#include "envoy/extensions/filters/network/sip_proxy/v3/route.pb.h"
#include "envoy/extensions/filters/network/sip_proxy/v3/route.pb.validate.h"

#include "source/common/config/metadata.h"
#include "source/extensions/filters/network/sip_proxy/router/config.h"
#include "source/extensions/filters/network/sip_proxy/router/router_impl.h"

#include "test/extensions/filters/network/sip_proxy/utility.h"
#include "test/test_common/utility.h"

#include "gtest/gtest.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace SipProxy {
namespace Router {
namespace {

// envoy::extensions::filters::network::sip_proxy::v3::RouteConfiguration
// parseRouteConfigurationFromV3Yaml(const std::string& yaml, bool avoid_boosting = true) {
//  envoy::extensions::filters::network::sip_proxy::v3::RouteConfiguration route_config;
//  TestUtility::loadFromYaml(yaml, route_config, false, avoid_boosting);
//  TestUtility::validate(route_config);
//  return route_config;
//}
//
// TEST(SipRouteMatcherTest, RouteByMethodNameWithNoInversion) {
//  const std::string yaml = R"EOF(
// name: config
// routes:
//  - match:
//      domain: "domain1"
//    route:
//      cluster: "cluster1"
//  - match:
//      domain: "domain2"
//    route:
//      cluster: "cluster2"
//)EOF";
//
//  envoy::extensions::filters::network::sip_proxy::v3::RouteConfiguration config =
//      parseRouteConfigurationFromV3Yaml(yaml);
//
//  RouteMatcher matcher(config);
//  MessageMetadata metadata;
//  EXPECT_EQ(nullptr, matcher.route(metadata, 0));
//  metadata.setDomain("unknown");
//  EXPECT_EQ(nullptr, matcher.route(metadata, 0));
//  metadata.setDomain("METHOD1");
//  EXPECT_EQ(nullptr, matcher.route(metadata, 0));
//
//  metadata.setDomain("method1");
//  RouteConstSharedPtr route = matcher.route(metadata, 0);
//  EXPECT_NE(nullptr, route);
//  EXPECT_EQ("cluster1", route->routeEntry()->clusterName());
//
//  metadata.setDomain("method2");
//  RouteConstSharedPtr route2 = matcher.route(metadata, 0);
//  EXPECT_NE(nullptr, route2);
//  EXPECT_EQ("cluster2", route2->routeEntry()->clusterName());
//}
//
// TEST(SipRouteMatcherTest, RouteByAnyMethodNameWithNoInversion) {
//  const std::string yaml = R"EOF(
// name: config
// routes:
//  - match:
//      domain: "domain1"
//    route:
//      cluster: "cluster1"
//  - match:
//      domain: ""
//    route:
//      cluster: "cluster2"
//)EOF";
//
//  envoy::extensions::filters::network::sip_proxy::v3::RouteConfiguration config =
//      parseRouteConfigurationFromV3Yaml(yaml);
//
//  RouteMatcher matcher(config);
//
//  {
//    MessageMetadata metadata;
//    metadata.setDomain("method1");
//    RouteConstSharedPtr route = matcher.route(metadata, 0);
//    EXPECT_NE(nullptr, route);
//    EXPECT_EQ("cluster1", route->routeEntry()->clusterName());
//
//    metadata.setDomain("anything");
//    RouteConstSharedPtr route2 = matcher.route(metadata, 0);
//    EXPECT_NE(nullptr, route2);
//    EXPECT_EQ("cluster2", route2->routeEntry()->clusterName());
//  }
//
//  {
//    MessageMetadata metadata;
//    RouteConstSharedPtr route2 = matcher.route(metadata, 0);
//    EXPECT_NE(nullptr, route2);
//    EXPECT_EQ("cluster2", route2->routeEntry()->clusterName());
//  }
//}
//
// TEST(SipRouteMatcherTest, RouteActionMetadataMatch) {
//  const std::string yaml = R"EOF(
// name: config
// routes:
//  - match:
//      domain: "domain1"
//    route:
//      cluster: cluster1
//  - match:
//      domain: "domain2"
//    route:
//      cluster: cluster2
//)EOF";
//
//  const envoy::extensions::filters::network::sip_proxy::v3::RouteConfiguration config =
//      parseRouteConfigurationFromV3Yaml(yaml);
//  RouteMatcher matcher(config);
//  MessageMetadata metadata;
//
//  // match with metadata
//  {
//    metadata.setDomain("method1");
//    RouteConstSharedPtr route = matcher.route(metadata, 0);
//    EXPECT_NE(nullptr, route);
//    EXPECT_NE(nullptr, route->routeEntry());
//
//    const Envoy::Router::MetadataMatchCriteria* criteria =
//        route->routeEntry()->metadataMatchCriteria();
//    EXPECT_NE(nullptr, criteria);
//    const std::vector<Envoy::Router::MetadataMatchCriterionConstSharedPtr>& mmc =
//        criteria->metadataMatchCriteria();
//    EXPECT_EQ(2, mmc.size());
//
//    ProtobufWkt::Value v1, v2;
//    v1.set_string_value("v1");
//    v2.set_string_value("v2");
//    HashedValue hv1(v1), hv2(v2);
//
//    EXPECT_EQ("k1", mmc[0]->name());
//    EXPECT_EQ(hv1, mmc[0]->value());
//
//    EXPECT_EQ("k2", mmc[1]->name());
//    EXPECT_EQ(hv2, mmc[1]->value());
//  }
//
//  // match with no metadata
//  {
//    metadata.setDomain("method2");
//    RouteConstSharedPtr route = matcher.route(metadata, 0);
//    EXPECT_NE(nullptr, route);
//    EXPECT_NE(nullptr, route->routeEntry());
//    EXPECT_EQ(nullptr, route->routeEntry()->metadataMatchCriteria());
//  }
//}

} // namespace
} // namespace Router
} // namespace SipProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
