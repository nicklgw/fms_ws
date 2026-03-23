#include "rics_data_service/fms_adapter/MqttTransport.h"
#include <algorithm>
#include <cstdlib>
#include <set> // 用于去重
#include "rics_data_service/fms_adapter/MqttMessage.h"
#include "rics_data_service/Logging.h"

using namespace rics;

MqttTransport::MqttTransport(const std::vector<MqttOption>& options,
                             const std::shared_ptr<MqttContext>& pContext)
    : m_pContext(pContext), m_options(options), m_bIsConnected(false), m_bReConnected(false) {
  MosquitoWrapper::CallbackList callbackList{
      std::bind(&MqttTransport::Connected, this, std::placeholders::_1),
      std::bind(&MqttTransport::OnReceived, this, std::placeholders::_1),
      std::bind(&MqttTransport::OnSend, this, std::placeholders::_1),
      std::bind(&MqttTransport::OnSubscribed, this, std::placeholders::_1, std::placeholders::_2)
  };

  for (auto& option : m_options) {
    auto pMosquitoWrapper = std::make_shared<MosquitoWrapper>(option);
    pMosquitoWrapper->Init(callbackList);
    m_mosquitoWrappers.push_back(pMosquitoWrapper);
  }
}

bool MqttTransport::Send(const std::string& strData) {
  int messageId = 0;
  return Send(strData, messageId);
}

bool MqttTransport::Send(const std::string& strData, int& msgId) {
  if (!m_options.size()) {
    return false;
  }
  auto IsQos0Topic = [this](const std::string& topic) {
    return std::any_of(m_options[0].m_lstQos0Topic.begin(), m_options[0].m_lstQos0Topic.end(),
                       [=](const std::string& cacheTopic) {
                         if (topic == cacheTopic) {
                           return true;
                         }
                         return false;
                       });
  };
  auto pMessage = std::make_shared<MqttMessage>();
  pMessage->m_strTopic = m_pContext->GetSendTopic();
  pMessage->m_strPayload = strData;
  if (IsQos0Topic(pMessage->m_strTopic)) {
    pMessage->m_nQos = 0;
  } else {
    pMessage->m_nQos = m_options[0].m_nQos;
  }
  pMessage->m_nMessageId = 0;
  bool bSucc = true;
  for (auto& pWrapper : m_mosquitoWrappers) {
    bSucc &= pWrapper->Send(pMessage);
  }
  msgId = pMessage->m_nMessageId;
  return bSucc;
}

void
MqttTransport::RegisterRecvCallback(const ReceiveCallback& callback)
{
    m_recvCallback = callback;
}

void
MqttTransport::RegisterListeners(IListenner* listener)
{
    {
        std::lock_guard<std::mutex> lock(m_mutexListenners);
        m_Listenners.push_back(listener);
    }
    
    // 如果已经连接，立即尝试订阅新监听者的话题
    if (m_bIsConnected && listener) {
        std::vector<std::string> topics = listener->GetSubscribeTopics();
        for (const auto& topic : topics) {
            if (!topic.empty()) {
                RICS_INFO("Register new listener, subscribing topic: %s", topic.c_str());
                for (auto& pWrapper : m_mosquitoWrappers) {
                    pWrapper->Subscribe(topic, 1);
                }
            }
        }
    }
}

void
MqttTransport::DelListeners(IListenner* listener)
{
    {
        std::lock_guard<std::mutex> lock(m_mutexListenners);
        for (auto it = m_Listenners.begin(); it != m_Listenners.end();)
        {
            if (*it == listener)
            {
                it = m_Listenners.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }
}

std::shared_ptr<ITransportContext> MqttTransport::GetContext() { return m_pContext; }

void MqttTransport::Connected(bool isConnected) {
  m_bIsConnected = isConnected;
  if (m_bIsConnected) {
    m_bReConnected = true;
    RICS_INFO("MQTT connected, collecting topics from listeners...");

    // 使用 set 收集话题，自动去重
    std::set<std::string> allTopics;

    {
        std::lock_guard<std::mutex> lock(m_mutexListenners);
        // 遍历所有监听者，收集话题
        for (auto listenner : m_Listenners) {
            if (listenner) {
                std::vector<std::string> topics = listenner->GetSubscribeTopics();
                for (const auto& topic : topics) {
                    if (!topic.empty()) {
                        allTopics.insert(topic);
                    }
                }
            }
        }
    }

    // 执行订阅
    for (const auto& topic : allTopics) {
        for (auto& pWrapper : m_mosquitoWrappers) {
            pWrapper->Subscribe(topic, 1);
        }
        RICS_INFO("Subscribed topic: %s (QoS=1)", topic.c_str());
    }
  } else {
    RICS_WARN("MQTT disconnected");
  }
}

void
MqttTransport::OnReceived(const std::shared_ptr<MqttMessage>& pMessage)
{
    if (m_recvCallback)
    {
        m_pContext->SetRecvTopic(pMessage->m_strTopic);
        m_recvCallback(pMessage->m_strPayload);
    }
#if 1       //add by ye 20220217 1535
    if (m_Listenners.size() > 0)
    {
        for (auto listenner : m_Listenners)
        {
           if (listenner && listenner->MessageCheck(pMessage->m_strTopic))
           {
               listenner->OnRecvMsgHandle(pMessage->m_strTopic,pMessage->m_strPayload);
           }
        }
    }
#endif
}

void MqttTransport::OnSend(int messageId) {
  std::lock_guard<std::mutex> lk(m_mutex);
  m_ConfirmData.insert(messageId);
}

void MqttTransport::OnSubscribed(int mid, const std::vector<int>& qos) {
  RICS_INFO("Subscribed success: mid=%d, QoS count=%zu", mid, qos.size());
}

/**
 * @brief:  确认上一次的数据是否发送成功，决定是否继续发送
 * @param:
 * @return:
 */
bool MqttTransport::ConfirmLastData(int messageId) {
  std::lock_guard<std::mutex> lk(m_mutex);
  if (m_bReConnected) {
    m_bReConnected = false;
    m_ConfirmData.clear();
    return true;
  }
  // 如果消息ID为0，表示不需要确认，直接返回true
  if (messageId == 0) {
    return true;
  }
  auto iter = m_ConfirmData.find(messageId);
  if (iter != m_ConfirmData.end()) {
    m_ConfirmData.erase(iter);
    return true;
  }
  return false;
}