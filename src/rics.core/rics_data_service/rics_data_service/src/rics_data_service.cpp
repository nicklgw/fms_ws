#include "rics_data_service/rics_data_service.hpp"

#include <filesystem>

namespace rics::data {
using namespace rics;

RicsDataService::RicsDataService(const rclcpp::NodeOptions& options)
    : rclcpp_lifecycle::LifecycleNode("rics_data_service", options), diagnostic_updater_(this) {
  // 注册诊断更新器
  diagnostic_updater_.setHardwareID("rics_data_service");
  diagnostic_updater_.add("RicsDataService Status", this, &RicsDataService::diagnostic_update);

  // 延迟初始化
  init_timer_ = this->create_wall_timer(std::chrono::milliseconds(100), [this]() {
    init_timer_->cancel();
    // 直接调用 configure()，由生命周期管理处理初始化流程
    configure();
  });
}

RicsDataService::~RicsDataService() {}

void RicsDataService::configure() {
  auto ret = on_configure(get_current_state());
  if (ret != rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn::SUCCESS) {
    RCLCPP_ERROR(get_logger(), "Failed to configure RicsDataService");
    return;
  }
  
  // 配置成功后自动激活服务
  activate();
}

void RicsDataService::activate() {
  auto ret = on_activate(get_current_state());
  if (ret != rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn::SUCCESS) {
    RCLCPP_ERROR(get_logger(), "Failed to activate RicsDataService");
    return;
  }
}

rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn
RicsDataService::on_configure(const rclcpp_lifecycle::State& previous_state) {
  (void)previous_state;  // 消除未使用参数警告
  RCLCPP_INFO(get_logger(), "on_configure");

  // 所有初始化都在 on_activate 阶段进行，保持代码风格统一

  return rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn::SUCCESS;
}

rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn
RicsDataService::on_activate(const rclcpp_lifecycle::State& previous_state) {
  (void)previous_state;  // 消除未使用参数警告
  RCLCPP_INFO(get_logger(), "on_activate");
  
  // 获取生命周期节点指针
  auto lifecycleNode = std::dynamic_pointer_cast<rclcpp_lifecycle::LifecycleNode>(this->shared_from_this());
  if (!lifecycleNode) {
    RCLCPP_ERROR(get_logger(), "LifecycleNode not available in on_activate");
    return rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn::FAILURE;
  }
  
  // 初始化 ParseRicsConfig
  pParseRicsConfig_ = std::make_shared<ParseRicsConfig>(lifecycleNode);
  
  // 检查服务启用状态
  bool enable_service = pParseRicsConfig_->GetEnableService();
  RCLCPP_INFO(get_logger(), "RicsDataService enable status: %s",
              enable_service ? "ENABLED" : "DISABLED");

  if (!enable_service) {
    RCLCPP_WARN(get_logger(), "RicsDataService is disabled by 'enable_service' parameter");
    return rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn::FAILURE;
  }

  // 加载配置选项
  auto ricsBusinessOption =
      boost::any_cast<RicsBusinessOption>(pParseRicsConfig_->GetAny(typeid(RicsBusinessOption)));
  auto dataCollectOption =
      boost::any_cast<DataCollectOption>(pParseRicsConfig_->GetAny(typeid(DataCollectOption)));
  auto ricsTransportOption =
      boost::any_cast<RicsTransportOption>(pParseRicsConfig_->GetAny(typeid(RicsTransportOption)));

  // 初始化核心组件
  HttpsHelper::Instance(ricsBusinessOption.strDefaultPath, ricsBusinessOption.strPrivateKey);
  pRicsTransport_ = TransportFactory::CreateTransport(ricsTransportOption);
  pDataCollectFactory_ = std::make_shared<DataCollectFactory>();
  pDataCollectFactory_->CreateDataCollectObj(pRicsTransport_, dataCollectOption,
                                             ricsBusinessOption);
  
  // 初始化 FMS 消息监听器
  auto mqttTransport = std::dynamic_pointer_cast<rics::MqttTransport>(pRicsTransport_);
  if (mqttTransport) {
    // 获取设备序列号
    std::string sn = ricsBusinessOption.robotConfig.strSN;
    if (sn.empty()) {
      RCLCPP_WARN(get_logger(), "Robot SN is empty, using default SN for FMS message listener");
      sn = "rics-ccu-8986-5869"; // 使用默认的设备序列号
    }
    
    // 创建 FmsMessageListener 实例
    pFmsMessageListener_ = std::make_unique<rics::fms::FmsMessageListener>(sn);
    
    // 直接使用 LifecycleNode 指针设置发布者
    pFmsMessageListener_->setPublisher(lifecycleNode);
    RCLCPP_INFO(get_logger(), "FMS message listener publisher set in on_activate using LifecycleNode");
    
    // 注册监听器到 MqttTransport
    mqttTransport->RegisterListeners(pFmsMessageListener_.get());
    RCLCPP_INFO(get_logger(), "FMS message listener registered in on_activate");
  } else {
    RCLCPP_WARN(get_logger(), "MqttTransport not available, FMS message listener not registered");
  }
  
  return rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn::SUCCESS;
}

rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn
RicsDataService::on_deactivate(const rclcpp_lifecycle::State& previous_state) {
  (void)previous_state;  // 消除未使用参数警告
  RCLCPP_INFO(get_logger(), "on_deactivate");
  return rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn::SUCCESS;
}

rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn
RicsDataService::on_cleanup(const rclcpp_lifecycle::State& previous_state) {
  (void)previous_state;  // 消除未使用参数警告
  RCLCPP_INFO(get_logger(), "on_cleanup");
  pFmsMessageListener_.reset();
  pParseRicsConfig_.reset();
  pDataCollectFactory_.reset();
  pRicsTransport_.reset();
  return rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn::SUCCESS;
}

rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn
RicsDataService::on_shutdown(const rclcpp_lifecycle::State& previous_state) {
  (void)previous_state;  // 消除未使用参数警告
  RCLCPP_INFO(get_logger(), "on_shutdown");
  pFmsMessageListener_.reset();
  pParseRicsConfig_.reset();
  pDataCollectFactory_.reset();
  pRicsTransport_.reset();
  return rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn::SUCCESS;
}

void RicsDataService::diagnostic_update(diagnostic_updater::DiagnosticStatusWrapper& stat) {
  if (!pParseRicsConfig_) {
    // 修复：get_name() 先转 std::string 再拼接
    stat.summary(diagnostic_msgs::msg::DiagnosticStatus::OK,
                 std::string(get_name()) + " is initializing (config not ready)");
    return;
  }

  bool enable_service = false;
  try {
    enable_service = pParseRicsConfig_->GetEnableService();
  } catch (const std::exception& e) {
    RCLCPP_WARN(get_logger(), "Failed to get enable_service in diagnostic_update: %s", e.what());
    // 修复：get_name() 先转 std::string 再拼接
    stat.summary(diagnostic_msgs::msg::DiagnosticStatus::WARN,
                 std::string(get_name()) + " config error: " + std::string(e.what()));
    return;
  }

  if (enable_service) {
    // 修复：get_name() 先转 std::string 再拼接
    stat.summary(diagnostic_msgs::msg::DiagnosticStatus::OK,
                 std::string(get_name()) + " is operational");
  } else {
    // 修复：get_name() 先转 std::string 再拼接
    stat.summary(diagnostic_msgs::msg::DiagnosticStatus::WARN,
                 std::string(get_name()) + " is disabled");
  }
}

}  // namespace rics::data

#include "rclcpp_components/register_node_macro.hpp"
RCLCPP_COMPONENTS_REGISTER_NODE(rics::data::RicsDataService)