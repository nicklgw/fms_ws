#include "rics_data_service/fms_adapter/MosquitoWrapper.h"

#include <mosquitto.h>

#include <cstring>
#include <iostream>

#include "rics_data_service/Logging.h"

using namespace rics;

MosquitoWrapper::MosquitoWrapper(MqttOption option)
    : m_option(option), m_pMosquittoHandle(nullptr), m_bExitFlag(false) {
  mosquitto_lib_init();
  m_pMosquittoHandle = mosquitto_new(nullptr, option.m_bCleanSession, this);
  if (!m_pMosquittoHandle) {
    std::string strError("mosquitto_new fail:");
    strError.append(strerror(errno));
    throw std::runtime_error(strError.c_str());
  }
}

MosquitoWrapper::~MosquitoWrapper() {
  if (m_pMosquittoHandle) {
    mosquitto_disconnect(m_pMosquittoHandle);
    mosquitto_loop_stop(m_pMosquittoHandle, true);
  }

  m_bExitFlag.store(true);
  if (m_loopThread.joinable()) {
    m_loopThread.join();
  }

  if (m_pMosquittoHandle) {
    mosquitto_destroy(m_pMosquittoHandle);
  }
  mosquitto_lib_cleanup();
}

void MosquitoWrapper::Init(const CallbackList& callbacks) {
  m_callbacks = callbacks;
  int ret = 0;

  // 1. 设置协议版本 (MQTT 3.1.1)
  int version = MQTT_PROTOCOL_V311;
  ret = mosquitto_opts_set(m_pMosquittoHandle, MOSQ_OPT_PROTOCOL_VERSION, &version);
  if (ret != MOSQ_ERR_SUCCESS) {
    throw std::runtime_error(GetErrMsg(ret));
  }

  // 2. 设置用户名密码 (如果有)
  if (!m_option.m_strUserName.empty() && !m_option.m_strPassword.empty()) {
    mosquitto_username_pw_set(m_pMosquittoHandle, m_option.m_strUserName.c_str(),
                              m_option.m_strPassword.c_str());
  }

  // 3. 【关键】TLS 初始化逻辑
  // 只有当有 cafile 时才启用 TLS，或者明确需要 TLS 连接
  if (!m_option.m_strCafile.empty() || m_option.m_nPort == 8883 || m_option.m_nPort == 2884) {
    const char* cafile = m_option.m_strCafile.empty() ? nullptr : m_option.m_strCafile.c_str();
    
    // 只有当 cafile 不为空时才调用 tls_set
    if (cafile != nullptr) {
      ret = mosquitto_tls_set(m_pMosquittoHandle, cafile, nullptr, nullptr, nullptr, nullptr);
      if (ret != MOSQ_ERR_SUCCESS) {
        throw std::runtime_error("TLS init failed: " + GetErrMsg(ret));
      }
    }

    // 4. 设置 insecure 模式（如果需要）
    if (m_option.m_bInsecure) {
      ret = mosquitto_tls_insecure_set(m_pMosquittoHandle, true);
      if (ret != MOSQ_ERR_SUCCESS) {
        throw std::runtime_error("TLS insecure set failed: " + GetErrMsg(ret));
      }
    }
  }

  // 5. 设置回调
  mosquitto_connect_callback_set(m_pMosquittoHandle, OnConnected);
  mosquitto_disconnect_callback_set(m_pMosquittoHandle, OnDisConnected);
  mosquitto_message_callback_set(m_pMosquittoHandle, OnReceived);
  mosquitto_subscribe_callback_set(m_pMosquittoHandle, OnSubscribed);
  mosquitto_publish_callback_set(m_pMosquittoHandle, OnPublished);
  mosquitto_max_inflight_messages_set(m_pMosquittoHandle, m_option.m_nMaxInflight);

  StartLoop();
}

