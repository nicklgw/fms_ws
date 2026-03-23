#include <chrono>
#include <iostream>
#include <memory>

#include "rics_data_service/rics_data_service.hpp"

int main(int argc, char** argv) {
  std::cout << "RicsDataService starting..." << std::endl;

  // 初始化ROS2
  rclcpp::init(argc, argv);

  // 创建节点选项和LifecycleNode实例
  rclcpp::NodeOptions options;
  auto service = std::make_shared<rics::data::RicsDataService>(options);

  // 创建多线程执行器并添加节点
  rclcpp::executors::MultiThreadedExecutor executor;
  executor.add_node(service->get_node_base_interface());

  // 运行执行器
  executor.spin();

  // 清理资源
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  rclcpp::shutdown();

  return 0;
}