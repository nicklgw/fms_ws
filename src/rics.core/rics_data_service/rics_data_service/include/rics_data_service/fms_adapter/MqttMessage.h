#ifndef RICS_MQTTMESSAGE_H
#define RICS_MQTTMESSAGE_H

#include <string>

namespace rics
{
    struct MqttMessage
    {
        /**< 消息话题 */
        std::string m_strTopic;
        /**< 消息内容 */
        std::string m_strPayload;
        /**< 消息id */
        int m_nMessageId;
        /**< qos */
        int m_nQos;
    };
}

#endif //RICS_MQTTMESSAGE_H
