#ifndef RICS_MOSQUITOWRAPPER_H
#define RICS_MOSQUITOWRAPPER_H

#include <atomic>
#include <functional>
#include <memory>
#include <thread>
#include <vector>

#include "MqttMessage.h"
#include "rics_data_service/RicsDefines.h"

struct mosquitto;
struct mosquitto_message;

namespace rics {
class MosquitoWrapper {
 public:
  struct CallbackList {
    std::function<void(bool)> connectedCallback;
    std::function<void(const std::shared_ptr<MqttMessage>&)> receivedCallback;
    std::function<void(int)> sendCallback;
    std::function<void(int, const std::vector<int>&)> subscribedCallback;
  };

  explicit MosquitoWrapper(MqttOption option);

  virtual ~MosquitoWrapper();

  void Init(const CallbackList& callbacks);

  bool Send(const std::shared_ptr<MqttMessage>& pMessage);

  bool Subscribe(const std::string& topic, int qos);

 protected:
  static void OnConnected(mosquitto* mosHandle, void* pObj, int reasonCode);
  static void OnDisConnected(mosquitto* mosHandle, void* pObj, int reasonCode);
  static void OnReceived(mosquitto* mosHandle, void* pObj, const mosquitto_message* pMessage);
  static void OnSubscribed(mosquitto* mosHandle, void* pObj, int mid, int qosCount, const int* grantedQos);
  static void OnPublished(mosquitto* mosHandle, void* pObj, int mid);

  void StartLoop();

  // void
  // ShowSubscriberTopics();

 private:
  std::string GetErrMsg(int errCode);

 private:
  MqttOption m_option;
  mosquitto* m_pMosquittoHandle;
  CallbackList m_callbacks;
  // std::vector<std::string> m_subTopics;
  std::thread m_loopThread;
  std::atomic_bool m_bExitFlag;
};
}  // namespace rics

#endif  // RICS_MOSQUITOWRAPPER_H
