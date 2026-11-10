
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
#include <system_diagnostic_msgs/msg/exception_aggregate.hpp>
#include "realtime_tools/realtime_buffer.hpp"

#include "realtime_tools/realtime_buffer.h"
#include <std_msgs/msg/float64_multi_array.hpp>

#include "rics_data_service_msgs/msg/mqtt_simple.hpp"

namespace iot_comm
{

class IotComm : public rclcpp::Node 
{
 public:
  IotComm();
  virtual ~IotComm();

 private:

  rclcpp::Publisher<rics_data_service_msgs::msg::MqttSimple>::SharedPtr pub_to_fms_ = nullptr;

  rclcpp::Subscription<std_msgs::msg::Int32>::SharedPtr sub_sys_state_;

  rclcpp::Subscription<nav_msgs::msg::Odometry>::SharedPtr sub_odom_ = nullptr;
  realtime_tools::RealtimeBuffer<std::shared_ptr<nav_msgs::msg::Odometry>> odom_rt_;

  rclcpp::Subscription<std_msgs::msg::Float64MultiArray>::SharedPtr sub_joint_motor_values_ = nullptr;        // 订阅关节电机信息
  realtime_tools::RealtimeBuffer<std::shared_ptr<std_msgs::msg::Float64MultiArray>> joint_motor_values_data_;

  diagnostic_updater::Updater updater_;

  void checkConnectionStatus(diagnostic_updater::DiagnosticStatusWrapper & stat);

  std::atomic<bool> is_running_{false}; // 系统运行状态

  std::atomic_int sys_state_{2};

  double scraper_length_  = 1.5; // 刮板长度

  rclcpp::TimerBase::SharedPtr update_timer_;   // 10s上报一次
  void update(void); // 上报作业数据

  void do_customInfo(); // 上报自定义信息

};

}  // namespace iot_comm

#endif  // IOT_COMM__IOT_COMM_HPP_
