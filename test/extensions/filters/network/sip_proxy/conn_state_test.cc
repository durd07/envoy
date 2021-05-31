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

// Test how markUpgraded/upgradedAttempts/isUpgraded when upgrade is successful.
TEST(SipConnectionStateTest, TestUpgradeSucceeded) {
  SipConnectionState cs;
  EXPECT_FALSE(cs.upgradeAttempted());
  EXPECT_FALSE(cs.isUpgraded());

  cs.markUpgraded();
  EXPECT_TRUE(cs.upgradeAttempted());
  EXPECT_TRUE(cs.isUpgraded());
}

// Test how markUpgraded/upgradedAttempts/isUpgraded when upgrade fails.
TEST(SipConnectionStateTest, TestUpgradeFailed) {
  SipConnectionState cs;
  EXPECT_FALSE(cs.upgradeAttempted());
  EXPECT_FALSE(cs.isUpgraded());

  cs.markUpgradeFailed();
  EXPECT_TRUE(cs.upgradeAttempted());
  EXPECT_FALSE(cs.isUpgraded());
}

} // namespace SipProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
