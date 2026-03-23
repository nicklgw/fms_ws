#ifndef RICS_MQTTCONTEXT_H
#define RICS_MQTTCONTEXT_H

#include <atomic>
#include <unordered_map>

#include "rics_data_service/ITransportContext.h"

namespace rics {
class MqttContext : public ITransportContext {
 public:
  explicit MqttContext();

  bool SetSendCmd(int sendCmd) override;

  bool SetSendTopic(const std::string& strTopic) override;
  bool SetRecvTopic(const std::string& strTopic);

  std::string GetSendTopic();
  std::string GetRecvTopic();

 protected:
  void SpinFor(std::atomic_bool& flag);

 private:
  int m_nProtocol;
  std::string m_strSN;
  /**< 发送标志 */
  std::atomic_bool m_bSending;
  /**< 当前发送命令 */
  int m_currentSendCmd;
  /**< 当前发送话题 */
  std::string m_currentSendTopic;
  /**< 当前接收话题 */
  std::string m_currentRecvTopic;
  /**< 发送命令与话题关系 */
  std::unordered_map<int, std::string> m_sendCmd2Topic;
};
}  // namespace rics

#endif  // RICS_MQTTCONTEXT_H
