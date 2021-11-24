#pragma once

#include <chrono>
#include <memory>
#include <utility>

#include "common/common/assert.h"
#include "common/common/logger.h"

#include "extensions/filters/network/sip_proxy/operation.h"
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
class MessageMetadata : public Logger::Loggable<Logger::Id::filter> {
public:
  MessageMetadata(){};
  MessageMetadata(std::string&& raw_msg) : raw_msg_(std::move(raw_msg)) {}

  MsgType msgType() { return msg_type_; }
  MethodType methodType() { return method_type_; }
  MethodType respMethodType() { return resp_method_type_; }
  absl::optional<absl::string_view> EP() { return ep_; }
  std::vector<Operation>& operationList() { return operation_list_; }
  absl::optional<std::pair<std::string, std::string>> pCookieIpMap() { return p_cookie_ip_map_; }

  absl::optional<absl::string_view> requestURI() { return request_uri_; }
  absl::optional<absl::string_view> topRoute() { return top_route_; }
  absl::optional<absl::string_view> domain() { return domain_; }
  absl::optional<absl::string_view> transactionId() { return transaction_id_; }
  absl::optional<absl::string_view> destination() { return destination_; }
  std::map<std::string, std::string>& paramMap() {return param_map_;};
  std::map<std::string, std::string>& destinationMap() { return destination_map_; }
  std::map<std::string, bool>& queryMap() { return query_map_; }
  std::map<std::string, bool>& subscribeMap() { return subscribe_map_; }
  void addDestination(std::string param, std::string value) { destination_map_[param] = value;}
  void addParam(std::string param, std::string value) { param_map_[param] = value; }
  void addQuery(std::string param, bool value) { query_map_[param] = value; }
  void addSubscribe(std::string param, bool value) { subscribe_map_[param] = value; }

  std::string& rawMsg() { return raw_msg_; }

  void setMsgType(MsgType data) { msg_type_ = data; }
  void setMethodType(MethodType data) { method_type_ = data; }
  void setRespMethodType(MethodType data) { resp_method_type_ = data; }
  void setOperation(Operation op) { operation_list_.emplace_back(op); }
  void setEP(absl::string_view data) { ep_ = data; }
  void setPCookieIpMap(std::pair<std::string, std::string> && data) { p_cookie_ip_map_ = data; }

  void setRequestURI(absl::string_view data) { request_uri_ = data; }
  void setTopRoute(absl::string_view data) { top_route_ = data; }
  void setDomain(absl::string_view header, std::string domainMatchParamName) {
    domain_ = getDomain(header, domainMatchParamName);
  }

  void addEPOperation(size_t rawOffset, absl::string_view& header, std::string ownDomain,
                      std::string domainMatchParamName) {
    if (header.find(";ep=") != absl::string_view::npos) {
      // already Contact have ep
      return;
    }
    auto pos = header.find(">");
    if (pos == absl::string_view::npos) {
      // no url
      return;
    }
    // Get domain
    absl::string_view domain = getDomain(header, domainMatchParamName);

    // Compare the domain
    if (domain != ownDomain) {
      ENVOY_LOG(trace, "header {} domain:{} is not equal to own_domain:{}, don't add EP.", header, domain, ownDomain);
      return;
    }

    setOperation(Operation(OperationType::Insert, rawOffset + pos, InsertOperationValue(";ep=")));
  }

  void addOpaqueOperation(size_t rawOffset, absl::string_view& header) {
    if (header.find(",opaque=") != absl::string_view::npos) {
      // already has opaque
      return;
    }
    auto pos = header.length();
    setOperation(
        Operation(OperationType::Insert, rawOffset + pos, InsertOperationValue(",opaque=")));
  }

