#pragma once

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <variant>
#include <vector>

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace SipProxy {

enum class HeaderType {
  TopLine,
  CallId,
  Via,
  To,
  From,
  Route,
  Contact,
  RRoute,
  Cseq,
  Path,
  Event,
  Other,
  InvalidFormat
};

enum class MsgType { Request, Response, ErrorMsg };

enum class MethodType {
  Invite,
  Register,
  Update,
  Refer,
  Subscribe,
  Notify,
  Ack,
  Bye,
  Cancel,
  Ok200,
  NullMethod
};

enum class AppExceptionType {
  Unknown = 0,
  UnknownMethod = 1,
  InvalidMessageType = 2,
  WrongMethodName = 3,
  BadSequenceId = 4,
  MissingResult = 5,
  InternalError = 6,
  ProtocolError = 7,
  InvalidTransform = 8,
  InvalidProtocol = 9,
  // FBThrift values.
  // See https://github.com/facebook/fbthrift/blob/master/thrift/lib/cpp/TApplicationException.h#L52
  UnsupportedClientType = 10,
  LoadShedding = 11,
  Timeout = 12,
  InjectedFailure = 13,
  ChecksumMismatch = 14,
  Interruption = 15,
};

enum class OperationType {
  Invalid = 0,
  Insert = 1,
  Delete = 2,
  Modify = 3,
  Query = 4, // not used yet.
};

struct InsertOperationValue {
  InsertOperationValue(std::string&& value) : value_(value) {}
  std::string value_;
};
struct DeleteOperationValue {
  DeleteOperationValue(size_t length) : length_(length) {}
  size_t length_;
};
struct ModifyOperationValue {
  ModifyOperationValue(size_t src_length, std::string&& dest)
      : src_length_(src_length), dest_(dest) {}
  size_t src_length_;
  std::string dest_;
};

class Operation {
public:
  Operation(OperationType type, size_t position,
            std::variant<InsertOperationValue, DeleteOperationValue, ModifyOperationValue> value)
      : type_(type), position_(position), value_(value) {}

  // constexpr bool operator<(const Operation& other) { return this->position_ < other.position_; }
  // constexpr bool operator>(const Operation& other) { return this->position_ > other.position_; }
  // constexpr bool operator==(const Operation& other) { return this->position_ == other.position_; }
  // constexpr bool operator!=(const Operation& other) { return this->position_ != other.position_; }
  // constexpr bool operator<=(const Operation& other) { return this->position_ <= other.position_; }
  // constexpr bool operator>=(const Operation& other) { return this->position_ >= other.position_; }
  // constexpr bool operator<=>(Operation &other) { return this->position_ <=> other.position_; }

  // private:
  OperationType type_;
  size_t position_;
  std::variant<InsertOperationValue, DeleteOperationValue, ModifyOperationValue> value_;
};

} // namespace SipProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
