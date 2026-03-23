
bzlrobot@raspberrypi:~/zhengping_ws/iot_comm_ws$ colcon build --packages-select iot_comm # 编译
bzlrobot@raspberrypi:~/zhengping_ws/iot_comm_ws$ source install/setup.bash # 设置环境变量
bzlrobot@raspberrypi:~/zhengping_ws/iot_comm_ws$ ros2 launch iot_comm iot_comm.launch.py # 启动程序

colcon build --packages-select iot_comm --allow-overriding iot_comm # 编译

# 启动mqtt-client代理
ros2 launch iot_comm mqtt_client.launch.py

# 启动mqtt-client代理
bzlrobot@raspberrypi:~/zhengping_ws/iot_comm_ws$ ros2 launch iot_comm mqtt_client.launch.ros2.primitive.xml
# 订阅MQTT话题，验证测试数据
nick@nick-dell:~/zhengping_ws/iot_comm_ws/src/iot_comm/config$ mosquitto_sub -v --insecure -V mqttv311 -h fms.bzlrobot.com -p 2884 -t "device/ZHENGPING-001/#" --cafile ./rootCA.ros2.pem

ros2 mqtt-client 安装并测试
https://blog.csdn.net/hai411741962/article/details/134593264
sudo apt update
sudo apt install ros-humble-mqtt-client

bzlrobot@raspberrypi:/opt/ros/humble/share/std_msgs/msg$ ros2 topic pub --once /iot/test std_msgs/msg/String '{"data":"hello"}'

订阅离合器结合状态(已结合true, 已分离false)
bzlrobot@raspberrypi:~$ ros2 topic echo /vehicle/engage


发布
话题: connect/ZHENGPING-001/lock
消息: 
{
    "Action": 1, 
    "DeviceCode": "ZHENGPING-001", 
    "LockType": 1, 
    "MsgID": "9e0ee920-6342-4f3a-a3a1-82c544852281", 
    "Timestamp": "1724995949203"
}

当锁机{engage:false}时， 由vehicle_cmd_gate上报错误码到遥控器[54障碍物检测报警] 

将两个配置文件中的设备ID批量修改
sed -i 's/old_content/new_content/g' filename
sed -i 's/ZHENGPING-001/ZHENGPING-003/g' mqtt_client.ros2.primitive.yaml iot_comm_params.yaml
sed -i 's/DZY46510002/ZHENGPING-001/g' mqtt_client_params.yaml iot_comm_params.yaml

修改配置文件
cd /opt/ros/humble/share/iot_comm/config
sed -i 's/ZHENGPING-001/DZY46510002/g' iot_comm_params.yaml  mqtt_client_params.yaml

sed -i 's/DZY46510002/ZHENGPING-001/g' iot_comm_params.yaml  mqtt_client_params.yaml


生产环境
host: fms.bzlrobot.com
port: 1883

测试环境
host: fms.bzlrobot.com
port: 2884

  Service Servers:
    /mqtt_client/is_connected: mqtt_client_interfaces/srv/IsConnected



安装自启脚本
sudo install -Dm644 mqtt_client.service  /usr/lib/systemd/system/mqtt_client.service

服务管理的常用操作

#重新使能服务
systemctl enable mqtt_client.service
# 启动服务：
systemctl start mqtt_client.service
# 关闭服务：
systemctl stop mqtt_client.service
# 重启服务：
systemctl restart mqtt_client.service
# 显示服务状态：
systemctl status mqtt_client.service
# 禁用服务开机启动：
systemctl disable mqtt_client.service

systemctl daemon-reload
systemctl start mqtt_client.service

查看system.service自启服务日志
journalctl -n 100 -u mqtt_client.service


安装自启脚本
sudo install -Dm644 iot_comm.service  /usr/lib/systemd/system/iot_comm.service

服务管理的常用操作

#重新使能服务
systemctl enable iot_comm.service
# 启动服务：
systemctl start iot_comm.service
# 关闭服务：
systemctl stop iot_comm.service
# 重启服务：
systemctl restart iot_comm.service
# 显示服务状态：
systemctl status iot_comm.service
# 禁用服务开机启动：
systemctl disable iot_comm.service

systemctl daemon-reload
systemctl start iot_comm.service

查看system.service自启服务日志
journalctl -n 100 -u iot_comm.service

$ bloom-generate rosdebian
$ fakeroot debian/rules binary


ros2 service call /mqtt_client/is_connected mqtt_client_interfaces/srv/IsConnected '{}'
/opt/ros/humble/share/iot_comm/launch/buffer
mkdir -m a=rwx -p /var/log/bzlrobot/mqtt_client /opt/ros/humble/share/iot_comm/launch/buffer
给buffer目录权限，不然mqtt_client启动会失败
sudo chmod 0777 -R /opt/ros/humble/share/iot_comm/buffer

持久化配置文件需要给可写权限
sudo chmod 0777 /opt/ros/humble/share/parameter_server/param/parameters_via_launch.yaml