  void deleteInstipOperation(size_t rawOffset, absl::string_view& header) {
    // Delete inst-ip and remove "sip:" in x-suri
    if (auto pos = header.find(";inst-ip="); pos != absl::string_view::npos) {
      setOperation(
          Operation(OperationType::Delete, rawOffset + pos,
                    DeleteOperationValue(
                        header.substr(pos, header.find_first_of(";>", pos + 1) - pos).size())));
      // auto xsuri = header.find("sip:pcsf-cfed");
      auto xsuri = header.find("x-suri=sip:");
      if (xsuri != absl::string_view::npos) {
        setOperation(Operation(OperationType::Delete, rawOffset + xsuri + strlen("x-suri="), DeleteOperationValue(4)));
      }
    }
  }

  // input is the full SIP header
  void setTransactionId(absl::string_view data) {
    auto start_index = data.find("branch=");
    if (start_index == absl::string_view::npos) {
      return;
    }
    start_index += strlen("branch=");

    auto end_index = data.find_first_of(";>", start_index);
    if (end_index == absl::string_view::npos) {
      end_index = data.size();
    }
    transaction_id_ = data.substr(start_index, end_index - start_index);
  }

  void setDestination(absl::string_view destination) { destination_ = destination; }
  void resetDestination() { destination_.reset(); }
  /*only used in UT*/
  void resetTransactionId() { transaction_id_.reset(); }

  std::map<std::string, std::string>::iterator destIter;

private:
  MsgType msg_type_;
  MethodType method_type_;
  MethodType resp_method_type_;
  std::vector<Operation> operation_list_;
  absl::optional<absl::string_view> ep_{};
  absl::optional<absl::string_view> pep_{};
  absl::optional<absl::string_view> route_ep_{};
  absl::optional<absl::string_view> route_opaque_{};

  absl::optional<std::pair<std::string, std::string>> p_cookie_ip_map_{};

  absl::optional<absl::string_view> request_uri_{};
  absl::optional<absl::string_view> top_route_{};
  absl::optional<absl::string_view> domain_{};
  absl::optional<absl::string_view> transaction_id_{};
  absl::optional<absl::string_view> destination_{};
  // Params get from Top Route header
  std::map<std::string, std::string> param_map_{};
  // Destination get from param_map_ ordered by CustomizedAffinity, not queried
  std::map<std::string, std::string> destination_map_{};
  // Could do remote query for this param
  std::map<std::string, bool> query_map_{};
  // Could do remote subscribe for this param
  std::map<std::string, bool> subscribe_map_{};

  std::string raw_msg_{};

  absl::string_view getDomain(absl::string_view header, std::string domainMatchParamName) {
    // ENVOY_LOG(error, "header: {}\ndomainMatchParamName: {}", header, domainMatchParamName);

    // Get domain
    absl::string_view domain = "";

    if (domainMatchParamName != "host") {
      auto start = header.find(domainMatchParamName);
      if (start == absl::string_view::npos) {
        domain = "";
      } else {
        // domainMatchParamName + "="
        // start = start + strlen(domainMatchParamName.c_str()) + strlen("=") ;
        start = start + domainMatchParamName.length() + strlen("=");
        if ("sip:" == header.substr(start, strlen("sip:"))) {
          start += strlen("sip:");
        }
        // end
        auto end = header.find_first_of(":;>", start);
        if (end == absl::string_view::npos) {
          domain = "";
        } else {
          domain = header.substr(start, end - start);
        }
      }
    }

    // Still get host if mapped domain is empty
    if (domainMatchParamName == "host" || domain == "") {
      auto start = header.find("sip:");
      if (start == absl::string_view::npos) {
        return "";
      }
      start += strlen("sip:");
      auto end = header.find_first_of(":;>", start);
      if (end == absl::string_view::npos) {
        return "";
      }

      auto addr = header.substr(start, end - start);

      // Remove name in format of sip:name@addr:pos
      auto pos = addr.find("@");
      if (pos == absl::string_view::npos) {
        domain = header.substr(start, end - start);
      } else {
        pos += strlen("@");
        domain = addr.substr(pos, addr.length() - pos);
      }
    }

    return domain;
  }
};

using MessageMetadataSharedPtr = std::shared_ptr<MessageMetadata>;

} // namespace SipProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
