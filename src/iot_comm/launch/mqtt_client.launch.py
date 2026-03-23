
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch_ros.actions import Node
import os

def generate_launch_description():

    params_file = os.path.join(get_package_share_directory("iot_comm"), "config", "mqtt_client_params.yaml")
    rootCA_file = os.path.join(get_package_share_directory("iot_comm"), "config", "rootCA.pem")
    buffer_path = os.path.join(get_package_share_directory("iot_comm"), "buffer")

    return LaunchDescription([
        Node(
            package='mqtt_client',
            executable='mqtt_client',
            name='mqtt_client',
            output="screen",
            parameters=[params_file, {"broker.tls.ca_certificate": rootCA_file}, {"client.buffer.directory": buffer_path}],
            arguments=['--ros-args', '--log-level', 'ERROR']
        )
    ])
