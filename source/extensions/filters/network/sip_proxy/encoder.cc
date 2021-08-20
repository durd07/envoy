#include "source/extensions/filters/network/sip_proxy/encoder.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace SipProxy {

//static constexpr bool operator<(const Operation& o1, const Operation & o2) { return o1.position_ < o2.position_; }

void EncoderImpl::encode(const MessageMetadataSharedPtr& metadata, Buffer::Instance& out) {
  std::string output = "";
  std::string& raw_msg = metadata->rawMsg();
  std::sort(metadata->operationList().begin(), metadata->operationList().end());

  size_t previous_position = 0;
  for (auto& operation : metadata->operationList()) {
    output += raw_msg.substr(previous_position, operation.position_ - previous_position);
    previous_position = operation.position_;

    switch (operation.type_) {
    case OperationType::Insert:
      output += std::get<InsertOperationValue>(operation.value_).value_;
      break;
    case OperationType::Modify:
      output += std::get<ModifyOperationValue>(operation.value_).dest_;
      previous_position += std::get<ModifyOperationValue>(operation.value_).src_length_;
      break;
    case OperationType::Delete:
      previous_position += std::get<DeleteOperationValue>(operation.value_).length_;
      break;
    default:
      break;
    }
  }

  output += raw_msg.substr(previous_position);

  out.add(output);
}

} // namespace SipProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
