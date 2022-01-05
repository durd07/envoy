#include "contrib/sip_proxy/filters/network/source/app_exception_impl.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace SipProxy {

DirectResponse::ResponseType AppException::encode(MessageMetadata& metadata,
                                                  Buffer::Instance& buffer) const {
  std::string output = "";

  // Topline
  output += "SIP/2.0 503 Service Unavaliable\r\n";

  // To
  output += "To: ";
  auto to = absl::get<StringHeader>(metadata.msgHeaderList()[HeaderType::To]);
  output += std::string(to);

  if (to.find("tag=") == absl::string_view::npos) {

    // We could simply use the time of day as a tag; however, that is not unique
    // enough.  So, let's perturb the time of day with a salt to get a better
    // unique number.  The salt I am using here is the summation of each
    // character of the proxy's IP address
    output += ";tag=";
    if (metadata.ep().has_value() && metadata.ep().value().length() > 0) {
      output += std::string(metadata.ep().value());
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
  output += std::string(absl::get<StringHeader>(metadata.msgHeaderList()[HeaderType::From]));
  output += "\r\n";

  // Call-ID
  output += "Call-ID: ";
  output += std::string(absl::get<StringHeader>(metadata.msgHeaderList()[HeaderType::CallId]));
  output += "\r\n";

  // Via
  for (auto via : absl::get<VectorHeader>(metadata.msgHeaderList()[HeaderType::Via])) {
    output += "Via: ";
    output += std::string(via);
    output += "\r\n";
  }

  // CSeq
  output += "CSeq: ";
  output += std::string(absl::get<StringHeader>(metadata.msgHeaderList()[HeaderType::Cseq]));
  output += "\r\n";

  // Content-length
  output += "Content-Length: 0";
  output += "\r\n";

  buffer.add(output);

  return DirectResponse::ResponseType::Exception;
}

} // namespace SipProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
