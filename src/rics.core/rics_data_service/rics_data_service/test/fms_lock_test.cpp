#include "rics_data_service/fms_adapter/MosquitoWrapper.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <thread>
#include <filesystem>
#include <memory>

using json = nlohmann::json;
using namespace rics;

class FmsLockTest {
private:
    // 成员变量
    MqttOption m_mqttOption;
    std::string m_sn;
    std::unique_ptr<MosquitoWrapper> m_mqtt;
    bool m_bSimulateLock = false; // 是否启用模拟锁机
    
    // 辅助函数
    std::string generateMsgId() {
        return "msg_" + std::to_string(getTimestamp());
    }
    
    int64_t getTimestamp() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
                   std::chrono::system_clock::now().time_since_epoch())
            .count();
    }
    
public:
    FmsLockTest(bool simulateLock = false) : m_bSimulateLock(simulateLock) {
        // 生成唯一的设备序列号
        m_sn = "TEST-" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
        
        // 配置 MQTT 选项 - 参考 MQTTX 的成功配置
        m_mqttOption.m_nVersion = 4; // MQTT 3.1.1
        m_mqttOption.m_nQos = 1;
        m_mqttOption.m_strHost = "fms.bzlrobot.com"; // FMS 服务器地址
        m_mqttOption.m_nPort = 1883; // 端口 1883，但使用 TLS
        
        m_mqttOption.m_nKeepAlive = 60;
        m_mqttOption.m_nMaxInflight = 10;
        m_mqttOption.m_nReConnectTime = 5;
        m_mqttOption.m_bInsecure = true; // 允许不安全的 TLS 连接
        m_mqttOption.m_bCleanSession = true;
        
        // 设置证书路径
        std::string config_path = "/home/jem/rics_ws/src/rics.core/rics_data_service/rics_data_service/config/";
        std::string cafile_path = config_path + "ca.pem";
        
        if (std::filesystem::exists(cafile_path)) {
            m_mqttOption.m_strCafile = cafile_path;
            std::cout << "Using CA certificate: " << cafile_path << std::endl;
        } else {
            m_mqttOption.m_strCafile = "";
            std::cout << "Warning: CA certificate not found, using insecure TLS" << std::endl;
        }
        
        // 使用有效的设备序列号格式
        m_sn = "rics-ccu-8986-5869"; // 使用 MQTTX 中成功的客户端 ID
        m_mqttOption.m_strRobotSN = m_sn;
        m_mqttOption.m_strUserName = "";
        m_mqttOption.m_strPassword = "";
        
        std::cout << "Testing FMS server with TLS: " << m_mqttOption.m_strHost << ":" << m_mqttOption.m_nPort << std::endl;
        std::cout << "Simulate lock mode: " << (m_bSimulateLock ? "ENABLED" : "DISABLED") << std::endl;
        
        // 创建 MQTT 客户端
        m_mqtt = std::make_unique<MosquitoWrapper>(m_mqttOption);
    }
    
    void run() {
        std::cout << "Starting FMS lock test..." << std::endl;
        std::cout << "Device SN: " << m_sn << std::endl;
        std::cout << "Server: " << m_mqttOption.m_strHost << ":" << m_mqttOption.m_nPort << std::endl;
        
        if (m_bSimulateLock) {
            std::cout << "Auto-simulate mode: Will auto-send lock/unlock commands." << std::endl;
            std::cout << "Test will run for 60 seconds." << std::endl;
        } else {
            std::cout << "Manual mode: Waiting for FMS server to send lock commands." << std::endl;
            std::cout << "To test lock functionality, send a lock command to topic: command/" << m_sn << "/lock" << std::endl;
            std::cout << "Example lock command payload: {\"Action\": 1, \"DeviceCode\": \"rics-ccu-8986-5869\", \"MsgID\": \"test_msg_123\"}" << std::endl;
            std::cout << "Test will run for 60 seconds." << std::endl;
        }
        
        // 设置回调
        MosquitoWrapper::CallbackList callbacks;
        callbacks.connectedCallback = [this](bool connected) {
            if (connected) {
                std::cout << "MQTT connected successfully!" << std::endl;
                onMqttConnected();
            } else {
                std::cout << "MQTT disconnected" << std::endl;
            }
        };
        
        callbacks.receivedCallback = [this](const std::shared_ptr<MqttMessage>& msg) {
            std::cout << "Received message: " << std::endl;
            std::cout << "  Topic: " << msg->m_strTopic << std::endl;
            std::cout << "  Payload: " << msg->m_strPayload << std::endl;
            onMqttMessageReceived(msg);
        };
        
        callbacks.sendCallback = [](int mid) {
            std::cout << "Message published, mid: " << mid << std::endl;
        };
        
        callbacks.subscribedCallback = [](int mid, const std::vector<int>& qos) {
            std::cout << "Subscribed, mid: " << mid << ", QoS: ";
            for (auto q : qos) {
                std::cout << q << " ";
            }
            std::cout << std::endl;
        };
        
        // 初始化 MQTT 客户端
        m_mqtt->Init(callbacks);
        
        // 等待测试完成
        std::cout << "Waiting for FMS responses and lock commands..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(60));
        std::cout << "Test completed." << std::endl;
    }
    
    // 设置模拟锁机开关
    void setSimulateLock(bool simulate) {
        m_bSimulateLock = simulate;
        std::cout << "Simulate lock mode changed to: " << (m_bSimulateLock ? "ENABLED" : "DISABLED") << std::endl;
    }
    
