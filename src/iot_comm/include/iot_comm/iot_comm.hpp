
#ifndef IOT_COMM__IOT_COMM_HPP_
#define IOT_COMM__IOT_COMM_HPP_

#include <memory>
#include <string>
#include <atomic>
#include <sys/time.h>
#include <random>

#include <diagnostic_updater/diagnostic_updater.hpp>
#include <diagnostic_msgs/msg/diagnostic_array.hpp>
#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"
#include "std_msgs/msg/bool.hpp"
#include "std_msgs/msg/int64.hpp"
#include "std_msgs/msg/int32.hpp"
#include "sensor_msgs/msg/battery_state.hpp"
#include "nav_msgs/msg/odometry.hpp"
#include "iot_comm/msg/custom_info.hpp"
#include <system_diagnostic_msgs/msg/exception_aggregate.hpp>
#include <mqtt_client_interfaces/srv/is_connected.hpp>

#include "iot_comm/persist_parameter_client.hpp"
#include "realtime_tools/realtime_buffer.h"

#include "rics_data_service_msgs/msg/mqtt_simple.hpp"

namespace iot_comm
{

class IotComm : public rclcpp::Node 
{
 public:
  IotComm();
  virtual ~IotComm();
  
  void run_();
  void stop_();

  void do_deviceinfo();
  void do_all_exception();
  void do_customInfo();
  void do_login();
  void do_engage();

 private:

  rclcpp::Publisher<rics_data_service_msgs::msg::MqttSimple>::SharedPtr pub_to_fms_ = nullptr;         // Robot->FMS    /rics/rics_data_to_fms
  rclcpp::Subscription<std_msgs::msg::String>::SharedPtr sub_from_fms_;                                // FMS->Robot    /rics/fms_to_robot
  void from_fms_cb(const std_msgs::msg::String::SharedPtr msg);

  // rclcpp::Publisher<std_msgs::msg::String>::SharedPtr pub_deviceinfo_ = nullptr;                                // 机器人实时数据上报
  // rclcpp::Publisher<std_msgs::msg::String>::SharedPtr pub_exception_ = nullptr;                                 // 机器人故障信息上报
  // rclcpp::Publisher<std_msgs::msg::String>::SharedPtr pub_all_exception_ = nullptr;                             // 机器人全量故障信息上报
  // rclcpp::Publisher<std_msgs::msg::String>::SharedPtr pub_device_login_ = nullptr;                              // 机器人开机上报login

  rclcpp::Subscription<std_msgs::msg::String>::SharedPtr sub_connect_login_ack_;                                // FMS响应login
  void connect_login_ack_cb(const std_msgs::msg::String::SharedPtr msg);
  std::atomic_int connect_login_ack_response_code_{-1};
  // rclcpp::Publisher<std_msgs::msg::String>::SharedPtr pub_device_lock_notify_ = nullptr;                        // 机器人上报锁机/解锁结果
  // rclcpp::Publisher<std_msgs::msg::String>::SharedPtr pub_device_customInfo_ = nullptr;                         // 地面整平机器人上装信息采集

  // rclcpp::Subscription<std_msgs::msg::String>::SharedPtr sub_connect_lock_;                                     // FMS下发lock指令
  void connect_lock_cb(const std_msgs::msg::String::SharedPtr msg);                                             // 锁机请求回调
  // rclcpp::Publisher<std_msgs::msg::String>::SharedPtr pub_device_lock_ack_ = nullptr;                           // 机器人响应lock指令

  rclcpp::Publisher<std_msgs::msg::Bool>::SharedPtr pub_vehicle_engage_ = nullptr;                              // 离合器 已结合?

  rclcpp::Subscription<sensor_msgs::msg::BatteryState>::SharedPtr sub_battery_state_ = nullptr;                 // 订阅电池信息
  rclcpp::Subscription<system_diagnostic_msgs::msg::ExceptionAggregate>::SharedPtr sub_exception_agg_;          // 发布异常代码聚合信息
  rclcpp::Subscription<std_msgs::msg::Int32>::SharedPtr sub_sys_state_;                                         // 订阅系统状态

