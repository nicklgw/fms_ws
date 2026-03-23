#pragma once
#include <rics_data_service/RicsDefines.h>
#include <rics_data_service/data_collect_service/adapter/ObjectInjection.h>
#include <rics_data_service/data_collect_service/port/IDataCollectService.h>

#include <memory>
#include <rclcpp/rclcpp.hpp>
// #include <rics_data_service/data_collect_service/adapter/DataSubscriptionManager.h>

namespace rics {
namespace data_collect {
class DataEventConsumer {
 public:
  DataEventConsumer();
  ~DataEventConsumer();

  /**
   * @brief 初始化消费者，创建节点、订阅者和执行器
   */
  void Init();

  /**
   * @brief 启动执行器线程
   */
  void Start();

  /**
   * @brief 停止执行器线程并释放资源
   */
  void Stop();

 private:
  /**
   * @brief 消息回调函数，处理接收到的MqttSimple消息
   * @param pMessage 接收到的消息指针
   */
  void OnNewData(MqttSimple::SharedPtr pMessage);

  /**
   * @brief 创建ROS订阅者
   */
  void CreateSubscriber();

  /**
   * @brief 取消订阅并清理缓存
   */
  void UnsubscribeNode();

 private:
  std::thread m_spin_thread;                                             ///< 自旋线程
  std::shared_ptr<ObjectInjection> m_objInjection;                       ///< 对象注入器
  std::shared_ptr<IDataCollectService> m_DataCollectService;             ///< 数据收集服务接口
  rclcpp::Subscription<MqttSimple>::SharedPtr m_pSub;                    ///< ROS订阅者
  std::shared_ptr<rclcpp::Node> m_pNode;                                 ///< ROS节点
  std::shared_ptr<rclcpp::executors::MultiThreadedExecutor> m_executor;  ///< ROS执行器
  // std::shared_ptr<DataSubscriptionManager> m_subscription_manager;
};
}  // namespace data_collect
}  // namespace rics