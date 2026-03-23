
#include "iot_comm/iot_comm.hpp"
#include "rclcpp/rclcpp.hpp"

void __attribute__((constructor)) print_version() 
{
  printf("task_control BUILD_TIME: %s, GIT_HASH: %s, GIT_BRANCH: %s \n", BUILD_TIME, GIT_HASH, GIT_BRANCH);
}

int main(int argc, char *argv[]) 
{
  rclcpp::init(argc, argv);

  auto node = std::make_shared<iot_comm::IotComm>();

  rclcpp::spin(node);

  rclcpp::shutdown();

  return 0;
}
