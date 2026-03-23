#include <jsoncpp/json/json.h>
#include <rics_data_service/Logging.h>

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <iostream>
// #include <interface/DefineRouter.h>
#include <rics_data_service/data_collect_service/domain/DataCollectRepository.h>

using namespace std;
using namespace rics;
using namespace rics::data_collect;

DataCollectRepository::DataCollectRepository() {}

DataCollectRepository::~DataCollectRepository() {}

void DataCollectRepository::Init() {
  m_objInjection = ObjectInjection::Getinstance();
  m_ricsBusinessOption =
      boost::any_cast<RicsBusinessOption>(m_objInjection->GetObject(k_ConfigRics));
  // m_Gpsinfo = boost::any_cast<shared_ptr<interface::IAdapter>>
  //             (m_objInjection->GetObject(k_GpsInfo));
  // m_pRouterAdapter = boost::any_cast<shared_ptr<interface::IAdapter>>
  //     (m_objInjection->GetObject(k_RouterInfo));
  m_queueRearIdx = -1;
}

bool DataCollectRepository::Enqueue(MqttSimple& pMessage) {
  CorrectInfo(pMessage);
  bool bret = m_cache.push(pMessage);

  if (bret) {
    m_queueRearIdx++;
  }

  return bret;
}

void DataCollectRepository::Peek(MqttSimple& pMessage) { pMessage = m_cache.front(); }

void DataCollectRepository::Pop(bool popAll) {
  if (popAll) {
    m_cache.reset();
    m_queueRearIdx = -1;
  } else {
    if (HasData()) {
      m_cache.pop();
      if (m_queueRearIdx >= 0) {
        m_queueRearIdx = m_queueRearIdx - 1;
      }
    }
  }
}

bool DataCollectRepository::HasData() { return (m_cache.empty() == 0) ? true : false; }

bool DataCollectRepository::Full() {
  if (m_queueRearIdx == (QUEUEMAX - 1)) return true;
  return false;
}

/**
 * 修正主题字符串中的设备序列号占位符
 * 如果两个斜杠之间的内容为"{sn}"，则替换为系统配置的实际序列号
 * 否则返回原主题字符串
 */
string DataCollectRepository::CorrectTopic(string& topic) {
  std::string correctedTopic = topic;
  auto firstPos = topic.find_first_of("/");
  auto secondPos = topic.find_last_of("/");
  
  /**< 检查格式并替换占位符 */
  if (firstPos != std::string::npos && secondPos != std::string::npos && firstPos != secondPos) {
    auto placeHolder = topic.substr(firstPos + 1, secondPos - firstPos - 1);
    // 只有当占位符为"{sn}"时才进行替换
    if (placeHolder == "{sn}") {
      correctedTopic = correctedTopic.replace(firstPos + 1, secondPos - firstPos - 1,
                                              m_ricsBusinessOption.robotConfig.strSN);
    }
  }
  return correctedTopic;
}

// 修正消息内容的函数
string DataCollectRepository::CorrectMessage(const string& topic, const string& msg) {
  // 仅处理设备信息相关的主题
  if (topic.find("/deviceinfo") == std::string::npos) {
    return msg;  // 非设备信息主题直接返回原始消息
  }

  // 创建JSON解析器
  auto reader = std::shared_ptr<Json::CharReader>(Json::CharReaderBuilder().newCharReader());
  Json::Value rootObj;

  // 解析JSON消息
  if (!reader->parse(msg.begin().base(), msg.end().base(), &rootObj, nullptr)) {
    RICS_WARN("parse msg failed");  // 解析失败日志
    return msg;                     // 返回原始消息
  }

  // 返回格式化后的JSON字符串
  return rootObj.toStyledString();
}

void DataCollectRepository::CorrectInfo(MqttSimple& pMessage) {
  string topic = pMessage.topic;
  string topicname = CorrectTopic(topic);
  string msg = CorrectMessage(topicname, pMessage.message);
  pMessage.topic = topicname;
  pMessage.message = msg;
}
