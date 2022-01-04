#include "contrib/sip_proxy/filters/network/source/encoder.h"

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
      std::string value = absl::get<InsertOperationValue>(operation.value_).value_;
      if (value == ";ep=" || value == ",opaque=") {
        if (metadata->ep().has_value() && metadata->ep().value().length() > 0) {
          output += raw_msg.substr(previous_position, operation.position_ - previous_position);
          previous_position = operation.position_;

          output += absl::get<InsertOperationValue>(operation.value_).value_;
          if (value == ",opaque=") {
            output += "\"";
          }
          output += std::string(metadata->ep().value());
          if (value == ",opaque=") {
            output += "\"";
          }
        }
      } else {
        output += raw_msg.substr(previous_position, operation.position_ - previous_position);
        previous_position = operation.position_;

        output += absl::get<InsertOperationValue>(operation.value_).value_;
      }
      break;
    }
    case OperationType::Modify:
      output += raw_msg.substr(previous_position, operation.position_ - previous_position);
      previous_position = operation.position_;

      output += absl::get<ModifyOperationValue>(operation.value_).dest_;
      previous_position += absl::get<ModifyOperationValue>(operation.value_).src_length_;
      break;
    case OperationType::Delete:
      output += raw_msg.substr(previous_position, operation.position_ - previous_position);
      previous_position = operation.position_;

      previous_position += absl::get<DeleteOperationValue>(operation.value_).length_;
      break;
    default:
      break;
    }
  }

  output += raw_msg.substr(previous_position);
  out.add(output);
}

void EncoderImpl::createFinalResponse(const MessageMetadataSharedPtr& metadata, std::string rCode,
                                      std::string reason, Buffer::Instance& out) {
  std::string output = "";

  // Topline
  output += "SIP/2.0 ";
  output += rCode; // 503
  output += " ";
  output += reason; //"Service Unavaliable"
  output += "\r\n";

  // To
  output += "To: ";
  auto to = absl::get<StringHeader>(metadata->msgHeaderList()[HeaderType::To]);
  output += std::string(to);

  if (to.find("tag=") == absl::string_view::npos) {

    // We could simply use the time of day as a tag; however, that is not unique
    // enough.  So, let's perturb the time of day with a salt to get a better
    // unique number.  The salt I am using here is the summation of each
    // character of the proxy's IP address
    output += ";tag=";
    if (metadata->ep().has_value() && metadata->ep().value().length() > 0) {
      output += std::string(metadata->ep().value());
    }
    std::time_t t;
    long s = 0;
    char buf[80];
    t = time(&t);
    s = std::labs(t - s);
    sprintf(buf, "%lx", s);
    output += buf;
  }
  output += "\r\n";

  // From
  output += "From: ";
  output += std::string(absl::get<StringHeader>(metadata->msgHeaderList()[HeaderType::From]));
  output += "\r\n";

  // Call-ID
  output += "Call-ID: ";
  output += std::string(absl::get<StringHeader>(metadata->msgHeaderList()[HeaderType::CallId]));
  output += "\r\n";

  // Via
  for (auto via : absl::get<VectorHeader>(metadata->msgHeaderList()[HeaderType::Via])) {
    output += "Via: ";
    output += std::string(via);
    output += "\r\n";
  }

  // CSeq
  output += "CSeq: ";
  output += std::string(absl::get<StringHeader>(metadata->msgHeaderList()[HeaderType::Cseq]));
  output += "\r\n";

  // Content-length
  output += "Content-Length: 0";
  output += "\r\n";

  out.add(output);
}

} // namespace SipProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
