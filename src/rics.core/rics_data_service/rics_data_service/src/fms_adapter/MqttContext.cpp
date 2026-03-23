#include "rics_data_service/fms_adapter/MqttContext.h"

#include <algorithm>

using namespace rics;

MqttContext::MqttContext() : m_bSending(false), m_currentSendCmd(-1) {}

bool MqttContext::SetSendCmd(int sendCmd) {
  SpinFor(m_bSending);
  m_currentSendTopic.clear();
  m_currentSendCmd = sendCmd;
  return m_sendCmd2Topic.count(sendCmd) > 0;
}

bool MqttContext::SetSendTopic(const std::string& strTopic) {
  SpinFor(m_bSending);
  m_currentSendTopic = strTopic;
  m_currentSendCmd = -1;
  return true;
}

std::string MqttContext::GetSendTopic() {
  std::string strTopic;

  if (!m_currentSendTopic.empty()) {
    strTopic = m_currentSendTopic;

  } else if (m_currentSendCmd != -1) {
    if (m_sendCmd2Topic.count(m_currentSendCmd)) {
      strTopic = m_sendCmd2Topic[m_currentSendCmd];
    }
  }

  m_bSending.store(false);
  return strTopic;
}

bool MqttContext::SetRecvTopic(const std::string& strTopic) {
  m_currentRecvTopic = strTopic;
  return true;
}

std::string MqttContext::GetRecvTopic() {
  return m_currentRecvTopic;
}

void MqttContext::SpinFor(std::atomic_bool& flag) {
  bool expected = false;
  while (!flag.compare_exchange_strong(expected, true));
}
