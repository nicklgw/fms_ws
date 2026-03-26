
#include "iot_comm/iot_comm.hpp"
#include "iot_comm/iface.hpp"
#include "iot_comm/uuid_helper.hpp"
#include <nlohmann/json.hpp>

namespace iot_comm {

static std::string getTimeStamp(void)
{
  struct timeval time;
  gettimeofday(&time, NULL);
  std::stringstream systemTime;
  systemTime << time.tv_sec;
  systemTime << std::fixed << std::setw(3) << std::setfill('0') << time.tv_usec / 1000;
  std::string timestamp = systemTime.str();
  return timestamp;
}

IotComm::IotComm()
 : rclcpp::Node::Node("iot_comm")
 , updater_(this)
 , persist_parameter_client_(this)
{
  declare_parameter("run_frequency", 1.0);
  run_frequency_ = get_parameter("run_frequency").as_double();

  declare_parameter("iface", "eth0");
  iface_ = get_parameter("iface").as_string();

  declare_parameter("did", "ZHENGPING-001");
  did_ = get_parameter("did").as_string();

  declare_parameter("mqtt_timeout", 10.0);
  mqtt_timeout_ = get_parameter("mqtt_timeout").as_double();

  declare_parameter("software_version", "V1.0-20240101-123456");
  software_version_ = get_parameter("software_version").as_string();

  declare_parameter("scraper_length", 2.5);
  scraper_length_ = get_parameter("scraper_length").as_double();

  RCLCPP_INFO(get_logger(), "IotComm::IotComm() run_frequency: %.3f, scraper_length: %.3f, did: %s, mqtt_timeout: %.3f, software_version: %s", run_frequency_, scraper_length_, did_.c_str(), mqtt_timeout_, software_version_.c_str());

  auto task_stat_msg =  std::make_shared<iot_comm::msg::CustomInfo>();
  task_stat_msg->task_magic = 0;
  task_stat_rt_.writeFromNonRT(task_stat_msg);

  pub_to_fms_ = create_publisher<rics_data_service_msgs::msg::MqttSimple>("/rics/rics_data_to_fms", rclcpp::SystemDefaultsQoS());
  sub_from_fms_ = create_subscription<rics_data_service_msgs::msg::MqttSimple>("/rics/fms_to_robot", rclcpp::SystemDefaultsQoS(), std::bind(&IotComm::from_fms_cb, this, std::placeholders::_1));

  pub_vehicle_engage_ = create_publisher<std_msgs::msg::Bool>("/vehicle/engage", 1);

  sub_battery_state_ = create_subscription<sensor_msgs::msg::BatteryState>("/battery_state", rclcpp::SensorDataQoS().keep_last(1), std::bind(&IotComm::battery_state_cb, this, std::placeholders::_1));
  sub_exception_agg_ = create_subscription<system_diagnostic_msgs::msg::ExceptionAggregate>("/exception_aggregate", rclcpp::QoS{1}, std::bind(&IotComm::exception_agg_cb, this, std::placeholders::_1)); // 订阅异常聚集信息

  sub_sys_state_ = create_subscription<std_msgs::msg::Int32>(
    "/iot/sys_state", rclcpp::SensorDataQoS().keep_last(1),
    [this](const std_msgs::msg::Int32::SharedPtr msg) { sys_state_ = msg->data; });
  
  odom_rt_.writeFromNonRT(std::make_shared<nav_msgs::msg::Odometry>());
  sub_odom_ = create_subscription<nav_msgs::msg::Odometry>(
    "/odom", rclcpp::SystemDefaultsQoS(), 
    [this](const nav_msgs::msg::Odometry::SharedPtr msg) { odom_rt_.writeFromNonRT(msg); }); // 订阅里程计信息
  
  sub_head_down_ = create_subscription<std_msgs::msg::Bool>(
    "/head/down", rclcpp::SensorDataQoS().keep_last(1),
    [this](const std_msgs::msg::Bool::SharedPtr msg) { head_been_down_ = msg->data; });
  
  sub_vibrate_ = create_subscription<std_msgs::msg::Bool>(
    "/dinkle_io/vibrate_cmd", rclcpp::SensorDataQoS().keep_last(1),
    [this](const std_msgs::msg::Bool::SharedPtr msg) { vibrate_command_ = msg->data; });
  
  period_ = std::chrono::seconds(1) / run_frequency_; // 1Hz
  
  get_mac_ip(iface_.c_str(), mac_, ip_);
  
  // 节点每次启动，都产生一个随机数
  {
    std::default_random_engine e;
    std::uniform_int_distribution<int> u(100000,999999); // 左闭右闭区间 六位数
    e.seed(time(0));

    this_magic_ = u(e);
    RCLCPP_INFO(get_logger(), "IotComm::IotComm() this_magic: %u", this_magic_);
  }

  // 从磁盘中读取锁机请求参数
  {
    if(!persist_parameter_client_.wait_param_server_ready())
    {
      throw NoServerError();
    }
    
    std::vector<rclcpp::Parameter> parameter;
    if (persist_parameter_client_.read_parameter(PERSISTENT_LOCK_REQUEST, parameter))
    {
      for(auto & param : parameter) 
      {
        if (param.get_type() == rclcpp::ParameterType::PARAMETER_INTEGER)
        {
          remote_lock_request_ = param.as_int();
          RCLCPP_INFO(get_logger(), "remote_lock_request_: %d size: %lu", remote_lock_request_, parameter.size());
        }
      }
    }

    if (persist_parameter_client_.read_parameter(PERSISTENT_TASK_MAGIC, parameter))
    {
      for(auto & param : parameter) 
      {
        if (param.get_type() == rclcpp::ParameterType::PARAMETER_INTEGER)
        {
          task_magic_ = param.as_int();
          RCLCPP_INFO(get_logger(), "task_magic: %d size: %lu", task_magic_, parameter.size());
        }
      }
    }

    if (persist_parameter_client_.read_parameter(PERSISTENT_TASK_TIME, parameter))
    {
      for(auto & param : parameter) 
      {
        if (param.get_type() == rclcpp::ParameterType::PARAMETER_DOUBLE)
        {
          task_time_ = param.as_double();
          RCLCPP_INFO(get_logger(), "task_time: %.3f size: %lu", task_time_, parameter.size());
        }
      }
    }

    if (persist_parameter_client_.read_parameter(PERSISTENT_TASK_AREA, parameter))
    {
      for(auto & param : parameter) 
      {
        if (param.get_type() == rclcpp::ParameterType::PARAMETER_DOUBLE)
        {
          task_area_ = param.as_double();
          RCLCPP_INFO(get_logger(), "task_area: %.3f size: %lu", task_area_, parameter.size());
        }
      }
    }
  }
  RCLCPP_INFO(get_logger(), "init task_magic: %d, task_time: %.4f, task_area: %.4f", task_magic_, task_time_, task_area_);

  updater_.setHardwareID("iot_comm");
  updater_.add("connection", this, &IotComm::checkConnectionStatus);

  thread_ = std::shared_ptr<std::thread>(new std::thread(&IotComm::run_, this)); // 上传至FMS的工作序列，移至获取参数之后，以免 task_time_/task_area 还没有读取成功
}

IotComm::~IotComm()
{
  stop_();
}

void IotComm::run_()
{
  while (!do_stop_)
  {
    RCLCPP_INFO(get_logger(), "IotComm::run_()");
    
    static uint32_t count = 0;
    
    switch(count++%4)
    {
    case 0:
      do_login();
      break;
    case 1:
      do_deviceinfo();
      break;
    case 2:
      do_all_exception();
      break;
    case 3:
      do_customInfo();
      break;
    }

    do_engage();

    std::this_thread::sleep_for(period_);
  }
}

void IotComm::stop_()
{
  do_stop_ = true;
  if (thread_.get()) {
    thread_->join();
    thread_.reset();
  }
}

void IotComm::exception_agg_cb(const system_diagnostic_msgs::msg::ExceptionAggregate::SharedPtr msg)
{
  std::lock_guard<std::mutex> guard(exception_aggregate_mutex_);
  exception_aggregate_ = *msg;
}

void IotComm::do_deviceinfo()
{
  nlohmann::json root;

  std::string timestamp = getTimeStamp();

  root["MsgID"] = tier4_autoware_utils::toHexString(tier4_autoware_utils::generateUUID());
  root["DeviceCode"] = did_;
  root["IP"] = ip_;
  root["MAC"] = mac_;
  root["SysVersion"] = "Ubuntu 22.04";
  root["Voltage"] = 48.0;
  root["Current"] = 10000.0;  
  root["BatState"] = 2;
  root["BatTemp"] = 50.0;
  root["BatLevel"] = battery_remaining_.load();
  root["SysState"] = sys_state_.load(); // 2:待机, 3:运行, 4:故障, 6:暂停
  root["Location-x"] = 0.0;
  root["Location-y"] = 0.0;
  root["Location-yaw"] = 0.0;
  root["Mode"] = "Manual";
  root["RadarState"] = 3;
  root["LockAction"] = remote_lock_request_;
  root["BatAlive"] = true;
  root["Timestamp"] = timestamp;

  rics_data_service_msgs::msg::MqttSimple mqtt_msg;
  mqtt_msg.topic = std::string("device/") + did_ + std::string("/deviceinfo");
  mqtt_msg.message = root.dump();
  pub_to_fms_->publish(mqtt_msg);
}

void IotComm::do_all_exception()
{
  nlohmann::json root;

  std::string timestamp = getTimeStamp();

  root["MsgID"] = tier4_autoware_utils::toHexString(tier4_autoware_utils::generateUUID());
  root["DeviceCode"] = did_;
  root["Timestamp"] = timestamp;

  nlohmann::json err_list = nlohmann::json::array();
  {
    std::lock_guard<std::mutex> guard(exception_aggregate_mutex_);

    for (size_t i = 0; i < exception_aggregate_.status.size(); i++)
    {
      auto &err = exception_aggregate_.status[i];
      if (err.error_code != "NULL")
      {
        nlohmann::json err_item;
        err_item["Code"] = err.error_code;
        err_item["Msg"] = err.message_cn;
        err_item["Source"] = err.creator_name;
        err_item["Time"] = timestamp;
        err_item["Level"] = 0;
        err_list[i] = err_item;
      }
    }
  }
  root["ErrList"] = std::move(err_list);

  rics_data_service_msgs::msg::MqttSimple mqtt_msg;
  mqtt_msg.topic = std::string("device/") + did_ + std::string("/all-exception");
  mqtt_msg.message = root.dump();
  pub_to_fms_->publish(mqtt_msg);
}

void IotComm::do_customInfo()
{
  double delta_time = 0.0; // 本次循环作业时间
  double delta_area = 0.0; // 本次循环作业面积

  // 上装作业任务统计
  {
    static bool last_vibrate_command = false;
    static double last_odometer = 0.0;

    auto odom = *(odom_rt_.readFromRT());
    double odometer = std::hypot(fabs(odom->pose.pose.position.x), fabs(odom->pose.pose.position.y));

    if (!last_vibrate_command && vibrate_command_)
    {
      last_odometer = odometer;
    }

    if (vibrate_command_)
    {
      delta_time = period_.count(); // 每次加4s
      delta_area = fabs(odometer - last_odometer) * scraper_length_;
      last_odometer = odometer;
    }

    last_vibrate_command = vibrate_command_;
  }

  {
    static double task_time_start = task_time_;
    static double task_area_start = task_area_;

    if (this_magic_ != task_magic_) // 不相等，相当于task_control重新开始统计
    {
      task_magic_ = this_magic_;
  
      task_time_start = task_time_;
      task_area_start = task_area_;
    }
    else 
    {
      this_time_ += delta_time;
      this_area_ += delta_area;
      task_time_ = task_time_start + this_time_;
      task_area_ = task_area_start + this_area_;
    }

    persist_parameter_client_.async_set_parameter(PERSISTENT_TASK_MAGIC, task_magic_); // 操作两次，才能写到磁盘文件中
    persist_parameter_client_.async_set_parameter(PERSISTENT_TASK_MAGIC, task_magic_); // 操作两次，才能写到磁盘文件中
  
    persist_parameter_client_.async_set_parameter(PERSISTENT_TASK_TIME, task_time_); // 操作两次，才能写到磁盘文件中
    persist_parameter_client_.async_set_parameter(PERSISTENT_TASK_TIME, task_time_); // 操作两次，才能写到磁盘文件中
  
    persist_parameter_client_.async_set_parameter(PERSISTENT_TASK_AREA, task_area_); // 操作两次，才能写到磁盘文件中
    persist_parameter_client_.async_set_parameter(PERSISTENT_TASK_AREA, task_area_); // 操作两次，才能写到磁盘文件中
  
    RCLCPP_INFO(get_logger(), "loop task_magic: %d, task_time: %.4f, task_area: %.4f", task_magic_, task_time_, task_area_);

    nlohmann::json root;

    std::string timestamp = getTimeStamp();

    root["MsgID"] = tier4_autoware_utils::toHexString(tier4_autoware_utils::generateUUID());
    root["Timestamp"] = timestamp;
    root["DeviceCode"] = did_;

    nlohmann::json data;

    data["manualMode"] = 1; // 手动控制模式 0:普通模式；1:作业模式
    data["rayMode"] = head_been_down_.load(); // 激光调平模式 0:手动；1:自动
    data["angleMode"] = head_been_down_.load(); // 倾角调平模式 0:手动；1:自动
    data["workState"] = head_been_down_.load(); // 上装状态 0:未工作；1:工作
    data["totalWorkTime"] = task_time_ / 60.0 / 60.0; // 累计作业时间
    data["workArea"] = delta_area; // 本次循环作业面积
    data["totalArea"] = task_area_; // 累计作业面积
    data["vibrateState"] = vibrate_command_.load(); // 振捣状态 0:关闭；1:开启
    data["thisMagic"] = this_magic_; // 本次开机，生成随机数
    data["thisTime"] = this_time_ / 60.0 / 60.0; // 本次开机，作业时间
    data["thisArea"] = this_area_; // 本次开机，作业面积

    root["Data"] = data;

    rics_data_service_msgs::msg::MqttSimple mqtt_msg;
    mqtt_msg.topic = std::string("device/") + did_ + std::string("/customInfo");
    mqtt_msg.message = root.dump();
    pub_to_fms_->publish(mqtt_msg);
  }
}

void IotComm::do_engage()
{
  std_msgs::msg::Bool engage;
  engage.data = (remote_lock_request_ != LOCK_ACTION_LOCK); // 不是锁机状态时,均为离合器已结合 engage=true
  pub_vehicle_engage_->publish(engage);
}

void IotComm::do_login()
{
  if(connect_login_ack_response_code_ == 0) // 如果上报成功
    return;
  
  std::string timestamp = getTimeStamp();

  nlohmann::json root;
  
  nlohmann::json header;
  header["msgId"] = tier4_autoware_utils::toHexString(tier4_autoware_utils::generateUUID());
  header["signature"] = "";
  header["timestamp"] = timestamp;
  
  nlohmann::json body;
  nlohmann::json data;
  data["architecture"] = "aarch64"; // 硬件架构X86或ARM
  data["encodingRule"] = "json"; // 编码规则，json,protobuf,默认json
  data["iccidOfRouter"] = "89860622330049671431"; // 4G卡iccid
  data["lockState"] = remote_lock_request_; // 锁机状态，1:已锁定，2:未锁定
  data["protocol"] = "1.3"; // 协议版本
  data["softwareVersion"] = software_version_; // 软件版本, 如 V1.0-20241023-160516
  data["sysVersion"] = "Ubuntu 22.04"; // 系统版本
  
  body["data"] = data;
  root["body"] = body;
  root["header"] = header;
  
  rics_data_service_msgs::msg::MqttSimple mqtt_msg;
  mqtt_msg.topic = std::string("device/") + did_ + std::string("/login");
  mqtt_msg.message = root.dump();
  pub_to_fms_->publish(mqtt_msg);
}

void IotComm::connect_login_ack_cb(const std_msgs::msg::String::SharedPtr msg)
{
  nlohmann::json root = nlohmann::json::parse(msg->data.c_str());

  int response_code = root["response"]["code"];
  connect_login_ack_response_code_ = response_code;
}

void IotComm::connect_lock_cb(const std_msgs::msg::String::SharedPtr msg)                                             // 锁机请求回调
{
  std::string timestamp = getTimeStamp();
  std::string MsgID;

  {
    nlohmann::json root = nlohmann::json::parse(msg->data.c_str());

    MsgID = root["MsgID"];
    int lock_action = root["Action"];
    RCLCPP_INFO(get_logger(), "MsgID: %s, lock_action: %d", MsgID.c_str(), lock_action);

    // "Action": 1, // 1：锁机；2: 解锁
    remote_lock_request_ = lock_action;

    persist_parameter_client_.async_set_parameter(PERSISTENT_LOCK_REQUEST, remote_lock_request_); // 操作两次，才能写到磁盘文件中
    persist_parameter_client_.async_set_parameter(PERSISTENT_LOCK_REQUEST, remote_lock_request_); // 操作两次，才能写到磁盘文件中

    RCLCPP_INFO(get_logger(), "remote_lock_request: %d", remote_lock_request_);
  }

  // 机器人响应lock指令
  {
    nlohmann::json root;
    root["DeviceCode"] = did_;
    root["Result"] = 1;
    root["ReturnMsg"] = "";
    root["MsgID"] = MsgID;
    root["Timestamp"] = timestamp;

    rics_data_service_msgs::msg::MqttSimple mqtt_msg;
    mqtt_msg.topic = std::string("device/") + did_ + std::string("/lock-ack");
    mqtt_msg.message = root.dump();
    pub_to_fms_->publish(mqtt_msg);
  }

  // 机器人上报锁机/解锁结果
  {
    nlohmann::json root;
    root["DeviceCode"] = did_;
    root["Event"] = (remote_lock_request_ == LOCK_ACTION_LOCK) ? "LOCK_SUCCESS" : "UNLOCK_SUCCESS";
    root["Msg"] = (remote_lock_request_ == LOCK_ACTION_LOCK) ? "锁机成功" : "解锁成功";
    root["MsgID"] = MsgID;
    root["Timestamp"] = timestamp;

    rics_data_service_msgs::msg::MqttSimple mqtt_msg;
    mqtt_msg.topic = std::string("device/") + did_ + std::string("/notify");
    mqtt_msg.message = root.dump();
    pub_to_fms_->publish(mqtt_msg);
  }
}

void IotComm::battery_state_cb(const sensor_msgs::msg::BatteryState::SharedPtr msg)
{
  battery_remaining_ = msg->percentage * 100.0; // 百分比
}

void IotComm::from_fms_cb(const rics_data_service_msgs::msg::MqttSimple::SharedPtr msg)
{
  std::string topic = msg->topic;
  std_msgs::msg::String::SharedPtr payload = std::make_shared<std_msgs::msg::String>();
  payload->data = msg->message;

  std::string login_ack = std::string("command/") + did_ + std::string("/login-ack");

  if (topic == login_ack)
  {
    connect_login_ack_cb(payload);
  }

  std::string lock = std::string("connect/") + did_ + std::string("/lock");

  if (topic == lock)
  {
    connect_lock_cb(payload);
  }
}

void IotComm::checkConnectionStatus(diagnostic_updater::DiagnosticStatusWrapper & stat)
{
  stat.add<bool>("mqtt_client_ready", mqtt_client_ready_);
  stat.add<bool>("mqtt_is_connected", mqtt_is_connected_);

  int8_t diag_level = diagnostic_msgs::msg::DiagnosticStatus::OK;
  std::string diag_message = "Connection is OK.";
  
  stat.name = "connection";
  stat.hardware_id = "connection";
  stat.level = diag_level;
  stat.message = diag_message;
}

}  // namespace iot_comm
