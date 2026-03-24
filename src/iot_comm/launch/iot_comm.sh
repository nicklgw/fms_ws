#!/bin/bash

export ROS_DOMAIN_ID=5
source /opt/ros/humble/setup.bash
export RMW_IMPLEMENTATION=rmw_cyclonedds_cpp
export CYCLONEDDS_URI=file:///etc/cyclonedds.xml

mkdir -m a=rwx -p /var/log/bzlrobot/iot_comm
export ROS_LOG_DIR="/var/log/bzlrobot/iot_comm"

/usr/bin/python3 /opt/ros/humble/bin/ros2 launch iot_comm iot_comm.launch.py
