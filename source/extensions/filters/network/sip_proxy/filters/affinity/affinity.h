#pragma once

#include <memory>
#include <string>

#include "envoy/extensions/filters/network/sip_proxy/filters/affinity/v3/affinity.pb.h"
#include "envoy/stats/scope.h"
#include "envoy/stats/stats_macros.h"

#include "common/stats/symbol_table_impl.h"

#include "extensions/filters/network/sip_proxy/filters/pass_through_filter.h"

namespace Envoy {
namespace Extensions {
namespace SipFilters {
namespace AffinityFilter {

using namespace Envoy::Extensions::NetworkFilters;

/**
 * Global configuration for Thrift rate limit filter.
 */
class Config {
public:
  Config(const envoy::extensions::filters::network::sip_proxy::filters::affinity::v3::
             AffinityConfiguration& config)
      : name_(config.name()) {
    for (const auto& affinity : config.affinities()) {
      affinities_.emplace(std::make_pair(affinity.match().domain(), affinity.affinity()));
    }
  }

  const std::string& name() const { return name_; }
  envoy::extensions::filters::network::sip_proxy::filters::affinity::v3::AffinitySetting&
  affinity(std::string domain) {
    return affinities_[domain];
  }

private:
  const std::string name_;
  absl::flat_hash_map<
      std::string,
      envoy::extensions::filters::network::sip_proxy::filters::affinity::v3::AffinitySetting>
      affinities_;
};

using ConfigSharedPtr = std::shared_ptr<Config>;

class Filter : public SipProxy::SipFilters::PassThroughDecoderFilter {
public:
  Filter(ConfigSharedPtr config) : config_(config) {}
  ~Filter() override = default;

  // SipFilters::PassThroughDecoderFilter
  void onDestroy() override;
  SipProxy::FilterStatus messageBegin(SipProxy::MessageMetadataSharedPtr) override;

private:
  ConfigSharedPtr config_;
  envoy::extensions::filters::network::sip_proxy::filters::affinity::v3::AffinitySetting affinity_;
};

} // namespace AffinityFilter
} // namespace SipFilters
} // namespace Extensions
} // namespace Envoy
