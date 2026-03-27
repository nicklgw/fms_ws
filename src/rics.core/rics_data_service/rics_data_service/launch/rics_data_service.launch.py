import os
import yaml
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch_ros.actions import Node
from launch_ros.actions import ComposableNodeContainer
from launch_ros.descriptions import ComposableNode
def generate_launch_description():

    iot_config_file = os.path.join(get_package_share_directory("iot_comm"), "config", "iot_comm_params.yaml")

    did = "SerialNumber"

    # 提取SerialNumber的值
    try:

        # 读取YAML文件
        with open(iot_config_file, 'r') as file:  # 替换为实际文件名
            data = yaml.safe_load(file)

        did = data['iot_comm']['ros__parameters']['did']
        print(f"did: {did}")

    except FileNotFoundError:
        print("cannot find iot_config_file.")
    except yaml.YAMLError as e:
        print(f"YAMLError: {e}")
    except KeyError as e:
        print(f"cannot find did - {e}")
    except Exception as e:
        print(f"Exception: {e}")

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
        parameters=[config_file, {"RobotConfig.SerialNumber": did}],
        arguments=ros_args,  # 显式传递日志级别参数
    )

    return LaunchDescription([rics_data_service_node])
