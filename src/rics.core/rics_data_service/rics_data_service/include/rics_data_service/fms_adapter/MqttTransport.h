#ifndef RICS_MQTTTRANSPORT_H
#define RICS_MQTTTRANSPORT_H

#include <mutex>
#include <set>
#include <vector>
#include <string> // 确保 std::string 可用
#include "MosquitoWrapper.h"
#include "MqttContext.h"
#include "MqttMessage.h"
#include "rics_data_service/ITransport.h"
#include "rics_data_service/RicsDefines.h"

namespace rics {

class IListenner {
public:
    virtual ~IListenner() = default;
    virtual bool MessageCheck(const std::string& topic) = 0;
    virtual void OnRecvMsgHandle(const std::string& topic, const std::string& payload) = 0;
    
    // 新增：由监听者提供需要订阅的话题列表
    virtual std::vector<std::string> GetSubscribeTopics() = 0;
};

class MqttTransport : public ITransport {
 public:
  explicit MqttTransport(const std::vector<MqttOption>& options,
                         const std::shared_ptr<MqttContext>& pContext);

  bool Send(const std::string& strData) override;
  bool Send(const std::string& strData, int& msgId) override;
  bool ConfirmLastData(int msgId) override;
  std::shared_ptr<ITransportContext> GetContext() override;
  bool IsConnected() { return m_bIsConnected; }

  void RegisterRecvCallback(const ReceiveCallback& callback);
  void RegisterListeners(IListenner* listener);
  void DelListeners(IListenner* listener);

 private:
  void Connected(bool isConnected);
  void OnReceived(const std::shared_ptr<MqttMessage>& pMessage);
  void OnSend(int messageId);
  void OnSubscribed(int mid, const std::vector<int>& qos);

 private:
  std::set<int> m_ConfirmData;
  std::mutex m_mutex;
  std::mutex m_mutexListenners;
  std::vector<IListenner*> m_Listenners;
  ReceiveCallback m_recvCallback;
  std::shared_ptr<MqttContext> m_pContext;
  std::vector<std::shared_ptr<MosquitoWrapper>> m_mosquitoWrappers;
  std::vector<MqttOption> m_options;
  bool m_bIsConnected, m_bReConnected;
};

}  // namespace rics

#endif  // RICS_MQTTTRANSPORT_H