#pragma once
#include <rics_data_service/RicsDefines.h>
#include <rics_data_service/fms_adapter/MqttTransport.h>

#include <memory>
#include <rclcpp/rclcpp.hpp>
#include <rclcpp_lifecycle/lifecycle_node.hpp>
// #include <std_msgs/msg/string.hpp>
#include <string>

namespace rics {
namespace fms {
class FmsMessageListener : public IListenner {
 public:
  /**
   * @brief 构造函数
   * @param sn 设备序列号（用于拼接订阅话题）
   */
  explicit FmsMessageListener(const std::string& sn);
  ~FmsMessageListener() override;

  /**
   * @brief 设置 ROS 2 发布者
   * @param node ROS 2 节点指针
   */
  void setPublisher(rclcpp::Node::SharedPtr node);

  /**
   * @brief 设置 ROS 2 发布者（使用生命周期节点）
   * @param node ROS 2 生命周期节点指针
   */
  void setPublisher(rclcpp_lifecycle::LifecycleNode::SharedPtr node);

  /**
   * @brief 检查消息主题是否为 FMS 相关主题
   * @param topic 消息主题
   * @return 是否为 FMS 相关主题
   */
  bool MessageCheck(const std::string& topic) override;

  /**
   * @brief 处理接收到的 FMS 消息
   * @param topic 消息主题
   * @param payload 消息内容
   */
  void OnRecvMsgHandle(const std::string& topic, const std::string& payload) override;

  /**
   * @brief 提供需要订阅的话题列表（实现 IListenner 接口）
   * @return 订阅话题列表
   */
  std::vector<std::string> GetSubscribeTopics() override;

 private:
  /**
   * @brief 创建 ROS 2 发布者
   */
  void CreatePublisher();

 private:
  std::string m_sn;                                                    ///< 设备序列号
  rclcpp::Node::SharedPtr m_rosNode;                                   ///< ROS 2 节点
  rclcpp::Publisher<MqttSimple>::SharedPtr m_rosPublisher;             ///< ROS 2 发布者
  bool m_publisherSet = false;                                         ///< 发布者是否已设置
};
}  // namespace fms
}  // namespace rics