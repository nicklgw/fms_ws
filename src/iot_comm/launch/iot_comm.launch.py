
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch_ros.actions import Node
import os

def generate_launch_description():

    params_file = os.path.join(get_package_share_directory("iot_comm"), "config", "iot_comm_params.yaml")
    software_version = open("/etc/version", "r").read().strip()

    return LaunchDescription([
        Node(
            package='iot_comm',
            executable='iot_comm_node',
            name='iot_comm',
            output="screen",
            parameters=[params_file, {"software_version": software_version}],
            arguments=['--ros-args', '--log-level', 'ERROR']
        )
    ])
