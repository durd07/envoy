#pragma once

#include "envoy/router/router.h"

#include "source/extensions/filters/network/sip_proxy/conn_manager.h"
#include "source/extensions/filters/network/sip_proxy/conn_state.h"
#include "source/extensions/filters/network/sip_proxy/filters/factory_base.h"
#include "source/extensions/filters/network/sip_proxy/filters/filter.h"
#include "source/extensions/filters/network/sip_proxy/metadata.h"
#include "source/extensions/filters/network/sip_proxy/protocol.h"
#include "source/extensions/filters/network/sip_proxy/router/router.h"

#include "test/mocks/network/mocks.h"
#include "test/mocks/stream_info/mocks.h"
#include "test/test_common/printers.h"

#include "gmock/gmock.h"

using testing::NiceMock;

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace SipProxy {

class MockConfig : public Config {
public:
  MockConfig();
  ~MockConfig() override;

  // SipProxy::Config
  MOCK_METHOD(SipFilters::FilterChainFactory&, filterFactory, ());
  MOCK_METHOD(SipFilterStats&, stats, ());
  MOCK_METHOD(DecoderPtr, createDecoder, (DecoderCallbacks&));
  MOCK_METHOD(Router::Config&, routerConfig, ());
};

////class MockTransport : public Transport {
////public:
////  MockTransport();
////  ~MockTransport() override;
////
////  // SipProxy::Transport
////  MOCK_METHOD(const std::string&, name, (), (const));
////  MOCK_METHOD(TransportType, type, (), (const));
////  MOCK_METHOD(bool, decodeFrameStart, (Buffer::Instance&, MessageMetadata&));
////  MOCK_METHOD(bool, decodeFrameEnd, (Buffer::Instance&));
////  MOCK_METHOD(void, encodeFrame, (Buffer::Instance&, const MessageMetadata&,
/// Buffer::Instance&));
////
////  std::string name_{"mock"};
////  TransportType type_{TransportType::Auto};
////};
////
////class MockProtocol : public Protocol {
////public:
////  MockProtocol();
////  ~MockProtocol() override;
////
////  // SipProxy::Protocol
////  MOCK_METHOD(const std::string&, name, (), (const));
////  MOCK_METHOD(ProtocolType, type, (), (const));
////  MOCK_METHOD(void, setType, (ProtocolType));
////  MOCK_METHOD(bool, readMessageBegin, (Buffer::Instance & buffer, MessageMetadata& metadata));
////  MOCK_METHOD(bool, readMessageEnd, (Buffer::Instance & buffer));
////  MOCK_METHOD(bool, readStructBegin, (Buffer::Instance & buffer, std::string& name));
////  MOCK_METHOD(bool, readStructEnd, (Buffer::Instance & buffer));
////  MOCK_METHOD(bool, readFieldBegin,
////              (Buffer::Instance & buffer, std::string& name, FieldType& field_type,
////               int16_t& field_id));
////  MOCK_METHOD(bool, readFieldEnd, (Buffer::Instance & buffer));
////  MOCK_METHOD(bool, readMapBegin,
////              (Buffer::Instance & buffer, FieldType& key_type, FieldType& value_type,
////               uint32_t& size));
////  MOCK_METHOD(bool, readMapEnd, (Buffer::Instance & buffer));
////  MOCK_METHOD(bool, readListBegin,
////              (Buffer::Instance & buffer, FieldType& elem_type, uint32_t& size));
////  MOCK_METHOD(bool, readListEnd, (Buffer::Instance & buffer));
////  MOCK_METHOD(bool, readSetBegin,
////              (Buffer::Instance & buffer, FieldType& elem_type, uint32_t& size));
////  MOCK_METHOD(bool, readSetEnd, (Buffer::Instance & buffer));
////  MOCK_METHOD(bool, readBool, (Buffer::Instance & buffer, bool& value));
////  MOCK_METHOD(bool, readByte, (Buffer::Instance & buffer, uint8_t& value));
////  MOCK_METHOD(bool, readInt16, (Buffer::Instance & buffer, int16_t& value));
////  MOCK_METHOD(bool, readInt32, (Buffer::Instance & buffer, int32_t& value));
////  MOCK_METHOD(bool, readInt64, (Buffer::Instance & buffer, int64_t& value));
////  MOCK_METHOD(bool, readDouble, (Buffer::Instance & buffer, double& value));
////  MOCK_METHOD(bool, readString, (Buffer::Instance & buffer, std::string& value));
////  MOCK_METHOD(bool, readBinary, (Buffer::Instance & buffer, std::string& value));
////
////  MOCK_METHOD(void, writeMessageBegin,
////              (Buffer::Instance & buffer, const MessageMetadata& metadata));
////  MOCK_METHOD(void, writeMessageEnd, (Buffer::Instance & buffer));
////  MOCK_METHOD(void, writeStructBegin, (Buffer::Instance & buffer, const std::string& name));
////  MOCK_METHOD(void, writeStructEnd, (Buffer::Instance & buffer));
////  MOCK_METHOD(void, writeFieldBegin,
////              (Buffer::Instance & buffer, const std::string& name, FieldType field_type,
////               int16_t field_id));
////  MOCK_METHOD(void, writeFieldEnd, (Buffer::Instance & buffer));
////  MOCK_METHOD(void, writeMapBegin,
////              (Buffer::Instance & buffer, FieldType key_type, FieldType value_type, uint32_t
/// size)); /  MOCK_METHOD(void, writeMapEnd, (Buffer::Instance & buffer)); /  MOCK_METHOD(void,
/// writeListBegin, /              (Buffer::Instance & buffer, FieldType elem_type, uint32_t size));
////  MOCK_METHOD(void, writeListEnd, (Buffer::Instance & buffer));
////  MOCK_METHOD(void, writeSetBegin, (Buffer::Instance & buffer, FieldType elem_type, uint32_t
/// size)); /  MOCK_METHOD(void, writeSetEnd, (Buffer::Instance & buffer)); /  MOCK_METHOD(void,
/// writeBool, (Buffer::Instance & buffer, bool value)); /  MOCK_METHOD(void, writeByte,
///(Buffer::Instance & buffer, uint8_t value)); /  MOCK_METHOD(void, writeInt16, (Buffer::Instance &
/// buffer, int16_t value)); /  MOCK_METHOD(void, writeInt32, (Buffer::Instance & buffer, int32_t
/// value)); /  MOCK_METHOD(void, writeInt64, (Buffer::Instance & buffer, int64_t value)); /
/// MOCK_METHOD(void, writeDouble, (Buffer::Instance & buffer, double value)); /  MOCK_METHOD(void,
/// writeString, (Buffer::Instance & buffer, const std::string& value)); /  MOCK_METHOD(void,
/// writeBinary, (Buffer::Instance & buffer, const std::string& value)); /  MOCK_METHOD(bool,
/// supportsUpgrade, ()); /  MOCK_METHOD(DecoderEventHandlerSharedPtr, upgradeRequestDecoder, ()); /
/// MOCK_METHOD(DirectResponsePtr, upgradeResponse, (const DecoderEventHandler&)); /
/// MOCK_METHOD(SipObjectPtr, attemptUpgrade, (Transport&, SipConnectionState&, Buffer::Instance&));
////  MOCK_METHOD(void, completeUpgrade, (SipConnectionState&, SipObject&));
////
////  std::string name_{"mock"};
////  ProtocolType type_{ProtocolType::Auto};
////};
//
class MockDecoderEventHandler : public DecoderEventHandler {
public:
  MockDecoderEventHandler();
  ~MockDecoderEventHandler() override;

  // SipProxy::DecoderEventHandler
  MOCK_METHOD(FilterStatus, transportBegin, (MessageMetadataSharedPtr metadata));
  MOCK_METHOD(FilterStatus, transportEnd, ());
  MOCK_METHOD(FilterStatus, messageBegin, (MessageMetadataSharedPtr metadata));
  MOCK_METHOD(FilterStatus, messageEnd, ());
};

class MockDecoderCallbacks : public DecoderCallbacks {
public:
  MockDecoderCallbacks();
  ~MockDecoderCallbacks() override;

  // SipProxy::DecoderCallbacks
  MOCK_METHOD(DecoderEventHandler&, newDecoderEventHandler, (MessageMetadataSharedPtr));
};

class MockDirectResponse : public DirectResponse {
public:
  MockDirectResponse();
  ~MockDirectResponse() override;

  // SipProxy::DirectResponse
  MOCK_METHOD(DirectResponse::ResponseType, encode,
              (MessageMetadata & metadata, Buffer::Instance& buffer), (const));
};
//
////class MockSipObject : public SipObject {
////public:
////  MockSipObject();
////  ~MockSipObject() override;
////
////  MOCK_METHOD(SipFieldPtrList&, fields, (), (const));
////  MOCK_METHOD(bool, onData, (Buffer::Instance&));
////};
//
namespace Router {
class MockRoute;
} // namespace Router
//
namespace SipFilters {

// class MockFilterChainFactoryCallbacks : public FilterChainFactoryCallbacks {
// public:
//  MockFilterChainFactoryCallbacks();
//  ~MockFilterChainFactoryCallbacks() override;
//
//  MOCK_METHOD(void, addDecoderFilter, (DecoderFilterSharedPtr));
//};
//
class MockDecoderFilter : public DecoderFilter {
public:
  MockDecoderFilter();
  ~MockDecoderFilter() override;

  // SipProxy::SipFilters::DecoderFilter
  MOCK_METHOD(void, onDestroy, ());
  MOCK_METHOD(void, setDecoderFilterCallbacks, (DecoderFilterCallbacks & callbacks));
  MOCK_METHOD(void, resetUpstreamConnection, ());
  MOCK_METHOD(bool, passthroughSupported, (), (const));

  // SipProxy::DecoderEventHandler
  MOCK_METHOD(FilterStatus, passthroughData, (Buffer::Instance & data));
  MOCK_METHOD(FilterStatus, transportBegin, (MessageMetadataSharedPtr metadata));
  MOCK_METHOD(FilterStatus, transportEnd, ());
  MOCK_METHOD(FilterStatus, messageBegin, (MessageMetadataSharedPtr metadata));
  MOCK_METHOD(FilterStatus, messageEnd, ());
  //  MOCK_METHOD(FilterStatus, structBegin, (absl::string_view name));
  //  MOCK_METHOD(FilterStatus, structEnd, ());
  //  MOCK_METHOD(FilterStatus, fieldBegin,
  //              (absl::string_view name, FieldType& msg_type, int16_t& field_id));
  //  MOCK_METHOD(FilterStatus, fieldEnd, ());
  //  MOCK_METHOD(FilterStatus, boolValue, (bool& value));
  //  MOCK_METHOD(FilterStatus, byteValue, (uint8_t & value));
  //  MOCK_METHOD(FilterStatus, int16Value, (int16_t & value));
  //  MOCK_METHOD(FilterStatus, int32Value, (int32_t & value));
  //  MOCK_METHOD(FilterStatus, int64Value, (int64_t & value));
  //  MOCK_METHOD(FilterStatus, doubleValue, (double& value));
  //  MOCK_METHOD(FilterStatus, stringValue, (absl::string_view value));
  //  MOCK_METHOD(FilterStatus, mapBegin,
  //              (FieldType & key_type, FieldType& value_type, uint32_t& size));
  //  MOCK_METHOD(FilterStatus, mapEnd, ());
  //  MOCK_METHOD(FilterStatus, listBegin, (FieldType & elem_type, uint32_t& size));
  //  MOCK_METHOD(FilterStatus, listEnd, ());
  //  MOCK_METHOD(FilterStatus, setBegin, (FieldType & elem_type, uint32_t& size));
  //  MOCK_METHOD(FilterStatus, setEnd, ());
};

class MockDecoderFilterCallbacks : public DecoderFilterCallbacks {
public:
  MockDecoderFilterCallbacks();
  ~MockDecoderFilterCallbacks() override;

  // SipProxy::SipFilters::DecoderFilterCallbacks
  MOCK_METHOD(uint64_t, streamId, (), (const));
  MOCK_METHOD(std::string, transactionId, (), (const));
  MOCK_METHOD(const Network::Connection*, connection, (), (const));
  MOCK_METHOD(Router::RouteConstSharedPtr, route, ());
  // MOCK_METHOD(std::shared_ptr<Router::TransactionInfos>, transaction_infos, ());
  MOCK_METHOD(Event::Dispatcher&, dispatcher, ());
  //  MOCK_METHOD(void, continueDecoding, ());
  //  MOCK_METHOD(Router::RouteConstSharedPtr, route, ());
  ////  MOCK_METHOD(TransportType, downstreamTransportType, (), (const));
  ////  MOCK_METHOD(ProtocolType, downstreamProtocolType, (), (const));
  MOCK_METHOD(void, sendLocalReply, (const DirectResponse&, bool));
  MOCK_METHOD(void, startUpstreamResponse, ());
  MOCK_METHOD(ResponseStatus, upstreamData, (MessageMetadataSharedPtr));
  MOCK_METHOD(void, resetDownstreamConnection, ());
  MOCK_METHOD(StreamInfo::StreamInfo&, streamInfo, ());
  MOCK_METHOD(std::shared_ptr<Router::TransactionInfos>, transactionInfos, ());
  MOCK_METHOD(std::shared_ptr<SipProxy::SipSettings>, settings, ());
  MOCK_METHOD(void, onReset, ());
  MOCK_METHOD(MessageMetadataSharedPtr, responseMetadata, ());
  MOCK_METHOD(bool, responseSuccess, ());

  uint64_t stream_id_{1};
  std::string transaction_id_{"test"};
  NiceMock<Network::MockConnection> connection_;
  NiceMock<StreamInfo::MockStreamInfo> stream_info_;
  //  MessageMetadataSharedPtr metadata_;
  std::shared_ptr<Router::MockRoute> route_;
  // using MockTransactionInfos = std::map<std::string, std::shared_ptr<MockTransactionInfo>>;
  std::shared_ptr<Router::TransactionInfos> transaction_infos_;
};

class MockFilterConfigFactory : public NamedSipFilterConfigFactory {
public:
  MockFilterConfigFactory();
  ~MockFilterConfigFactory() override;

  FilterFactoryCb
  createFilterFactoryFromProto(const Protobuf::Message& proto_config,
                               const std::string& stats_prefix,
                               Server::Configuration::FactoryContext& context) override;

  ProtobufTypes::MessagePtr createEmptyConfigProto() override {
    return std::make_unique<ProtobufWkt::Struct>();
  }

  std::string name() const override { return name_; }

  ProtobufWkt::Struct config_struct_;
  std::string config_stat_prefix_;

private:
  std::shared_ptr<MockDecoderFilter> mock_filter_;
  const std::string name_;
};

} // namespace SipFilters
//
namespace Router {
//
////class MockRateLimitPolicyEntry : public RateLimitPolicyEntry {
////public:
////  MockRateLimitPolicyEntry();
////  ~MockRateLimitPolicyEntry() override;
////
////  MOCK_METHOD(uint32_t, stage, (), (const));
////  MOCK_METHOD(const std::string&, disableKey, (), (const));
////  MOCK_METHOD(void, populateDescriptors,
////              (const RouteEntry&, std::vector<RateLimit::Descriptor>&, const std::string&,
////               const MessageMetadata&, const Network::Address::Instance&),
////              (const));
////
////  std::string disable_key_;
////};
//
////class MockRateLimitPolicy : public RateLimitPolicy {
////public:
////  MockRateLimitPolicy();
////  ~MockRateLimitPolicy() override;
////
////  MOCK_METHOD(bool, empty, (), (const));
////  MOCK_METHOD(const std::vector<std::reference_wrapper<const RateLimitPolicyEntry>>&,
////              getApplicableRateLimit, (uint32_t), (const));
////
////  std::vector<std::reference_wrapper<const RateLimitPolicyEntry>> rate_limit_policy_entry_;
////};
//
class MockRouteEntry : public RouteEntry {
public:
  MockRouteEntry();
  ~MockRouteEntry() override;

  //  // SipProxy::Router::RouteEntry
  MOCK_METHOD(const std::string&, clusterName, (), (const));
  MOCK_METHOD(const Envoy::Router::MetadataMatchCriteria*, metadataMatchCriteria, (), (const));
  //  MOCK_METHOD(const Envoy::Router::TlsContextMatchCriteria*, tlsContextMatchCriteria, (),
  //  (const));
  //  //MOCK_METHOD(RateLimitPolicy&, rateLimitPolicy, (), (const));
  //  MOCK_METHOD(bool, stripServiceName, (), (const));
  //  MOCK_METHOD(const Http::LowerCaseString&, clusterHeader, (), (const));
  //
  std::string cluster_name_{"fake_cluster"};
  //  Http::LowerCaseString cluster_header_{""};
  //  //NiceMock<MockRateLimitPolicy> rate_limit_policy_;
};
//
class MockRoute : public Route {
public:
  MockRoute();
  ~MockRoute() override;

  // SipProxy::Router::Route
  MOCK_METHOD(const RouteEntry*, routeEntry, (), (const));

  NiceMock<MockRouteEntry> route_entry_;
};

} // namespace Router
} // namespace SipProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
