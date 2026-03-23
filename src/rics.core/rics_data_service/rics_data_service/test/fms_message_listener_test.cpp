#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>

#include <iostream>
#include <string>

class FmsMessageListenerTest : public rclcpp::Node {
 public:
  FmsMessageListenerTest() : Node("fms_message_listener_test") {
    // 订阅 FMS 消息的 ROS 2 topic
    subscription_ = this->create_subscription<std_msgs::msg::String>(
        "/rics/fms_to_robot", 10, 
        std::bind(&FmsMessageListenerTest::topic_callback, this, std::placeholders::_1));
    
    RCLCPP_INFO(this->get_logger(), "FMS Message Listener Test started");
    RCLCPP_INFO(this->get_logger(), "Subscribed to topic: /rics/fms_to_robot");
    RCLCPP_INFO(this->get_logger(), "Waiting for FMS messages...");
  }

 private:
  void topic_callback(const std_msgs::msg::String::SharedPtr msg) {
    RCLCPP_INFO(this->get_logger(), "Received FMS message:");
    RCLCPP_INFO(this->get_logger(), "  Data: %s", msg->data.c_str());
  }

  rclcpp::Subscription<std_msgs::msg::String>::SharedPtr subscription_;
};

int main(int argc, char * argv[]) {
  rclcpp::init(argc, argv);
  auto node = std::make_shared<FmsMessageListenerTest>();
  
  // 运行 60 秒后自动退出
  rclcpp::executors::MultiThreadedExecutor executor;
  executor.add_node(node);
  
  // 创建一个定时器，60 秒后退出
  auto timer = node->create_wall_timer(
      std::chrono::seconds(60),
      [&]() {
        RCLCPP_INFO(node->get_logger(), "Test completed after 60 seconds");
        rclcpp::shutdown();
      });
  
  executor.spin();
  return 0;
}