void MosquitoWrapper::StartLoop() {
  m_loopThread = std::thread([this]() {
    while (!m_bExitFlag.load()) {
      // 调试输出参数值
      // std::cout << "DEBUG - Connecting to: Host=" << m_option.m_strHost
      //           << ", Port=" << m_option.m_nPort << ", KeepAlive=" << m_option.m_nKeepAlive
      //           << std::endl;
      RICS_INFO("Connecting to: Host=%s, Port=%d, KeepAlive=%d", m_option.m_strHost.c_str(),
                m_option.m_nPort, m_option.m_nKeepAlive);

      // 参数验证
      if (m_option.m_strHost.empty()) {
        // std::cerr << "MQTT host address is empty!" << std::endl;
        RICS_ERROR("MQTT host address is empty!");
        std::this_thread::sleep_for(std::chrono::seconds(5));
        continue;
      }

      if (m_option.m_nPort <= 0 || m_option.m_nPort > 65535) {
        // std::cerr << "Invalid port number: " << m_option.m_nPort << std::endl;
        RICS_ERROR("Invalid port number: %d", m_option.m_nPort);
        std::this_thread::sleep_for(std::chrono::seconds(5));
        continue;
      }

      // std::cout << "Attempting connection to " << m_option.m_strHost << ":" << m_option.m_nPort
      //           << "...\n";
      RICS_INFO("Attempting connecting to: Host=%s, Port=%d, KeepAlive=%d",
                m_option.m_strHost.c_str(), m_option.m_nPort, m_option.m_nKeepAlive);

      int ret = mosquitto_connect(m_pMosquittoHandle, m_option.m_strHost.c_str(), m_option.m_nPort,
                                  m_option.m_nKeepAlive);
      if (MOSQ_ERR_SUCCESS != ret) {
        std::string strError("mosquitto_connect fail: " + GetErrMsg(ret));
        // std::cerr << "Network error[" << strError << "], Connect failed, retry ..." << std::endl;
        RICS_ERROR("Network error[%s], Connect failed, retry ...", strError.c_str());
      } else {
        ret = mosquitto_loop_forever(m_pMosquittoHandle, m_option.m_nReConnectTime, 1);
        if (MOSQ_ERR_SUCCESS != ret) {
          // TODO:事件
          // std::cerr << "mosquitto_loop_forever returned! error=" << GetErrMsg(ret) << std::endl;
          RICS_ERROR("mosquitto_loop_forever returned! error=%s", GetErrMsg(ret).c_str());
        }
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
  });
}

std::string MosquitoWrapper::GetErrMsg(int errCode) {
  if (MOSQ_ERR_ERRNO == errCode) {
    return strerror(errCode);
  } else {
    return mosquitto_strerror(errCode);
  }
}

bool MosquitoWrapper::Send(const std::shared_ptr<MqttMessage>& pMessage) {
  if (pMessage->m_strTopic.empty()) {
    return false;
  }

  int ret = mosquitto_publish(m_pMosquittoHandle, &pMessage->m_nMessageId,
                              pMessage->m_strTopic.c_str(), pMessage->m_strPayload.size(),
                              pMessage->m_strPayload.c_str(), pMessage->m_nQos, false);
  // std::cout << "MosquitoWrapper::Send: topic=" << pMessage->m_strTopic
  //           << ", payload=" << pMessage->m_strPayload << ", qos=" << pMessage->m_nQos
  //           << ", messageId=" << pMessage->m_nMessageId << std::endl;
  RICS_INFO("Send: topic=%s, payload=%s, qos=%d, messageId=%d", pMessage->m_strTopic.c_str(),
             pMessage->m_strPayload.c_str(), pMessage->m_nQos, pMessage->m_nMessageId);
  return MOSQ_ERR_SUCCESS == ret;
}

bool MosquitoWrapper::Subscribe(const std::string& topic, int qos) {
  if (!m_pMosquittoHandle || topic.empty()) {
    return false;
  }
  int mid = 0;
  int ret = mosquitto_subscribe(m_pMosquittoHandle, &mid, topic.c_str(), qos);
  RICS_INFO("Subscribe topic: %s, qos: %d, mid: %d", topic.c_str(), qos, mid);
  return MOSQ_ERR_SUCCESS == ret;
}

void MosquitoWrapper::OnConnected(mosquitto* mosHandle, void* pObj, int reasonCode) {
  (void)mosHandle;
  auto pInstance = static_cast<MosquitoWrapper*>(pObj);
  if (pInstance) {
    // for (const auto& topic : pInstance->m_subTopics)
    // {
    //     mosquitto_subscribe(pInstance->m_pMosquittoHandle,
    //         nullptr, topic.c_str(), pInstance->m_option.m_nQos);
    // }

    if (pInstance->m_callbacks.connectedCallback) {
      pInstance->m_callbacks.connectedCallback(reasonCode == 0);
    }
  }

  if (reasonCode == 0) {
    // std::cout << "mqtt connect success" << std::endl;
    RICS_INFO("mqtt connect success");
  } else {
    // std::cout << "OnConnected reason " << reasonCode << std::endl;
    RICS_ERROR("mqtt connect failed, reasonCode: %d", reasonCode);
  }
}

void MosquitoWrapper::OnDisConnected(mosquitto* mosHandle, void* pObj, int reasonCode) {
  (void)mosHandle;
  auto pInstance = static_cast<MosquitoWrapper*>(pObj);
  if (pInstance && pInstance->m_callbacks.connectedCallback) {
    pInstance->m_callbacks.connectedCallback(false);
  }
  if (pInstance && reasonCode != MOSQ_ERR_SUCCESS) {
    // std::cerr << "Mqtt disconnected: reasonCode: " << reasonCode
    //           << ", strerror: " << pInstance->GetErrMsg(reasonCode) << std::endl;
    RICS_ERROR("Mqtt disconnected: reasonCode: %d, strerror: %s", reasonCode,
               pInstance->GetErrMsg(reasonCode).c_str());
  }
}

void MosquitoWrapper::OnReceived(mosquitto* mosHandle, void* pObj, const mosquitto_message* pMessage) {
  (void)mosHandle;
  auto pInstance = static_cast<MosquitoWrapper*>(pObj);
  if (!pInstance || !pMessage) return;

  auto msg = std::make_shared<MqttMessage>();
  msg->m_strTopic = pMessage->topic;
  
  // 处理空 payload 的情况
  if (pMessage->payload && pMessage->payloadlen > 0) {
    msg->m_strPayload = std::string(static_cast<char*>(pMessage->payload), pMessage->payloadlen);
  } else {
    msg->m_strPayload = "";
  }
  
  msg->m_nQos = pMessage->qos;
  msg->m_nMessageId = pMessage->mid;

  RICS_INFO("Received: topic=%s, payload=%s, qos=%d", msg->m_strTopic.c_str(),
             msg->m_strPayload.c_str(), msg->m_nQos);

  if (pInstance->m_callbacks.receivedCallback) {
    pInstance->m_callbacks.receivedCallback(msg);
  }
}

void MosquitoWrapper::OnSubscribed(mosquitto* mosHandle, void* pObj, int mid, int qosCount, const int* grantedQos) {
  (void)mosHandle;
  auto pInstance = static_cast<MosquitoWrapper*>(pObj);
  if (!pInstance) return;

  std::vector<int> qosList(grantedQos, grantedQos + qosCount);
  RICS_INFO("Subscribed: mid=%d, qosCount=%d", mid, qosCount);

  if (pInstance->m_callbacks.subscribedCallback) {
    pInstance->m_callbacks.subscribedCallback(mid, qosList);
  }
}

void MosquitoWrapper::OnPublished(mosquitto* mosHandle, void* pObj, int mid) {
  // std::cout << "OnPublished : publish success, mid : " << mid << std::endl;
  (void)mosHandle;
  auto pInstance = static_cast<MosquitoWrapper*>(pObj);
  if (pInstance && pInstance->m_callbacks.sendCallback) {
    pInstance->m_callbacks.sendCallback(mid);
  }
}