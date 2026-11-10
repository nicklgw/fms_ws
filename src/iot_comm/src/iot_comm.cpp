
#include "iot_comm/iot_comm.hpp"
#include <nlohmann/json.hpp>
#include <unique_identifier_msgs/msg/uuid.hpp>

#include <random>
#include <string>

namespace iot_comm {

static unique_identifier_msgs::msg::UUID generateUUID()
{
  // Generate random number
  unique_identifier_msgs::msg::UUID uuid;
  std::mt19937 gen(std::random_device{}());
  std::independent_bits_engine<std::mt19937, 8, uint8_t> bit_eng(gen);
  std::generate(uuid.uuid.begin(), uuid.uuid.end(), bit_eng);

  return uuid;
}

static std::string toHexString(const unique_identifier_msgs::msg::UUID & id)
{
  std::stringstream ss;
  for (auto i = 0; i < 16; ++i) {
    ss << std::hex << std::setfill('0') << std::setw(2) << +id.uuid[i];
  }
  return ss.str();
}

static std::string getTimeStamp(void)
{
  struct timeval time;
  gettimeofday(&time, NULL);
  std::stringstream systemTime;
  systemTime << time.tv_sec;
  systemTime << std::fixed << std::setw(3) << std::setfill('0') << time.tv_usec / 1000;
  std::string timestamp = systemTime.str();
  return timestamp;
}

IotComm::IotComm()
 : rclcpp::Node::Node("iot_comm")
 , updater_(this)
{
  declare_parameter("scraper_length", 1.5);
  scraper_length_ = get_parameter("scraper_length").as_double();

  RCLCPP_INFO(get_logger(), "IotComm::IotComm() scraper_length: %.3f", scraper_length_);  

  pub_to_fms_ = create_publisher<rics_data_service_msgs::msg::MqttSimple>("/rics/rics_data_to_fms", rclcpp::SystemDefaultsQoS());

  sub_sys_state_ = create_subscription<std_msgs::msg::Int32>(
    "/iot/sys_state", rclcpp::SensorDataQoS().keep_last(1),
    [this](const std_msgs::msg::Int32::SharedPtr msg) { sys_state_ = msg->data; is_running_ = (sys_state_ == 3); });

  odom_rt_.writeFromNonRT(std::make_shared<nav_msgs::msg::Odometry>());
  sub_odom_ = create_subscription<nav_msgs::msg::Odometry>(
    "/odom", rclcpp::SystemDefaultsQoS(), 
    [this](const nav_msgs::msg::Odometry::SharedPtr msg) { odom_rt_.writeFromNonRT(msg); }); // 订阅里程计信息

  joint_motor_values_data_.writeFromNonRT(std::make_shared<std_msgs::msg::Float64MultiArray>());
  sub_joint_motor_values_ = create_subscription<std_msgs::msg::Float64MultiArray>("/joint_motor_values", rclcpp::SystemDefaultsQoS(), [this](const std_msgs::msg::Float64MultiArray::SharedPtr msg) { joint_motor_values_data_.writeFromNonRT(msg); });

  int period_ms_ = 10 * 1000; // 10s

  update_timer_ = create_wall_timer(std::chrono::milliseconds(period_ms_), std::bind(&IotComm::update, this));

  updater_.setHardwareID("iot_comm");
  updater_.add("connection", this, &IotComm::checkConnectionStatus);
}

IotComm::~IotComm()
{

}

void IotComm::update(void)
{
  // 根据业务逻辑更新
  do_customInfo(); // 每10s更新一次作业面积统计
}

void IotComm::do_customInfo()
{
  double delta_area = 0.0; // 本次循环作业面积

  // 上装作业任务统计
  {
    static bool last_is_running = false;
    static double last_odometer = 0.0;

    auto odom = *(odom_rt_.readFromRT());
    double odometer = std::hypot(fabs(odom->pose.pose.position.x), fabs(odom->pose.pose.position.y));

    if (!last_is_running && is_running_)
    {
      last_odometer = odometer;
    }

    if (is_running_)
    {
      delta_area = fabs(odometer - last_odometer) * scraper_length_;
      last_odometer = odometer;
    }

    last_is_running = is_running_;
  }

  // 刀盘转速 小于10rpm, 才允许 按重定位按钮生效
  double left_blade_vel = 0.0;
  {
    auto motor_values = (*(joint_motor_values_data_.readFromRT()))->data;
    if (motor_values.size() > 0)
    {
      left_blade_vel = motor_values[0];
    }
  }

  {
    nlohmann::json root;

    std::string timestamp = getTimeStamp();

    root["MsgID"] = toHexString(generateUUID());
    root["Timestamp"] = timestamp;
    root["DeviceCode"] = "+";

    nlohmann::json data;

    data["workState"] = is_running_.load(); // 上装状态: workState 0(未作业) / 1(作业中) 
    data["cutterheadSpeed"] = left_blade_vel; // 刀盘转速：cutterheadSpeed，单位rpm
    data["workArea"] = delta_area; // 作业面积：workArea，间隔时间内作业面积增量，单位㎡

    root["Data"] = data;

    rics_data_service_msgs::msg::MqttSimple mqtt_msg;
    mqtt_msg.topic = std::string("device/+/customInfo");
    mqtt_msg.message = root.dump();
    pub_to_fms_->publish(mqtt_msg);
  }
}

void IotComm::checkConnectionStatus(diagnostic_updater::DiagnosticStatusWrapper & stat)
{
  stat.add<bool>("mqtt_client_ready", true);
  stat.add<bool>("mqtt_is_connected", true);

  int8_t diag_level = diagnostic_msgs::msg::DiagnosticStatus::OK;
  std::string diag_message = "Connection is OK.";
  
  stat.name = "connection";
  stat.hardware_id = "connection";
  stat.level = diag_level;
  stat.message = diag_message;
}

}  // namespace iot_comm