private:
    void onMqttConnected() {
        // 订阅 FMS 指令主题
        std::cout << "Subscribing to FMS topics..." << std::endl;
        m_mqtt->Subscribe("command/" + m_sn + "/login-ack", 1);
        m_mqtt->Subscribe("command/" + m_sn + "/lock", 1);
        m_mqtt->Subscribe("command/" + m_sn + "/notify-ack", 1);
        
        // 上报 login 消息
        std::cout << "Reporting login message..." << std::endl;
        reportLogin();
        
        // 只有在模拟模式下才自动测试锁机和解锁功能
        if (m_bSimulateLock) {
            std::thread([this]() {
                // 等待 2 秒，确保登录完成
                std::this_thread::sleep_for(std::chrono::seconds(2));
                
                // 测试锁机功能
                std::cout << "\n=== 自动测试锁机功能（模拟模式）===" << std::endl;
                sendLockCommand(1); // 1: 锁机
                
                // 等待 5 秒
                std::this_thread::sleep_for(std::chrono::seconds(5));
                
                // 测试解锁功能
                std::cout << "\n=== 自动测试解锁功能（模拟模式）===" << std::endl;
                sendLockCommand(2); // 2: 解锁
            }).detach();
        } else {
            std::cout << "\n=== 等待FMS服务器发送锁机命令 ===" << std::endl;
            std::cout << "请使用MQTTX或其他MQTT客户端向以下主题发送命令：" << std::endl;
            std::cout << "主题: command/" << m_sn << "/lock" << std::endl;
            std::cout << "锁机命令: {\"Action\": 1, \"DeviceCode\": \"" << m_sn << "\", \"MsgID\": \"manual_lock_001\"}" << std::endl;
            std::cout << "解锁命令: {\"Action\": 2, \"DeviceCode\": \"" << m_sn << "\", \"MsgID\": \"manual_unlock_001\"}" << std::endl;
        }
    }
    
    void sendLockCommand(int action) {
        // 发送锁机/解锁指令
        json payload = {
            {"Action", action},
            {"DeviceCode", m_sn},
            {"MsgID", generateMsgId()},
            {"Timestamp", getTimestamp()}
        };
        
        // 锁机时添加 LockType 字段
        if (action == 1) {
            payload["LockType"] = 1; // 1: 立即锁
        }
        
        auto msg = std::make_shared<MqttMessage>();
        msg->m_strTopic = "command/" + m_sn + "/lock";
        msg->m_strPayload = payload.dump();
        msg->m_nQos = 1;
        
        std::cout << "Sending " << (action == 1 ? "lock" : "unlock") << " command: " << payload.dump() << std::endl;
        m_mqtt->Send(msg);
    }
    
    void reportLogin() {
        json body = {
            {"data", {
                {"architecture", "aarch64"},
                {"encodingRule", "json"},
                {"iccidOfRouter", "89860622330049671431"},
                {"lockState", 2}, // 未锁定
                {"protocol", "1.3"},
                {"softwareVersion", "V0.3.15"},
                {"sysVersion", "Ubuntu16.04"}
            }}
        };
        json header = {
            {"msgId", generateMsgId()},
            {"signature", ""},
            {"timestamp", getTimestamp()}
        };
        json payload = {
            {"header", header},
            {"body", body}
        };
        
        auto msg = std::make_shared<MqttMessage>();
        msg->m_strTopic = "device/" + m_sn + "/login";
        msg->m_strPayload = payload.dump();
        msg->m_nQos = 1;
        m_mqtt->Send(msg);
        
        std::cout << "Login message sent." << std::endl;
    }
    
    void onMqttMessageReceived(const std::shared_ptr<MqttMessage>& msg) {
        const std::string& topic = msg->m_strTopic;
        const std::string& payload = msg->m_strPayload;
        
        if (topic == "command/" + m_sn + "/login-ack") {
            std::cout << "Received login-ack: " << payload << std::endl;
        } else if (topic == "command/" + m_sn + "/lock") {
            std::cout << "Received lock command from FMS server: " << payload << std::endl;
            handleLockCommand(payload);
        } else if (topic == "command/" + m_sn + "/notify-ack") {
            std::cout << "Received notify-ack: " << payload << std::endl;
        }
    }
    
    void handleLockCommand(const std::string& payload) {
        try {
            json j = json::parse(payload);
            int action = j["Action"]; // 1:锁机, 2:解锁
            std::string deviceCode = j["DeviceCode"];
            std::string msgId = j["MsgID"];
            
            // 处理 LockType 字段（锁机时存在）
            int lockType = 1; // 默认立即锁
            if (action == 1 && j.contains("LockType")) {
                lockType = j["LockType"];
                std::cout << "LockType: " << lockType << std::endl;
            }
            
            std::cout << "Processing lock command: Action=" << action 
                     << ", DeviceCode=" << deviceCode 
                     << ", MsgID=" << msgId << std::endl;
            
            // 1. 响应 lock-ack
            sendLockAck(deviceCode, msgId, 1, "");
            
            // 2. 在实际应用中，这里应该执行真正的锁机/解锁逻辑
            std::cout << (action == 1 ? "执行锁机操作..." : "执行解锁操作...") << std::endl;
            
            // 模拟执行时间
            std::this_thread::sleep_for(std::chrono::seconds(1));
            
            // 3. 上报 notify 结果
            bool success = true; // 假设操作成功
            std::string event = success ? (action == 1 ? "LOCK_SUCCESS" : "UNLOCK_SUCCESS")
                                       : (action == 1 ? "LOCK_FAILED" : "UNLOCK_FAILED");
            std::string msgStr = success ? (action == 1 ? "锁机成功" : "解锁成功")
                                         : (action == 1 ? "锁机失败" : "解锁失败");
            sendNotify(deviceCode, event, msgStr);
            
        } catch (const json::exception& e) {
            std::cerr << "Parse lock command failed: " << e.what() << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Handle lock command failed: " << e.what() << std::endl;
        }
    }
    
    void sendLockAck(const std::string& deviceCode, const std::string& msgId, int result, const std::string& returnMsg) {
        json payload = {
            {"DeviceCode", deviceCode},
            {"Result", result},
            {"ReturnMsg", returnMsg},
            {"MsgID", msgId},
            {"Timestamp", getTimestamp()}
        };
        
        auto msg = std::make_shared<MqttMessage>();
        msg->m_strTopic = "device/" + m_sn + "/lock-ack";
        msg->m_strPayload = payload.dump();
        msg->m_nQos = 1;
        m_mqtt->Send(msg);
        
        std::cout << "Lock-ack sent." << std::endl;
    }
    
    void sendNotify(const std::string& deviceCode, const std::string& event, const std::string& msgStr) {
        json payload = {
            {"DeviceCode", deviceCode},
            {"Event", event},
            {"Msg", msgStr},
            {"MsgID", generateMsgId()},
            {"Timestamp", getTimestamp()}
        };
        
        auto msg = std::make_shared<MqttMessage>();
        msg->m_strTopic = "device/" + m_sn + "/notify";
        msg->m_strPayload = payload.dump();
        msg->m_nQos = 1;
        m_mqtt->Send(msg);
        
        std::cout << "Notify sent: " << event << std::endl;
    }
};

int main(int argc, char* argv[]) {
    try {
        // 解析命令行参数
        bool simulateLock = false;
        for (int i = 1; i < argc; i++) {
            std::string arg = argv[i];
            if (arg == "--simulate" || arg == "-s") {
                simulateLock = true;
            } else if (arg == "--help" || arg == "-h") {
                std::cout << "Usage: " << argv[0] << " [options]" << std::endl;
                std::cout << "Options:" << std::endl;
                std::cout << "  -s, --simulate    Enable auto-simulate lock/unlock mode" << std::endl;
                std::cout << "  -h, --help        Show this help message" << std::endl;
                return 0;
            }
        }
        
        FmsLockTest test(simulateLock);
        test.run();
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}