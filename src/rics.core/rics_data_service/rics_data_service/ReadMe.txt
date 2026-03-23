

cd ~/fms_ws/
colcon build
source install/setup.bash 
ros2 launch rics_data_service rics_data_service.launch.py 

ros2 launch iot_comm iot_comm.launch.py # 启动程序
