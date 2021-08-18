#include "extensions/filters/network/sip_proxy/encoder.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace SipProxy {

static constexpr bool operator<(const Operation& o1, const Operation & o2) { return o1.position_ < o2.position_; }

void EncoderImpl::encode(const MessageMetadataSharedPtr& metadata, Buffer::Instance& out) {
  std::string raw_msg = metadata->rawMsg();

  std::sort(metadata->operation_list.begin(), metadata->operation_list.end());
  std::string msg = "";

  size_t previous_position = 0;
  for (auto& operation : metadata->operation_list) {
    msg += raw_msg.substr(previous_position, operation.position_ - previous_position);
    previous_position = operation.position_;

    switch (operation.type_) {
    case OperationType::Insert:
      msg += std::get<InsertOperationValue>(operation.value_).value_;
      break;
    case OperationType::Modify:
      msg += std::get<ModifyOperationValue>(operation.value_).dest_;
      previous_position += std::get<ModifyOperationValue>(operation.value_).src_length_;
      break;
    case OperationType::Delete:
      previous_position += std::get<DeleteOperationValue>(operation.value_).length_;
      break;
    default:
      break;
    }
  }

  msg += raw_msg.substr(previous_position);

//  std::string& msg = metadata->rawMsg();
//
//  if (metadata->methodType() == MethodType::Ok200) {
//    if (metadata->insertEPLocation().has_value() && metadata->insertTagLocation().has_value() &&
//        metadata->EP().has_value()) {
//      auto ep_insert_value = ";ep=" + std::string(metadata->EP().value());
//      if (metadata->insertEPLocation().value() < metadata->insertTagLocation().value()) {
//        msg.insert(metadata->insertEPLocation().value(), ep_insert_value);
//        msg.insert(metadata->insertTagLocation().value() + ep_insert_value.length(), ";tag");
//      } else {
//        msg.insert(metadata->insertTagLocation().value(), ";tag");
//        msg.insert(metadata->insertEPLocation().value(), ep_insert_value);
//      }
//    } else {
//      if (metadata->insertEPLocation().has_value() && metadata->EP().has_value()) {
//        msg.insert(metadata->insertEPLocation().value(),
//                   ";ep=" + std::string(metadata->EP().value()));
//      } else if (metadata->insertTagLocation().has_value()) {
//        msg.insert(metadata->insertTagLocation().value(), ";tag");
//      }
//    }
//  } else if (metadata->methodType() == MethodType::Invite &&
//             metadata->insertEPLocation().has_value() && metadata->EP().has_value()) {
//    msg.insert(metadata->insertEPLocation().value(), ";ep=" + std::string(metadata->EP().value()));
//  } else if ((metadata->methodType() == MethodType::Ack ||
//              metadata->methodType() == MethodType::Bye ||
//              metadata->methodType() == MethodType::Cancel) &&
//             metadata->insertTagLocation().has_value()) {
//    msg.insert(metadata->insertTagLocation().value(), ";tag");
//  }

  out.add(msg);
}

} // namespace SipProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
