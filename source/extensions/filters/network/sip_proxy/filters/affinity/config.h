#pragma once

#include "envoy/extensions/filters/network/sip_proxy/filters/affinity/v3/affinity.pb.h"
#include "envoy/extensions/filters/network/sip_proxy/filters/affinity/v3/affinity.pb.validate.h"

#include "extensions/filters/network/sip_proxy/filters/factory_base.h"
#include "extensions/filters/network/sip_proxy/filters/well_known_names.h"

namespace Envoy {
namespace Extensions {
namespace SipFilters {
namespace AffinityFilter {

using namespace Envoy::Extensions::NetworkFilters;

/**
 * Config registration for the affinity filter. @see NamedSipFilterConfigFactory.
 */
class AffinityFilterConfig
    : public SipProxy::SipFilters::FactoryBase<envoy::extensions::filters::network::sip_proxy::
                                                   filters::affinity::v3::AffinityConfiguration> {
public:
  AffinityFilterConfig() : FactoryBase(SipProxy::SipFilters::SipFilterNames::get().AFFINITY) {}

private:
  SipProxy::SipFilters::FilterFactoryCb
  createFilterFactoryFromProtoTyped(const envoy::extensions::filters::network::sip_proxy::filters::
                                        affinity::v3::AffinityConfiguration& proto_config,
                                    const std::string& stats_prefix,
                                    Server::Configuration::FactoryContext& context) override;
};

} // namespace AffinityFilter
} // namespace SipFilters
} // namespace Extensions
} // namespace Envoy
