#pragma once

#include <fmt/format.h>

#include <memory>
#include <mutex>
#include <rcl_interfaces/srv/get_parameters.hpp>
#include <rclcpp/rclcpp.hpp>

namespace rics {

class ParameterLoader {
 public:
  // 构造函数：允许传入节点，若未传入则创建默认节点
  explicit ParameterLoader(rclcpp::Node::SharedPtr node = nullptr) : node_(node) {
    if (!node_) {
      node_ = std::make_shared<rclcpp::Node>("robot_param_loader");
    }
    executor_ = std::make_shared<rclcpp::executors::SingleThreadedExecutor>();
    executor_->add_node(node_);
  }

  ~ParameterLoader() {
    // std::cout << "ParameterLoader destructor called" << std::endl;
    executor_->remove_node(node_);
  }

  // 静态工厂方法：创建 ParameterLoader 实例
  static std::shared_ptr<ParameterLoader> create(rclcpp::Node::SharedPtr node = nullptr) {
    return std::make_shared<ParameterLoader>(node);
  }

  template <typename T>
  T load_once(const std::string& node_name, const std::string& param_name, const T& default_value,
              std::chrono::nanoseconds timeout = std::chrono::seconds(2)) {
    return do_load(node_name, param_name, default_value, timeout);
  }

 private:
  template <typename T>
  T do_load(const std::string& node_name, const std::string& param_name, const T& default_value,
            std::chrono::nanoseconds timeout) {
    std::lock_guard<std::mutex> lock(load_mutex_);

    auto client = node_->create_client<rcl_interfaces::srv::GetParameters>(
        fmt::format("{}/get_parameters", node_name));

    if (!client->wait_for_service(timeout)) {
      RCLCPP_WARN(node_->get_logger(), "Service %s unavailable", client->get_service_name());
      return default_value;
    }

    auto request = std::make_shared<rcl_interfaces::srv::GetParameters::Request>();
    request->names = {param_name};

    auto future = client->async_send_request(request);

    if (executor_->spin_until_future_complete(future, timeout) !=
        rclcpp::FutureReturnCode::SUCCESS) {
      RCLCPP_WARN(node_->get_logger(), "Timeout loading %s", param_name.c_str());
      return default_value;
    }

    try {
      return rclcpp::ParameterValue(future.get()->values[0]).get<T>();
    } catch (...) {
      return default_value;
    }
  }

  rclcpp::Node::SharedPtr node_;
  rclcpp::Executor::SharedPtr executor_;
  std::mutex load_mutex_;
};

}  // namespace rics