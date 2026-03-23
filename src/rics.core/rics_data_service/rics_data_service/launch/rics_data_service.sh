#!/bin/bash

export ROS_DOMAIN_ID=5
source /opt/ros/humble/setup.bash
export ROS_LOCALHOST_ONLY=1
export RMW_IMPLEMENTATION=rmw_cyclonedds_cpp

mkdir -m a=rwx -p /var/log/bzlrobot/rics_data_service
export ROS_LOG_DIR="/var/log/bzlrobot/rics_data_service"

/usr/bin/python3 /opt/ros/humble/bin/ros2 launch rics_data_service rics_data_service.launch.py
