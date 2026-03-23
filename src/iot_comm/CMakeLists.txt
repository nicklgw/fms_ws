cmake_minimum_required(VERSION 3.10)
project(iot_comm)

execute_process(COMMAND date "+%F %T%z" OUTPUT_VARIABLE BUILD_TIME OUTPUT_STRIP_TRAILING_WHITESPACE WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}) 
execute_process(COMMAND git log -1 --pretty=format:%h OUTPUT_VARIABLE GIT_HASH OUTPUT_STRIP_TRAILING_WHITESPACE WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}) 
execute_process(COMMAND git symbolic-ref --short -q HEAD OUTPUT_VARIABLE GIT_BRANCH OUTPUT_STRIP_TRAILING_WHITESPACE WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
add_definitions(-DBUILD_TIME=\"${BUILD_TIME}\" -DGIT_HASH=\"${GIT_HASH}\" -DGIT_BRANCH=\"${GIT_BRANCH}\")

# Compiler settings
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 20)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Dependencies
find_package(ament_cmake REQUIRED)
find_package(ament_index_cpp REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(nav_msgs REQUIRED)
find_package(diagnostic_msgs REQUIRED)
find_package(diagnostic_updater REQUIRED)
find_package(nlohmann_json 3.6.0 REQUIRED)
find_package(system_diagnostic_msgs REQUIRED)
find_package(unique_identifier_msgs REQUIRED)
find_package(mqtt_client_interfaces REQUIRED)
find_package(realtime_tools REQUIRED)
find_package(rics_data_service_msgs REQUIRED)

# Generate the interfaces
find_package(rosidl_default_generators REQUIRED)
set(msg_files
  "msg/CustomInfo.msg"
  DEPENDENCIES std_msgs
)
rosidl_generate_interfaces(${PROJECT_NAME}
  ${msg_files}
)
rosidl_get_typesupport_target(cpp_typesupport_target ${PROJECT_NAME} rosidl_typesupport_cpp)
ament_export_dependencies(rosidl_default_runtime)

# Logic shared by all targets
include_directories(
  include
)

set(project_LIBRARIES
  ${rclcpp_LIBRARIES}
  ${cpp_typesupport_target}
)

set(project_DEPENDENCIES
  ament_index_cpp
  rclcpp
  std_msgs
  sensor_msgs
  nav_msgs
  diagnostic_msgs
  diagnostic_updater
  nlohmann_json
  system_diagnostic_msgs
  unique_identifier_msgs
  mqtt_client_interfaces
  realtime_tools
  rics_data_service_msgs
)

# Executable target
add_executable(${PROJECT_NAME}_node src/main.cpp src/iot_comm.cpp src/persist_parameter_client.cpp)
target_link_libraries(${PROJECT_NAME}_node ${project_LIBRARIES})
ament_target_dependencies(${PROJECT_NAME}_node ${project_DEPENDENCIES})
install(TARGETS
  ${PROJECT_NAME}_node
  DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY
  config
  launch
  buffer
  DESTINATION share/${PROJECT_NAME}/
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  # the following line skips the linter which checks for copyrights
  # comment the line when a copyright and license is added to all source files
  set(ament_cmake_copyright_FOUND TRUE)
  # the following line skips cpplint (only works in a git repo)
  # comment the line when this package is in a git repo and when
  # a copyright and license is added to all source files
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
