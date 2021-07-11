#include "extensions/filters/network/sip_proxy/metadata.h"

#include "test/test_common/printers.h"
#include "test/test_common/utility.h"

#include "gtest/gtest.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace SipProxy {

 TEST(MessageMetadataTest, Fields) {
  MessageMetadata metadata;

  metadata.setRespMethodType(MethodType::Invite);
  EXPECT_EQ(MethodType::Invite, metadata.respMethodType());

  metadata.setInsertEPLocation(100);
  EXPECT_EQ(100, metadata.insertEPLocation());

  metadata.setInsertTagLocation(200);
  EXPECT_EQ(200, metadata.insertTagLocation());

}

} // namespace SipProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