  // rclcpp::Subscription<iot_comm::msg::CustomInfo>::SharedPtr sub_custom_info_;                                  // 上装消息

  rclcpp::Subscription<nav_msgs::msg::Odometry>::SharedPtr sub_odom_ = nullptr;                                 // 订阅里程计信息  
  realtime_tools::RealtimeBuffer<std::shared_ptr<nav_msgs::msg::Odometry>> odom_rt_;
  
  rclcpp::Subscription<std_msgs::msg::Bool>::SharedPtr sub_vibrate_ = nullptr;                                  // 振动电机开关
  rclcpp::Subscription<std_msgs::msg::Bool>::SharedPtr sub_head_down_ = nullptr;                                // 整平头已放下状态

  rclcpp::Client<mqtt_client_interfaces::srv::IsConnected>::SharedPtr mqtt_conn_client_ = nullptr;
  bool mqtt_is_connected_{false};
  bool mqtt_client_ready_{false};
  void mqtt_conn_send_request(void);

  void exception_agg_cb(const system_diagnostic_msgs::msg::ExceptionAggregate::SharedPtr msg);
  system_diagnostic_msgs::msg::ExceptionAggregate exception_aggregate_;
  std::mutex exception_aggregate_mutex_;
  
  void battery_state_cb(const sensor_msgs::msg::BatteryState::SharedPtr msg);
  std::atomic_int battery_remaining_{0}; // 剩余电量

  // void custom_info_cb(const iot_comm::msg::CustomInfo::SharedPtr msg);

  diagnostic_updater::Updater updater_;
  
  void checkConnectionStatus(diagnostic_updater::DiagnosticStatusWrapper & stat);

  std::shared_ptr<std::thread> thread_;
  volatile bool do_stop_;
  std::chrono::duration<double, std::ratio<1L>> period_;

  double mqtt_timeout_{60.0};
  std::string software_version_;
  std::string did_;
  double run_frequency_;

  std::atomic<bool> head_been_down_{false}; // 整平头已放下状态?
  std::atomic<bool> vibrate_command_{false}; // 振捣电机状态

  std::string mac_;
  std::string ip_;

  std::atomic_int sys_state_{2};

  static constexpr const char* PERSISTENT_LOCK_REQUEST = "persistent.lock_request"; // 持久化锁机请求
  // 锁机请求 1：锁机；2: 解锁
  static constexpr int LOCK_ACTION_LOCK = 1;
  static constexpr int LOCK_ACTION_UNLOCK = 2;
  
  int remote_lock_request_{0}; // 锁机请求 1：锁机；2: 解锁

  static constexpr const char* PERSISTENT_TASK_MAGIC = "persistent.task_magic"; // 本次开始统计的随机数，如果与该值相同，意味着同一次
  static constexpr const char* PERSISTENT_TASK_TIME  = "persistent.task_time";  // 累计作业时间，每10s更新一次数据，单位小时   h
  static constexpr const char* PERSISTENT_TASK_AREA  = "persistent.task_area";  // 累计作业面积，每10s更新一次数据，单位平方米 ㎡
  int       task_magic_{123456}; 
  double    task_time_ {0.0001}; // 累计作业时长
  double    task_area_ {0.0001}; // 累计作业面积
  int       this_magic_{123456}; // 本次程序启动，产生一个随机数
  double    this_time_ {0.0001}; // 本次程序启动，作业时长 单位:s
  double    this_area_ {0.0001}; // 本次程序启动，作业面积 单位:㎡

  double scraper_length_  = 2.5; // 刮板长度

  realtime_tools::RealtimeBuffer<std::shared_ptr<iot_comm::msg::CustomInfo>> task_stat_rt_;

  PersistParametersClient persist_parameter_client_;
};

}  // namespace iot_comm

#endif  // IOT_COMM__IOT_COMM_HPP_
