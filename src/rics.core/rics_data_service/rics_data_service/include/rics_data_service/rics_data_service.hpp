#include <rics_data_service/TransportFactory.h>

#include <diagnostic_updater/diagnostic_updater.hpp>
#include <rclcpp_lifecycle/lifecycle_node.hpp>
#include <rclcpp_lifecycle/lifecycle_publisher.hpp>

#include "RicsDefines.h"
#include "data_collect_service/adapter/DataCollectFactory.h"
#include "fms_adapter/FmsMessageListener.h"
#include "rics_data_service/HttpsHelper.h"
#include "rics_data_service/ParseRicsConfig.h"
#include "rics_data_service/RicsUtils.h"

namespace rics::data {

using namespace rics::data_collect;

class RicsDataService : public rclcpp_lifecycle::LifecycleNode {
 public:
  RicsDataService(const rclcpp::NodeOptions& options);
  ~RicsDataService() override;

  // LifecycleNode标准生命周期回调
  rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn on_configure(
      const rclcpp_lifecycle::State& previous_state);

  rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn on_activate(
      const rclcpp_lifecycle::State& previous_state);

  rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn on_deactivate(
      const rclcpp_lifecycle::State& previous_state);

  rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn on_cleanup(
      const rclcpp_lifecycle::State& previous_state);

  rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn on_shutdown(
      const rclcpp_lifecycle::State& previous_state);

  void diagnostic_update(diagnostic_updater::DiagnosticStatusWrapper& stat);

 private:
  // 初始化辅助函数
  void configure();
  void activate();

 private:
  std::shared_ptr<ParseRicsConfig> pParseRicsConfig_;
  std::shared_ptr<DataCollectFactory> pDataCollectFactory_;
  std::shared_ptr<ITransport> pRicsTransport_;
  std::unique_ptr<rics::fms::FmsMessageListener> pFmsMessageListener_;

  rclcpp::TimerBase::SharedPtr init_timer_;
  diagnostic_updater::Updater diagnostic_updater_;
};

}  // namespace rics::data