#pragma once

#include <chrono>
#include <memory>

#include "extensions/filters/network/sip_proxy/sip.h"

#include "absl/strings/string_view.h"
#include "absl/types/optional.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace SipProxy {

/**
 * MessageMetadata encapsulates metadata about Sip messages. The various fields are considered
 * optional since they may come from either the transport or protocol in some cases. Unless
 * otherwise noted, accessor methods throw absl::bad_optional_access if the corresponding value has
 * not been set.
 */
class MessageMetadata {
public:
  MessageMetadata(){};
  MessageMetadata(std::string&& raw_msg) : raw_msg_(std::move(raw_msg)) {}

  MsgType msgType() { return msg_type_; }
  MethodType methodType() { return method_type_; }
  MethodType respMethodType() { return resp_method_type_; }
  absl::optional<size_t> insertEPLocation() { return insert_ep_location_; }
  absl::optional<size_t> insertTagLocation() { return insert_tag_location_; }
  absl::optional<absl::string_view> EP() { return ep_; }

  absl::optional<absl::string_view> requestURI() { return request_uri_; }
  absl::optional<absl::string_view> topRoute() { return top_route_; }
  absl::optional<absl::string_view> domain() { return domain_; }
  absl::optional<absl::string_view> transactionId() { return transaction_id_; }
  absl::optional<absl::string_view> destination() { return destination_; }

  std::vector<Operation> operation_list;

  std::string& rawMsg() { return raw_msg_; }

  void setMsgType(MsgType data) { msg_type_ = data; }
  void setMethodType(MethodType data) { method_type_ = data; }
  void setRespMethodType(MethodType data) { resp_method_type_ = data; }
  void setInsertEPLocation(size_t data) {
          operation_list.emplace_back(Operation(OperationType::Insert, data, InsertOperationValue("")));
	  insert_ep_location_ = data;
  }
  void setInsertTagLocation(size_t data) { insert_tag_location_ = data; }
  void setEP(absl::string_view data) { ep_ = data; }

  void setRequestURI(absl::string_view data) { request_uri_ = data; }
  void setTopRoute(absl::string_view data) { top_route_ = data; }
  void setDomain(absl::string_view data) { domain_ = data; }

  // input is the full SIP header
  void setTransactionId(absl::string_view data)
  {
	  auto start_index = data.find("branch=");
	  if (start_index == absl::string_view::npos) {
		  return;
	  }
	  start_index += strlen("branch=");

	  auto end_index = data.find(";", start_index);
	  if (end_index == absl::string_view::npos) {
		  end_index = data.size();
	  }
	  transaction_id_ = data.substr(start_index, end_index - start_index);
  }

  void setDestination(absl::string_view destination) { destination_ = destination; }

private:
  MsgType msg_type_;
  MethodType method_type_;
  MethodType resp_method_type_;
  absl::optional<size_t> insert_ep_location_{};
  absl::optional<size_t> insert_tag_location_{};
  absl::optional<absl::string_view> ep_{};
  absl::optional<absl::string_view> pep_{};

  absl::optional<absl::string_view> request_uri_{};
  absl::optional<absl::string_view> top_route_{};
  absl::optional<absl::string_view> domain_{};
  absl::optional<absl::string_view> transaction_id_{};
  absl::optional<absl::string_view> destination_{};

  std::string raw_msg_{};
};

using MessageMetadataSharedPtr = std::shared_ptr<MessageMetadata>;

} // namespace SipProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
