#include "extensions/filters/network/sip_proxy/encoder.h"

#include "algorithm"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace SipProxy {
void EncoderImpl::encode(const MessageMetadataSharedPtr& metadata, Buffer::Instance& out) {
  std::string output = "";
  std::string& raw_msg = metadata->rawMsg();
  std::sort(metadata->operationList().begin(), metadata->operationList().end());

  size_t previous_position = 0;
  for (auto& operation : metadata->operationList()) {
    switch (operation.type_) {
    case OperationType::Insert: {
      std::string value = std::get<InsertOperationValue>(operation.value_).value_;
      if (value == ";ep=" || value == ",opaque=") {
        if (metadata->EP().has_value() && metadata->EP().value().length() > 0) {
          output += raw_msg.substr(previous_position, operation.position_ - previous_position);
          previous_position = operation.position_;

          output += std::get<InsertOperationValue>(operation.value_).value_;
	  /* Base64
          if (value == ",opaque=") {
            output += "\"";
          }
          auto str = Base64::encode(metadata->EP().value().data(), metadata->EP()->length());
          auto pos = str.find("=");
          while (pos != absl::string_view::npos) {
            str.replace(pos, strlen("="), "%3D");
            pos = str.find("=");
          }
          output += str;
          if (value == ",opaque=") {
            output += "\"";
          }*/
          if (value == ",opaque=") {
            output += "\"";
	  }
          output +=  metadata->EP().value();
          if (value == ",opaque=") {
            output += "\"";
	  }
        }
      } else {
        output += raw_msg.substr(previous_position, operation.position_ - previous_position);
        previous_position = operation.position_;

        output += std::get<InsertOperationValue>(operation.value_).value_;
      }
      break;
    }
    case OperationType::Modify:
      output += raw_msg.substr(previous_position, operation.position_ - previous_position);
      previous_position = operation.position_;

      output += std::get<ModifyOperationValue>(operation.value_).dest_;
      previous_position += std::get<ModifyOperationValue>(operation.value_).src_length_;
      break;
    case OperationType::Delete:
      output += raw_msg.substr(previous_position, operation.position_ - previous_position);
      previous_position = operation.position_;

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
