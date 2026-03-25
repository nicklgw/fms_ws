cmake_minimum_required(VERSION 3.8)
project(rics_data_service)

# Default c++ standard
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 20)
endif()

set(CMP_NEED_NEW_LIST
  CMP0057
  CMP0007
  CMP0012
)

foreach(CMP IN ITEMS ${CMP_NEED_NEW_LIST})
  if (POLICY ${CMP})
    cmake_policy(SET ${CMP} NEW)
  endif()
endforeach()


if (POLICY CMP0057)
  cmake_policy(SET CMP0057 NEW)
endif()

# 设置编译器警告和C++标准
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

set(dependencies
  rclcpp
  ament_index_cpp 
  rclcpp_lifecycle
  lifecycle_msgs
  rics_data_service_msgs
  diagnostic_msgs
  diagnostic_updater
  rclcpp_components
)

find_package(ament_cmake REQUIRED)
foreach(dependency IN ITEMS ${dependencies})
  find_package(${dependency} REQUIRED)
endforeach()

# 查找 Poco 并声明 Net 和 NetSSL 组件
find_package(Poco REQUIRED COMPONENTS Net NetSSL Foundation)
find_package(Boost REQUIRED COMPONENTS system thread filesystem iostreams)
find_package(nlohmann_json REQUIRED)

# 收集源文件
file(GLOB_RECURSE ${PROJECT_NAME}_SRC
  ${CMAKE_CURRENT_LIST_DIR}/src/*.cpp
  ${CMAKE_CURRENT_LIST_DIR}/src/data_center/*.cpp
  ${CMAKE_CURRENT_LIST_DIR}/src/data_collect_service/*.cpp
  ${CMAKE_CURRENT_LIST_DIR}/src/fms_adapter/*.cpp
)
list(FILTER ${PROJECT_NAME}_SRC EXCLUDE REGEX ".*/mock_service_main\\.cpp$")

# 创建共享库
add_library(${PROJECT_NAME} SHARED ${${PROJECT_NAME}_SRC})

# 链接依赖项
target_link_libraries(${PROJECT_NAME}
  PUBLIC            
    Poco::Net
    Poco::NetSSL
    Poco::Foundation
    Boost::system
    Boost::thread
    Boost::filesystem
    Boost::iostreams
    mosquitto
    jsoncpp
    nlohmann_json::nlohmann_json
)

# 添加包含路径
target_include_directories(${PROJECT_NAME}
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
  PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
)

# 注册组件依赖
ament_target_dependencies(${PROJECT_NAME} PUBLIC ${dependencies})


# 创建可执行文件
add_executable(${PROJECT_NAME}_executable src/mock_service_main.cpp)
target_link_libraries(${PROJECT_NAME}_executable
  PRIVATE ${PROJECT_NAME}
)
ament_target_dependencies(${PROJECT_NAME}_executable PUBLIC ${dependencies})

# 注册组件节点到共享库，移除EXECUTABLE参数或修正名称
rclcpp_components_register_node(${PROJECT_NAME}
  PLUGIN "rics::data::RicsDataService"
  EXECUTABLE ${PROJECT_NAME}_node
)

# ament_index_register_resource(rics_service_register CONTENT "share/${PROJECT_NAME}/config/rics_services.xml")

install(
  TARGETS ${PROJECT_NAME}_executable
  RUNTIME DESTINATION lib/${PROJECT_NAME}
)

# 安装配置
install(
  FILES rics_services.xml
  DESTINATION share/${PROJECT_NAME}/config
)

install(
  TARGETS ${PROJECT_NAME}
  EXPORT export_${PROJECT_NAME}
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

# Install config files.
install(DIRECTORY config
  DESTINATION share/${PROJECT_NAME})

install(DIRECTORY launch
  DESTINATION share/${PROJECT_NAME})


# 测试支持
if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()
  
  # FMS 锁机测试
  add_executable(fms_lock_test test/fms_lock_test.cpp)
  target_link_libraries(fms_lock_test
    PRIVATE ${PROJECT_NAME}
    nlohmann_json::nlohmann_json
  )
  ament_target_dependencies(fms_lock_test PUBLIC ${dependencies})
  
  # FMS 消息监听器测试
  add_executable(fms_message_listener_test test/fms_message_listener_test.cpp)
  target_link_libraries(fms_message_listener_test
    PRIVATE ${PROJECT_NAME}
  )
  ament_target_dependencies(fms_message_listener_test PUBLIC ${dependencies})
  
  install(TARGETS fms_lock_test fms_message_listener_test
    RUNTIME DESTINATION lib/${PROJECT_NAME}
  )
endif()

ament_package()
