#include <rics_data_service/data_collect_service/adapter/DataEventConsumer.h>
#include <unistd.h>

#include <iostream>

// #include <rics_data_service/HttpsHelper.h>
using namespace std;
using namespace rics::data_collect;

DataEventConsumer::DataEventConsumer() {}

DataEventConsumer::~DataEventConsumer() { Stop(); }

void DataEventConsumer::Init() {
  // 创建ROS节点
  m_pNode = std::make_shared<rclcpp::Node>("datacollect_listener");

  // 依赖注入（从ObjectInjection获取IDataCollectService）
  m_objInjection = ObjectInjection::Getinstance();
  auto obj = m_objInjection->GetObject(k_DataCollectService);
  if (obj.empty()) {
    throw std::runtime_error("IDataCollectService not found in ObjectInjection");
  }
  m_DataCollectService = boost::any_cast<std::shared_ptr<IDataCollectService>>(obj);

  // 创建订阅管理器
  // m_subscription_manager = std::make_shared<DataSubscriptionManager>(m_pNode,
  // m_DataCollectService); m_subscription_manager->RegisterSubscribers();
  // m_subscription_manager->StartAggregationTimer(3);

  // 创建并配置执行器
  m_executor = std::make_shared<rclcpp::executors::MultiThreadedExecutor>();
  m_executor->add_node(m_pNode);

  // 创建订阅者
  CreateSubscriber();

  Start();
}

void DataEventConsumer::Start() {
  if (!m_spin_thread.joinable()) {
    m_spin_thread = std::thread([this]() { m_executor->spin(); });
  }
}

void DataEventConsumer::Stop() {
  if (m_spin_thread.joinable()) {
    m_executor->cancel();
    m_spin_thread.join();
  }
  m_pSub.reset();
  m_executor.reset();
  m_pNode.reset();  // 释放ROS节点
  UnsubscribeNode();
}

void DataEventConsumer::CreateSubscriber() {
  // 配置QoS并创建订阅者
  const rclcpp::QoS& qos = rclcpp::SystemDefaultsQoS();
  auto callback = std::bind(&DataEventConsumer::OnNewData, this, std::placeholders::_1);
  m_pSub = m_pNode->create_subscription<MqttSimple>("/rics/rics_data_to_fms", qos, callback);
}

void DataEventConsumer::OnNewData(MqttSimple::SharedPtr pMessage) {
  if (m_DataCollectService) {
    // std::cout << "Received topic: " << pMessage->topic << std::endl;
    // std::cout << "Received message: " << pMessage->message << std::endl;
    m_DataCollectService->PushData(*pMessage);
  }
}

void DataEventConsumer::UnsubscribeNode() {
  if (m_DataCollectService) {
    m_DataCollectService->ClearCache();
  }
}
