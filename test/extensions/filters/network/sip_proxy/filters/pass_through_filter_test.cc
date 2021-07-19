#include <memory>
#include <string>

#include "source/extensions/filters/network/sip_proxy/filters/pass_through_filter.h"

#include "test/extensions/filters/network/sip_proxy/mocks.h"
#include "test/test_common/printers.h"
#include "test/test_common/utility.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

using testing::NiceMock;

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace SipProxy {
namespace SipFilters {

using namespace Envoy::Extensions::NetworkFilters;

class SipPassThroughDecoderFilterTest : public testing::Test {
public:
  class Filter : public PassThroughDecoderFilter {
  public:
    DecoderFilterCallbacks* decoderFilterCallbacks() { return decoder_callbacks_; }
  };

  void initialize() {
    filter_ = std::make_unique<Filter>();
    filter_->setDecoderFilterCallbacks(filter_callbacks_);
  }

  std::unique_ptr<Filter> filter_;
  NiceMock<MockDecoderFilterCallbacks> filter_callbacks_;
  SipProxy::MessageMetadataSharedPtr request_metadata_;
};

// Tests that each method returns SipProxy::FilterStatus::Continue.
TEST_F(SipPassThroughDecoderFilterTest, AllMethodsAreImplementedTrivially) {
  initialize();

  EXPECT_EQ(&filter_callbacks_, filter_->decoderFilterCallbacks());

  EXPECT_EQ(SipProxy::FilterStatus::Continue, filter_->transportBegin(request_metadata_));
  EXPECT_EQ(SipProxy::FilterStatus::Continue, filter_->messageBegin(request_metadata_));
  {
    std::string dummy_str = "dummy";
    EXPECT_EQ(SipProxy::FilterStatus::Continue, filter_->structBegin(dummy_str));
  }
  {
    std::string dummy_str = "dummy";
    SipProxy::FieldType dummy_ft{SipProxy::FieldType::I32};
    int16_t dummy_id{1};
    EXPECT_EQ(SipProxy::FilterStatus::Continue, filter_->fieldBegin(dummy_str, dummy_ft, dummy_id));
  }
  {
    bool dummy_val{false};
    EXPECT_EQ(SipProxy::FilterStatus::Continue, filter_->boolValue(dummy_val));
  }
  {
    uint8_t dummy_val{0};
    EXPECT_EQ(SipProxy::FilterStatus::Continue, filter_->byteValue(dummy_val));
  }
  {
    int16_t dummy_val{0};
    EXPECT_EQ(SipProxy::FilterStatus::Continue, filter_->int16Value(dummy_val));
  }
  {
    int32_t dummy_val{0};
    EXPECT_EQ(SipProxy::FilterStatus::Continue, filter_->int32Value(dummy_val));
  }
  {
    int64_t dummy_val{0};
    EXPECT_EQ(SipProxy::FilterStatus::Continue, filter_->int64Value(dummy_val));
  }
  {
    double dummy_val{0.0};
    EXPECT_EQ(SipProxy::FilterStatus::Continue, filter_->doubleValue(dummy_val));
  }
  {
    std::string dummy_str = "dummy";
    EXPECT_EQ(SipProxy::FilterStatus::Continue, filter_->stringValue(dummy_str));
  }
  {
    SipProxy::FieldType dummy_ft = SipProxy::FieldType::I32;
    uint32_t dummy_size{1};
    EXPECT_EQ(SipProxy::FilterStatus::Continue, filter_->mapBegin(dummy_ft, dummy_ft, dummy_size));
    EXPECT_EQ(SipProxy::FilterStatus::Continue, filter_->mapEnd());
  }
  {
    SipProxy::FieldType dummy_ft = SipProxy::FieldType::I32;
    uint32_t dummy_size{1};
    EXPECT_EQ(SipProxy::FilterStatus::Continue, filter_->listBegin(dummy_ft, dummy_size));
    EXPECT_EQ(SipProxy::FilterStatus::Continue, filter_->listEnd());
  }
  {
    SipProxy::FieldType dummy_ft = SipProxy::FieldType::I32;
    uint32_t dummy_size{1};
    EXPECT_EQ(SipProxy::FilterStatus::Continue, filter_->setBegin(dummy_ft, dummy_size));
    EXPECT_EQ(SipProxy::FilterStatus::Continue, filter_->setEnd());
  }
  EXPECT_EQ(SipProxy::FilterStatus::Continue, filter_->structEnd());
  EXPECT_EQ(SipProxy::FilterStatus::Continue, filter_->fieldEnd());
  EXPECT_EQ(SipProxy::FilterStatus::Continue, filter_->messageEnd());
  EXPECT_EQ(SipProxy::FilterStatus::Continue, filter_->transportEnd());

  EXPECT_NO_THROW(filter_->onDestroy());
}

} // namespace SipFilters
} // namespace SipProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
