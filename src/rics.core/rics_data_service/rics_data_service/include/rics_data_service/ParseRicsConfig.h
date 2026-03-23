#ifndef RICS_DATA_SERVICE_CONFIG_STORAGE_H_
#define RICS_DATA_SERVICE_CONFIG_STORAGE_H_

#include <rics_data_service/IAdapter.h>
#include <rics_data_service/Logging.h>
#include <rics_data_service/RicsDefines.h>
#include <rics_data_service/data_collect_service/domain/Defines.h>

#include <ament_index_cpp/get_package_share_directory.hpp>
#include <boost/any.hpp>
#include <rclcpp/rclcpp.hpp>
#include <rclcpp_lifecycle/lifecycle_node.hpp>

namespace rics {

using EventHandler = std::function<boost::any(const boost::any&)>;
using ApplyHandler = std::function<void(const boost::any&)>;
using TypeId = boost::typeindex::stl_type_index;

class ParseRicsConfig : public rics::IAdapter {
 public:
  // 构造函数参数改为LifecycleNode
  ParseRicsConfig(std::shared_ptr<rclcpp_lifecycle::LifecycleNode> node);
  ~ParseRicsConfig();

  void LoadFromRosParams();

  boost::any GetAny(TypeId type) override;

  /**
   * @brief 获取服务启用状态（enable_service参数值）
   * @return true-启用服务，false-禁用服务
   */
  bool GetEnableService() const;

 private:
  std::string FullConfigPath(const std::string& relative_path);

  void declareParams();
  void LoadRicsConfigParams();
  void LoadRobotConfigParams();
  void LoadDefaultParamConfig();
  void LoadMqttConfig();
  void LoadTopicsConfig();
  void LoadDataCollectConfig();

  RicsBusinessOption m_ricsBusinessOption;
  RicsTransportOption m_transportOption;
  DataCollectOption m_dataCollectOption;
  std::string m_strConfigFile;
  // 成员变量改为LifecycleNode类型
  std::shared_ptr<rclcpp_lifecycle::LifecycleNode> m_node;

  bool m_enable_service{true};
};

}  // namespace rics

#endif  // RICS_DATA_SERVICE_CONFIG_STORAGE_H_