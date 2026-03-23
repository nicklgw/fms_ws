#include <rics_data_service/ITransportContext.h>
#include <rics_data_service/JsonHelper.h>
#include <rics_data_service/data_collect_service/adapter/DataReporter.h>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <iostream>
#include <string>

#include "rics_data_service/Logging.h"

using namespace std;
using namespace rics::data_collect;

DataReporter::DataReporter() : m_LastConnectStu(true), m_LastMessageId(0) {}

DataReporter::~DataReporter() {}

void DataReporter::Init() {
  m_objInjection = ObjectInjection::Getinstance();
  m_TransPort =
      boost::any_cast<shared_ptr<rics::ITransport>>(m_objInjection->GetObject(k_TransPort));
  m_ricsBusinessOption =
      boost::any_cast<RicsBusinessOption>(m_objInjection->GetObject(k_ConfigRics));
}

bool DataReporter::IsConnected() { return m_TransPort->IsConnected(); }

bool DataReporter::IsReConnect() {
  bool ret = false;
  bool currStu = m_TransPort->IsConnected();
  if ((currStu != m_LastConnectStu) && (currStu)) ret = true;
  m_LastConnectStu = currStu;
  return ret;
}

// 发送消息到指定主题
bool DataReporter::SendMsg(const string& topicName, const string& msg) {
  // 确认上次数据已成功发送
  if (!m_TransPort->ConfirmLastData(m_LastMessageId)) return false;

  // 准备JSON解析器
  Json::Value rootObj;
  auto reader = std::shared_ptr<Json::CharReader>(Json::CharReaderBuilder().newCharReader());
  JSONCPP_STRING errs;
  bool ret = false;

  // 解析原始消息
  if (reader->parse(msg.c_str(), msg.c_str() + msg.length(), &rootObj, &errs)) {
    // 添加类型校验
    if (!rootObj.isObject()) {
      // std::cerr << "Invalid JSON structure, expected object type" << std::endl;
      RICS_ERROR("Invalid JSON structure, expected object type");
      return false;
    }

    // 生成消息唯一标识（如果不存在）
    if (rootObj["MsgID"].isNull()) {
      boost::uuids::uuid a_uuid = boost::uuids::random_generator()();
      rootObj["MsgID"] = boost::uuids::to_string(a_uuid);
    }

    // 填充设备编码（使用机器人序列号）
    if (rootObj["DeviceCode"].isNull()) {
      rootObj["DeviceCode"] = m_ricsBusinessOption.robotConfig.strSN;  // 来自机器人配置的序列号
    }

    // 设置传输主题并发送消息
    m_TransPort->GetContext()->SetSendTopic(topicName);
    ret = m_TransPort->Send(rootObj.toStyledString(), m_LastMessageId);
  } else {
    // 处理消息解析错误
    // std::cerr << " parse message error!" << std::endl;
    RICS_ERROR(" parse message error!");
  }

  return ret;
}
