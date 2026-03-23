import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch_ros.actions import Node
from launch_ros.actions import ComposableNodeContainer
from launch_ros.descriptions import ComposableNode
def generate_launch_description():

    # rics_data_service_container = ComposableNodeContainer(
    #         name='rics_data_service_container',
    #         namespace='',
    #         package='rclcpp_components',
    #         executable='component_container',
    #         output='screen', 
    #         composable_node_descriptions=[
    #             ComposableNode(
    #                 namespace='rics',
    #                 package='rics_data_service',
    #                 plugin='rics::data::RicsDataService',
    #                 name='rics_data_service',
    #                 parameters=[os.path.join(
    #                     get_package_share_directory("rics_data_service"), 
    #                     "config", "rics_data_service_launch.yaml")]
    #                 ),
    #         ]#,
    # )

    # return LaunchDescription([rics_data_service_container])

    # 获取配置文件路径（增加文件存在性检查）
    config_file = os.path.join(
        get_package_share_directory('rics_data_service'),
        'config',
        'rics_data_service_launch.yaml'
    )
    if not os.path.exists(config_file):
        raise FileNotFoundError(f"Config file not found: {config_file}")

    # 强制设置 ROS 日志级别（Debug 模式）
    ros_args = ['--ros-args', '--log-level', 'rics_data_service:=debug']

    # 独立节点配置
    rics_data_service_node = Node(
        package='rics_data_service',
        executable='rics_data_service_node',
        # name='rics_data_service',
        namespace='rics',
        output='screen',
        parameters=[config_file],
        arguments=ros_args,  # 显式传递日志级别参数
    )

    return LaunchDescription([rics_data_service_node])