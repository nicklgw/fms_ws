
#include <vector>
#include <sstream>
#include <functional>

#include "iot_comm/persist_parameter_client.hpp"

PersistParametersClient::PersistParametersClient(rclcpp::Node *node, const std::string & remote_node_name)
: node_(node)
{
  sync_param_client_ = std::make_unique<rclcpp::SyncParametersClient>(node_, remote_node_name);
  async_param_client_ = std::make_unique<rclcpp::AsyncParametersClient>(node_, remote_node_name);
}

bool PersistParametersClient::wait_param_server_ready()
{
  bool ret = false;

  if(rclcpp::ok()) 
  {
    RCLCPP_INFO(node_->get_logger(), "Waiting 3 seconds to wait for the parameter server to be ready...");
    ret = sync_param_client_->wait_for_service(3s);
  }

  return ret;
}

bool PersistParametersClient::read_parameter(const std::string & param_name, std::vector<rclcpp::Parameter> & parameter)
{
  bool ret = true;

  parameter = sync_param_client_->get_parameters({param_name});
  for(auto & param : parameter)
  {
      switch (param.get_type())
      {
        case rclcpp::ParameterType::PARAMETER_NOT_SET:
        {
          RCLCPP_INFO(node_->get_logger(), "READ OPERATION : parameter %s was not set(or deleted), it will not be stored", param_name.c_str());
          break;
        }
        case rclcpp::ParameterType::PARAMETER_BOOL:
        {
          bool value = param.as_bool();
          RCLCPP_INFO(node_->get_logger(), "GET OPERATION : parameter %s's value is %s", param_name.c_str(), value?"true":"false");
          break;
        }
        case rclcpp::ParameterType::PARAMETER_INTEGER:
        {
          int64_t value = param.as_int();
          RCLCPP_INFO(node_->get_logger(), "GET OPERATION : parameter %s's value is %ld", param_name.c_str(), value);
          break;
        }
        case rclcpp::ParameterType::PARAMETER_DOUBLE:
        {
          double value = param.as_double();
          RCLCPP_INFO(node_->get_logger(), "GET OPERATION : parameter %s's value is %lf", param_name.c_str(), value);
          break;
        }
        case rclcpp::ParameterType::PARAMETER_STRING:
        {
          std::string value = param.as_string();
          RCLCPP_INFO(node_->get_logger(), "GET OPERATION : parameter %s's value is %s", param_name.c_str(), value.c_str());
          break;
        }
        case rclcpp::ParameterType::PARAMETER_BYTE_ARRAY:
        {
          std::ostringstream ss;
          auto array = param.as_byte_array();
          format_array_output<uint8_t>(ss, array);
          RCLCPP_INFO(node_->get_logger(), "GET OPERATION : parameter %s's value is %s", param_name.c_str(), ss.str().c_str());
          break;
        }
        case rclcpp::ParameterType::PARAMETER_BOOL_ARRAY:
        {
          std::ostringstream ss;
          auto array = param.as_bool_array();
          format_array_output<bool>(ss, array);
          RCLCPP_INFO(node_->get_logger(), "GET OPERATION : parameter %s's value is %s", param_name.c_str(), ss.str().c_str());
          break;
        }
        case rclcpp::ParameterType::PARAMETER_INTEGER_ARRAY:
        {
          std::ostringstream ss;
          auto array = param.as_integer_array();
          format_array_output<int64_t>(ss, array);
          RCLCPP_INFO(node_->get_logger(), "GET OPERATION : parameter %s's value is %s", param_name.c_str(), ss.str().c_str());   
          break;
        }
        case rclcpp::ParameterType::PARAMETER_DOUBLE_ARRAY:
        {
          std::ostringstream ss;
          auto array = param.as_double_array();
          format_array_output<double>(ss, array);
          RCLCPP_INFO(node_->get_logger(), "GET OPERATION : parameter %s's value is %s", param_name.c_str(), ss.str().c_str());
          break;
        }
        case rclcpp::ParameterType::PARAMETER_STRING_ARRAY:
        {
          std::ostringstream ss;
          auto array = param.as_string_array();
          format_array_output<std::string>(ss, array);
          RCLCPP_INFO(node_->get_logger(), "GET OPERATION : parameter %s's value is %s", param_name.c_str(), ss.str().c_str());
          break;
        }
        default: {
          ret = false;
          RCLCPP_INFO(node_->get_logger(), "parameter %s unsupported type %d", param_name.c_str(), param.get_type());
          break;
      }
    }
  }

  return ret;
}
