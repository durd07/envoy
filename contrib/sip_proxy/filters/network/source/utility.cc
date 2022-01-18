#include "contrib/sip_proxy/filters/network/source/utility.h"
#include "contrib/sip_proxy/filters/network/source/filters/filter.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace SipProxy {

void PendingList::pushIntoPendingList(const std::string& type, const std::string& key,
                                      SipFilters::DecoderFilterCallbacks& activetrans,
                                      std::function<void(void)> func) {
  ENVOY_LOG(debug, "PUSH {}-{} {} into PendigList", type, key, activetrans.transactionId());
  if (activetrans.metadata()->queryMap()[type]) {
    if (pending_list_[type + key].empty()) {
      // need to do tra query
      func();
    }
    pending_list_[type + key].emplace_back(activetrans);
  } else {
    func();
    pending_list_[type + key].emplace_back(activetrans);
  }
}

// TODO this should be enhanced to save index in hash table keyed with
// transaction_id to improve search performace
void PendingList::eraseActiveTransFromPendingList(std::string& transaction_id) {
  ENVOY_LOG(debug, "POP {} from PendigList", transaction_id);
  for (auto& item : pending_list_) {
    for (auto it = item.second.begin(); it != item.second.end();) {
      if ((*it).get().transactionId() == transaction_id) {
        it = item.second.erase(it);
      } else {
        ++it;
      }
    }
  }
}

void PendingList::onResponseHandleForPendingList(const std::string& type, const std::string& key,
                                                 std::function<void(DecoderEventHandler&)> func) {
  for (auto& activetrans_ref : pending_list_[type + key]) {
    func(dynamic_cast<DecoderEventHandler&>(activetrans_ref.get()));
  }

  pending_list_[key].clear();
}
} // namespace SipProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
