#!/usr/bin/env python3

import rclpy
from rclpy.node import Node
from rics_data_service_msgs.msg import MqttSimple
import json
import time

def main():
    rclpy.init()
    node = Node('send_login_to_fms')
    publisher = node.create_publisher(MqttSimple, '/rics/rics_data_to_fms', 10)
    
    # 构造登录消息
    timestamp = int(time.time() * 1000)
    msg_id = f"msg_{int(time.time())}"
    
    login_message = {
        "header": {
            "msgId": msg_id,
            "signature": "",
            "timestamp": timestamp
        },
        "body": {
            "data": {
                "architecture": "aarch64",
                "encodingRule": "json",
                "iccidOfRouter": "89860622330049671431",
                "lockState": 2,  # 未锁定
                "protocol": "1.3",
                "softwareVersion": "V0.3.15",
                "sysVersion": "Ubuntu16.04"
            }
        }
    }
    
    # 创建MqttSimple消息
    mqtt_msg = MqttSimple()
    mqtt_msg.topic = "device/rics-ccu-8986-5869/login"
    mqtt_msg.message = json.dumps(login_message)
    
    # 发布消息
    publisher.publish(mqtt_msg)
    print(f"Published login message to topic: {mqtt_msg.topic}")
    print(f"Message: {mqtt_msg.message}")
    
    # 等待消息发送
    rclpy.spin_once(node, timeout_sec=1.0)
    
    # 清理
    publisher.destroy()
    node.destroy_node()
    rclpy.shutdown()

if __name__ == '__main__':
    main()
