#ifndef RICS_DEFINES_H
#define RICS_DEFINES_H

#include <boost/any.hpp>
#include <boost/optional.hpp>
#include <cstdint>
#include <list>
#include <string>
#include <vector>

namespace rics {

typedef struct {
  std::string host = "192.168.1.1";
  unsigned int port = 80;
  std::string userName = "admin";
  std::string password = "admin";
  int loopInterval = 3;
} RouterOption;

// ======== 基础结构体 ========
struct RicsClientOption {
  int nProtocol;  // 通信协议版本
  std::string strSN;
};

struct AppProtocolOption {
  std::string strProtocol;
};

struct OfflineDataInfo {
  std::string name;
  std::int64_t size;
  std::string md5;
  std::string fileid;
};

// ======== MQTT配置 ========
struct MqttOption {
  int m_nVersion;                         // 协议版本
  int m_nQos;                             // 服务质量等级
  int m_nPort;                            // 服务器端口
  int m_nMaxInflight;                     // 最大并发消息数
  int m_nKeepAlive;                       // 心跳间隔(秒)
  int m_nReConnectTime;                   // 重连间隔(秒)
  bool m_bInsecure;                       // 是否跳过SSL验证
  bool m_bCleanSession;                   // 是否启用干净会话
  std::string m_strHost;                  // 服务器地址
  std::string m_strCafile;                // CA证书路径
  std::string m_strUserName;              // 用户名
  std::string m_strPassword;              // 密码
  std::string m_strRobotSN;               // 设备序列号
  std::list<std::string> m_lstQos0Topic;  // QoS0主题列表
};

struct RicsTransportOption {
  int nProtocol;  // 通信协议
  std::string strSN;
  std::string strType;  // 传输类型
  std::vector<MqttOption> mqttOptions;
};

struct TcpOption {
  int nPort;
  std::string strHost;
};

struct AppTransportOption {
  std::string strProtocol;
  std::string strType;
  TcpOption tcpOption;
};

// ======== 系统配置 ========
struct RobotConfig {
  std::string strSN;            // 序列号
  std::string strRobotType;     // 机器人类型
  std::string strRobotVersion;  // 机器人版本
  std::string strMFGD;          // 生产日期
  bool bVerifyFormat;           // SN校验开关
  bool syncTime;                // 时间同步开关
  std::string nICCID;           // SIM卡ICCID
};

struct DataBaseConfig {
  std::string strFileName;  // 数据库名称
};

struct ParameterConfig {
  int reportPeriod;        // 上报间隔(毫秒)
  int retryTime;           // 最大重试次数
  int retryPeriod;         // 重试间隔(毫秒)
  int radarUploadSwitch;   // 雷达数据开关
  int signatureSupport;    // 签名支持标志
  std::string extendData;  // 扩展参数
};

struct TopicsConfig {
  std::list<std::string> lstCacheTopic;    // 缓存主题
  std::list<std::string> lstExcludeTopic;  // 排除主题
  std::list<std::string> lstQos0Topic;     // QoS0主题
};

struct FMSAddress {
  std::string host;
  int port;
};

struct MapConfig {
  std::string strSavePath;  // 地图存储路径
};

struct RadarConfig {
  int reportCount{30};  // 雷达上报点数
};

struct RicsBusinessOption {
  int nProtocol;                           // 协议版本
  std::string strTransportType;            // 传输层类型
  std::string strDefaultPath;              // 默认存储路径
  std::string strScriptsPath;              // 脚本目录
  DataBaseConfig dbConfig;                 // 数据库配置
  RobotConfig robotConfig;                 // 机器人配置
  MapConfig mapConfig;                     // 地图配置
  RadarConfig radarConfig;                 // 雷达配置
  ParameterConfig defaultParam;            // 运行参数
  TopicsConfig topicsConfig;               // 主题配置
  FMSAddress fmsAddress;                   // 当前FMS地址
  std::vector<FMSAddress> fmsAddressList;  // 备用FMS地址
  std::string strPrivateKey;               // SSL私钥
  std::string strFmsEnvUrl;                // FMS环境地址
};

// ======== 版本与授权 ========
struct SoftwareVersion {
  std::string module;
  std::string version;
  std::string md5;
};

struct AuthorizationInfo {
  std::int64_t remainSecond;
  std::int64_t expireSecond;
  std::string lockState;
  std::string lockReason;
};

struct HardwareVersion {
  std::string module;
  std::string version;
  std::string firmware;
};

struct RobotInfo {
  boost::optional<std::string> serialNumber;
  boost::optional<std::string> robotType;
  boost::optional<std::string> robotVersion;
  boost::optional<std::string> manufactureDate;
  boost::optional<std::string> ICCID;
  boost::optional<std::vector<SoftwareVersion>> softwareVersions;
  boost::optional<FMSAddress> fmsAddress;
  boost::optional<std::vector<FMSAddress>> fmsAddressReserved;
  boost::optional<AuthorizationInfo> authorizationInfo;
  boost::optional<std::vector<HardwareVersion>> hardwareVersions;
};

struct SubscriberConfig {
  std::string topic;
  std::string message_type;  // "MqttSimple" or "GenericData"
  std::string handler_type;  // 类型名，如 "DeviceInfoHandler"
};

}  // namespace rics

// ======== 通信主题 ========
#include "rics_data_service_msgs/msg/generic_data.hpp"
#include "rics_data_service_msgs/msg/mqtt_simple.hpp"
using MqttSimple = rics_data_service_msgs::msg::MqttSimple;
using GenericData = rics_data_service_msgs::msg::GenericData;
// using DataPair = rics_data_service_msgs::msg::DataPair;

#endif  // RICS_DEFINES_H