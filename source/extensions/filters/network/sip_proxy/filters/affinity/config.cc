#include "extensions/filters/network/sip_proxy/filters/affinity/config.h"

#include <memory>

#include "envoy/extensions/filters/network/sip_proxy/filters/affinity/v3/affinity.pb.h"
#include "envoy/extensions/filters/network/sip_proxy/filters/affinity/v3/affinity.pb.validate.h"
#include "envoy/registry/registry.h"

#include "common/common/macros.h"
#include "common/config/utility.h"
#include "common/protobuf/utility.h"

#include "extensions/filters/network/sip_proxy/filters/affinity/affinity.h"

namespace Envoy {
namespace Extensions {
namespace SipFilters {
namespace AffinityFilter {

using namespace Envoy::Extensions::NetworkFilters;

SipProxy::SipFilters::FilterFactoryCb AffinityFilterConfig::createFilterFactoryFromProtoTyped(
    const envoy::extensions::filters::network::sip_proxy::filters::affinity::v3::
        AffinityConfiguration& proto_config,
    const std::string&, Server::Configuration::FactoryContext& context) {
  UNREFERENCED_PARAMETER(context);
  ConfigSharedPtr config(new Config(proto_config));

  return [config](SipProxy::SipFilters::FilterChainFactoryCallbacks& callbacks) -> void {
    callbacks.addDecoderFilter(std::make_shared<Filter>(config));
  };
}

/**
 * Static registration for the affinity filter. @see RegisterFactory.
 */
REGISTER_FACTORY(AffinityFilterConfig, SipProxy::SipFilters::NamedSipFilterConfigFactory);

} // namespace AffinityFilter
} // namespace SipFilters
} // namespace Extensions
} // namespace Envoy
