#include "extensions/filters/network/sip_proxy/filters/affinity/affinity.h"

#include "common/common/macros.h"
#include "common/tracing/http_tracer_impl.h"

#include "extensions/filters/network/sip_proxy/app_exception_impl.h"
#include "extensions/filters/network/sip_proxy/filters/well_known_names.h"
#include "extensions/filters/network/sip_proxy/router/router.h"

namespace Envoy {
namespace Extensions {
namespace SipFilters {
namespace AffinityFilter {

using namespace Envoy::Extensions::NetworkFilters;

SipProxy::FilterStatus Filter::messageBegin(SipProxy::MessageMetadataSharedPtr metadata) {
  affinity_ = config_->affinity(std::string(metadata->domain().value()));

  if (affinity_.session_affinity()) {
    switch (metadata->methodType()) {
    case SipProxy::MethodType::Invite: {
      return SipProxy::FilterStatus::Continue;
    }
    case SipProxy::MethodType::Ack: {
      if (metadata->EP().has_value()) {
        auto host = metadata->EP().value();
        metadata->setDestination(host);
      }
    }
    default:
      if (metadata->EP().has_value()) {
        auto host = metadata->EP().value();
        metadata->setDestination(host);
      }
    }
  }

  if (affinity_.registration_affinity()) {
  }

  return SipProxy::FilterStatus::Continue;
}

void Filter::onDestroy() {}

} // namespace AffinityFilter
} // namespace SipFilters
} // namespace Extensions
} // namespace Envoy
