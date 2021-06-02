#pragma once

#include <initializer_list>

#include "envoy/extensions/filters/network/sip_proxy/v3/sip_proxy.pb.h"

#include "common/buffer/buffer_impl.h"
#include "common/common/byte_order.h"

#include "extensions/filters/network/sip_proxy/sip.h"

#include "test/common/buffer/utility.h"

#include "absl/strings/ascii.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

using ::testing::TestParamInfo;

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace SipProxy {
namespace {

using Envoy::Buffer::addRepeated; // NOLINT(misc-unused-using-decls)
using Envoy::Buffer::addSeq;      // NOLINT(misc-unused-using-decls)

MATCHER_P2(HasAppException, t, m, "") {
  if (!arg.hasAppException()) {
    *result_listener << "has no exception";
    return false;
  }

  if (arg.appExceptionType() != t) {
    *result_listener << "has exception with type " << static_cast<int>(arg.appExceptionType());
    return false;
  }

  if (std::string(m) != arg.appExceptionMessage()) {
    *result_listener << "has exception with message " << arg.appExceptionMessage();
    return false;
  }

  return true;
}

} // namespace
} // namespace SipProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
