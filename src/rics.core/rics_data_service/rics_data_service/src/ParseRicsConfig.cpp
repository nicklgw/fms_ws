#include <rics_data_service/JsonHelper.h>
#include <rics_data_service/Logging.h>
#include <rics_data_service/ParseRicsConfig.h>
#include <rics_data_service/RicsDefines.h>
#include <rics_data_service/RicsUtils.h>

#include <ament_index_cpp/get_package_share_directory.hpp>
#include <boost/format.hpp>
#include <fstream>
#include <rclcpp/rclcpp.hpp>
#include <regex>
#include <rics_data_service/rics_robot_help.hpp>
#include <sstream>
#include <stdexcept>
#include <string>

namespace rics {

ParseRicsConfig::ParseRicsConfig(std::shared_ptr<rclcpp_lifecycle::LifecycleNode> node)
    : m_node(node) {
  declareParams();
  LoadFromRosParams();
}

ParseRicsConfig::~ParseRicsConfig() {}

void ParseRicsConfig::declareParams() {
  std::string package_path = ament_index_cpp::get_package_share_directory("rics_data_service");
  std::string config_path;
  if (!package_path.empty()) {
    config_path = package_path + "/config/";
  }

  // 声明enable_service参数
  m_node->declare_parameter("enable_service", true);

  m_node->declare_parameter("Protocol", 13);
  m_node->declare_parameter("TransportType", "Mqtt");
  m_node->declare_parameter("DefaultPath", config_path);
  m_node->declare_parameter("FmsEnvUrl",
                            "https://fms.bzlrobot.com/v1/system/fms-env?env=prod,test");
  m_node->declare_parameter("SslPrivateKey", "secret");

  m_node->declare_parameter("RobotConfig.SerialNumber", "");
  m_node->declare_parameter("RobotConfig.VerifyFormat", false);
  m_node->declare_parameter("RobotConfig.syncTime", false);
  m_node->declare_parameter("RobotConfig.RobotType", "");
  m_node->declare_parameter("RobotConfig.RobotVersion", "");
  m_node->declare_parameter("RobotConfig.MFGD", "");
  m_node->declare_parameter("RobotConfig.ICCID", "");
  m_node->declare_parameter("MqttConfig.Host", "202.104.27.142");
  m_node->declare_parameter("MqttConfig.Port", 2884);

  m_node->declare_parameter("DefaultParameter.ReportPeriod", 3000);
  m_node->declare_parameter("DefaultParameter.RetryTime", 3);
  m_node->declare_parameter("DefaultParameter.RetryPeriod", 5000);
  m_node->declare_parameter("DefaultParameter.RadarUploadSwitch", 1);
  m_node->declare_parameter("DefaultParameter.SignatureSupport", 0);
  m_node->declare_parameter("DefaultParameter.ExtendData", "");

  m_node->declare_parameter("MqttConfig.Version", 3);
  m_node->declare_parameter("MqttConfig.Qos", 1);
  m_node->declare_parameter("MqttConfig.Insecure", true);
  m_node->declare_parameter("MqttConfig.CleanSession", true);
  m_node->declare_parameter("MqttConfig.MaxInflight", 20);
  m_node->declare_parameter("MqttConfig.KeepAlive", 6);
  m_node->declare_parameter("MqttConfig.ReConnectTime", 10);
  m_node->declare_parameter("MqttConfig.Cafile", "ca.pem");
  m_node->declare_parameter("MqttConfig.UserName", "");
  m_node->declare_parameter("MqttConfig.Password", "");

  m_node->declare_parameter("TopicsConfig.Qos0Topics",
                            std::vector<std::string>{"device/+/deviceinfo"});
  m_node->declare_parameter("TopicsConfig.CacheTopics",
                            std::vector<std::string>{"*", "-device/+/location"});

  m_node->declare_parameter("DataCollectConfig.CacheFilePath", "/var/rics/data/");
  m_node->declare_parameter("DataCollectConfig.DiskFreePercent", 1);
  m_node->declare_parameter("DataCollectConfig.UploadFmsServer.Host", "fms.bzlrobot.com");
  m_node->declare_parameter("DataCollectConfig.UploadFmsServer.Port", 443);
  m_node->declare_parameter("DataCollectConfig.RegisterFmsServer.Usrname", "fms");
  m_node->declare_parameter("DataCollectConfig.RegisterFmsServer.Key",
                            "I6wTzwoM2oCMUlkkyLu3YRJFd6HtGfAM");
}

void ParseRicsConfig::LoadFromRosParams() {
  LoadRicsConfigParams();
  LoadRobotConfigParams();
  LoadDefaultParamConfig();
  LoadMqttConfig();
  LoadTopicsConfig();
  LoadDataCollectConfig();

  m_transportOption.nProtocol = m_ricsBusinessOption.nProtocol;
  m_transportOption.strType = m_ricsBusinessOption.strTransportType;
  m_transportOption.strSN = m_ricsBusinessOption.robotConfig.strSN;
  for (auto& iter : m_transportOption.mqttOptions) {
    iter.m_lstQos0Topic.assign(m_ricsBusinessOption.topicsConfig.lstQos0Topic.begin(),
                               m_ricsBusinessOption.topicsConfig.lstQos0Topic.end());
  }
}

void ParseRicsConfig::LoadRicsConfigParams() {
  // 加载enable_service参数
  m_node->get_parameter("enable_service", m_enable_service);
  RICS_INFO("ParseRicsConfig: enable_service = %s", m_enable_service ? "true" : "false");

  m_node->get_parameter("Protocol", m_ricsBusinessOption.nProtocol);
  m_node->get_parameter("TransportType", m_ricsBusinessOption.strTransportType);
  m_node->get_parameter("DefaultPath", m_ricsBusinessOption.strDefaultPath);
  m_node->get_parameter("FmsEnvUrl", m_ricsBusinessOption.strFmsEnvUrl);
  m_node->get_parameter("SslPrivateKey", m_ricsBusinessOption.strPrivateKey);

  RicsUtils::FormatPath(m_ricsBusinessOption.strDefaultPath);
  if (!RicsUtils::RecurseCreateDir(m_ricsBusinessOption.strDefaultPath,
                                   S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)) {
    std::string strError = "create dir " + m_ricsBusinessOption.strDefaultPath + " fail!";
    throw std::runtime_error(strError.c_str());
  }
}

void ParseRicsConfig::LoadRobotConfigParams() {
  // 1. 优先从 ROS 参数加载序列号
  m_node->get_parameter("RobotConfig.SerialNumber", m_ricsBusinessOption.robotConfig.strSN);

  // 2. 如果参数获取的序列号为空，尝试从 ParameterLoader 获取
  if (m_ricsBusinessOption.robotConfig.strSN.empty()) {
    try {
      auto loader = rics::ParameterLoader::create();
      const std::string ROBOT_SN =
          loader->load_once<std::string>("/rics/main_controller", "serial_number", "");
      if (!ROBOT_SN.empty()) {
        m_ricsBusinessOption.robotConfig.strSN = ROBOT_SN;
        RICS_INFO("Robot SN loaded from ParameterLoader: %s", ROBOT_SN.c_str());
      } else {
        RICS_WARN("Robot SN is empty in both ROS params and ParameterLoader");
      }
    } catch (const std::exception& ex) {
      RICS_WARN("get sn from ParameterLoader failed: %s", ex.what());
    }
  } else {
    RICS_INFO("Robot SN loaded from ROS params: %s",
              m_ricsBusinessOption.robotConfig.strSN.c_str());
  }

  // 加载其他机器人配置参数
  m_node->get_parameter("RobotConfig.RobotType", m_ricsBusinessOption.robotConfig.strRobotType);
  m_node->get_parameter("RobotConfig.RobotVersion",
                        m_ricsBusinessOption.robotConfig.strRobotVersion);
  m_node->get_parameter("RobotConfig.MFGD", m_ricsBusinessOption.robotConfig.strMFGD);
  m_node->get_parameter("MqttConfig.Host", m_ricsBusinessOption.fmsAddress.host);
  m_node->get_parameter("MqttConfig.Port", m_ricsBusinessOption.fmsAddress.port);
  m_node->get_parameter("RobotConfig.VerifyFormat", m_ricsBusinessOption.robotConfig.bVerifyFormat);
  m_node->get_parameter("RobotConfig.syncTime", m_ricsBusinessOption.robotConfig.syncTime);
  m_node->get_parameter("RobotConfig.ICCID", m_ricsBusinessOption.robotConfig.nICCID);
}

void ParseRicsConfig::LoadDefaultParamConfig() {
  m_node->get_parameter("DefaultParameter.ReportPeriod",
                        m_ricsBusinessOption.defaultParam.reportPeriod);
  m_node->get_parameter("DefaultParameter.RetryTime", m_ricsBusinessOption.defaultParam.retryTime);
  m_node->get_parameter("DefaultParameter.RetryPeriod",
                        m_ricsBusinessOption.defaultParam.retryPeriod);
  m_node->get_parameter("DefaultParameter.RadarUploadSwitch",
                        m_ricsBusinessOption.defaultParam.radarUploadSwitch);
  m_node->get_parameter("DefaultParameter.SignatureSupport",
                        m_ricsBusinessOption.defaultParam.signatureSupport);
  m_node->get_parameter("DefaultParameter.ExtendData",
                        m_ricsBusinessOption.defaultParam.extendData);
}

void ParseRicsConfig::LoadMqttConfig() {
  MqttOption option;
  m_node->get_parameter("MqttConfig.Version", option.m_nVersion);
  m_node->get_parameter("MqttConfig.Host", option.m_strHost);
  m_node->get_parameter("MqttConfig.Port", option.m_nPort);
  m_node->get_parameter("MqttConfig.Qos", option.m_nQos);
  m_node->get_parameter("MqttConfig.Insecure", option.m_bInsecure);
  m_node->get_parameter("MqttConfig.CleanSession", option.m_bCleanSession);
  m_node->get_parameter("MqttConfig.MaxInflight", option.m_nMaxInflight);
  m_node->get_parameter("MqttConfig.KeepAlive", option.m_nKeepAlive);
  m_node->get_parameter("MqttConfig.ReConnectTime", option.m_nReConnectTime);
  m_node->get_parameter("MqttConfig.UserName", option.m_strUserName);
  m_node->get_parameter("MqttConfig.Password", option.m_strPassword);

  std::string cafile;
  m_node->get_parameter("MqttConfig.Cafile", cafile);
  option.m_strCafile = FullConfigPath(cafile);

  m_transportOption.mqttOptions.emplace_back(option);
}

void ParseRicsConfig::LoadTopicsConfig() {
  auto correctTopic = [this](const std::string& topic) {
    std::string realTopic = topic;
    auto firstPos = topic.find_first_of("/");
    auto secondPos = topic.find_last_of("/");
    if (firstPos != std::string::npos && secondPos != std::string::npos && firstPos != secondPos) {
      auto sn = topic.substr(firstPos + 1, secondPos - firstPos - 1);
      if (sn != m_ricsBusinessOption.robotConfig.strSN) {
        realTopic = realTopic.replace(firstPos + 1, secondPos - firstPos - 1,
                                      m_ricsBusinessOption.robotConfig.strSN);
      }
    }
    return realTopic;
  };

  std::vector<std::string> arrQos0Topics;
  m_node->get_parameter("TopicsConfig.Qos0Topics", arrQos0Topics);

  std::vector<std::string> arrCacheTopics;
  m_node->get_parameter("TopicsConfig.CacheTopics", arrCacheTopics);

  for (const auto& topic : arrQos0Topics) {
    m_ricsBusinessOption.topicsConfig.lstQos0Topic.push_back(correctTopic(topic));
  }
  for (const auto& topic : arrCacheTopics) {
    auto realTopic = correctTopic(topic);
    if (realTopic.length() == 0) {
      continue;
    }
    if (realTopic[0] == '-') {
      m_ricsBusinessOption.topicsConfig.lstExcludeTopic.push_back(
          realTopic.substr(1, realTopic.length() - 1));
    } else {
      m_ricsBusinessOption.topicsConfig.lstCacheTopic.push_back(realTopic);
    }
  }
}

void ParseRicsConfig::LoadDataCollectConfig() {
  m_node->get_parameter("DataCollectConfig.CacheFilePath", m_dataCollectOption.cacheFilePath);
  m_node->get_parameter("DataCollectConfig.DiskFreePercent", m_dataCollectOption.diskFreePercent);
  m_node->get_parameter("DataCollectConfig.UploadFmsServer.Host",
                        m_dataCollectOption.uploadFmsConfig.Host);
  m_node->get_parameter("DataCollectConfig.UploadFmsServer.Port",
                        m_dataCollectOption.uploadFmsConfig.Port);
  m_node->get_parameter("DataCollectConfig.RegisterFmsServer.Usrname",
                        m_dataCollectOption.regFmsConfig.Usrname);
  m_node->get_parameter("DataCollectConfig.RegisterFmsServer.Key",
                        m_dataCollectOption.regFmsConfig.Key);
}

std::string ParseRicsConfig::FullConfigPath(const std::string& relative_path) {
  std::string package_share_dir = ament_index_cpp::get_package_share_directory("rics_data_service");

  if (package_share_dir.empty()) {
    throw std::runtime_error("Package share directory not found");
  }

  if (package_share_dir.back() != '/') {
    package_share_dir += '/';
  }

  return package_share_dir + "config/" + relative_path;
}

bool ParseRicsConfig::GetEnableService() const { return m_enable_service; }

boost::any ParseRicsConfig::GetAny(rics::TypeId type) {
  if (typeid(RicsBusinessOption) == type) {
    return m_ricsBusinessOption;
  } else if (typeid(RicsTransportOption) == type) {
    return m_transportOption;
  } else if (typeid(DataCollectOption) == type) {
    return m_dataCollectOption;
  } else {
    auto fmt =
        boost::format("[ParseRicsConfig::GetAny] type %s unsupported") % type.pretty_name().c_str();
    throw std::runtime_error(fmt.str().c_str());
  }
}

}  // namespace rics