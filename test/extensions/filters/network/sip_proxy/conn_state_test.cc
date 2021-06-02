#include "extensions/filters/network/sip_proxy/conn_state.h"

#include "gtest/gtest.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace SipProxy {

// Test behavior of nextSequenceId()
TEST(SipConnectionStateTest, NextSequenceId) {
  // Default sequence ids
  {
    SipConnectionState cs;

    EXPECT_EQ(0, cs.nextSequenceId());
    EXPECT_EQ(1, cs.nextSequenceId());
  }

  // Overflow is handled without producing negative values.
  {
    SipConnectionState cs(std::numeric_limits<int32_t>::max());

    EXPECT_EQ(std::numeric_limits<int32_t>::max(), cs.nextSequenceId());
    EXPECT_EQ(0, cs.nextSequenceId());
  }
}

} // namespace SipProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
