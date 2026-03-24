#include "rics_data_service/fms_adapter/FmsMessageListener.h"

#include <iostream>

using namespace rics::fms;
using namespace std;

FmsMessageListener::FmsMessageListener(const std::string& sn) : m_sn(sn) {
  cout << "FmsMessageListener initialized with SN: " << m_sn << endl;
}

FmsMessageListener::~FmsMessageListener() { cout << "FmsMessageListener destroyed" << endl; }

void FmsMessageListener::setPublisher(rclcpp::Node::SharedPtr node) {
  m_rosNode = node;
  CreatePublisher();
  m_publisherSet = true;
  cout << "FmsMessageListener publisher set" << endl;
}

void FmsMessageListener::setPublisher(rclcpp_lifecycle::LifecycleNode::SharedPtr node) {
  // 直接使用生命周期节点创建发布者
  if (node) {
    // 配置 QoS 并创建发布者（使用自定义 MqttSimple 消息）
    const rclcpp::QoS& qos = rclcpp::SystemDefaultsQoS();
    m_rosPublisher =
        node->create_publisher<MqttSimple>("/rics/fms_to_robot", qos);
    m_publisherSet = true;
    cout << "FmsMessageListener publisher set using LifecycleNode" << endl;
  } else {
    cout << "LifecycleNode not set, cannot create publisher" << endl;
  }
}

bool FmsMessageListener::MessageCheck(const string& topic) {
  // 检查是否为 FMS 相关主题
  // 订阅的主题格式：command/{sn}/login-ack, command/{sn}/lock, command/{sn}/notify-ack
  if (topic.find("/login-ack") != string::npos || topic.find("/lock") != string::npos ||
      topic.find("/notify-ack") != string::npos) {
    return true;
  }
  return false;
}

void FmsMessageListener::OnRecvMsgHandle(const string& topic, const string& payload) {
  cout << "Received FMS message: " << endl;
  cout << "  Topic: " << topic << endl;
  cout << "  Payload: " << payload << endl;

  // 只有当发布者已设置时才发布消息
  if (m_publisherSet && m_rosPublisher) {
    // 创建自定义 MqttSimple 消息并发布
    auto ros_msg = make_shared<MqttSimple>();
    // 直接设置字段，无需手动拼接 JSON
    ros_msg->topic = topic;
    ros_msg->message = payload;

    m_rosPublisher->publish(*ros_msg);
    cout << "Published to ROS topic: /rics/fms_to_robot" << endl;
  } else {
    cout << "ROS publisher not set, skipping publish" << endl;
  }
}

// 实现：提供订阅话题列表
std::vector<std::string> FmsMessageListener::GetSubscribeTopics() {
  return {"command/" + m_sn + "/login-ack", "connect/" + m_sn + "/lock",
          "command/" + m_sn + "/notify-ack"};
}

void FmsMessageListener::CreatePublisher() {
  if (m_rosNode) {
    // 配置 QoS 并创建发布者（使用自定义 MqttSimple 消息）
    const rclcpp::QoS& qos = rclcpp::SystemDefaultsQoS();
    m_rosPublisher =
        m_rosNode->create_publisher<MqttSimple>("/rics/fms_to_robot", qos);
    cout << "ROS publisher created for topic: /rics/fms_to_robot" << endl;
  } else {
    cout << "ROS node not set, cannot create publisher" << endl;
  }
}