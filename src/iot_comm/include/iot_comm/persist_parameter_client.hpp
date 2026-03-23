
#ifndef __PERSIST_PARAMETER_CLIENT_H__
#define __PERSIST_PARAMETER_CLIENT_H__

#include "rclcpp/rclcpp.hpp"
#include <stdexcept>

using namespace std::chrono_literals;

class PersistParametersClient
{
public:
  PersistParametersClient(rclcpp::Node *node, const std::string & remote_node_name = "parameter_server");

  // Format the array value for easy output.
  template <typename ValueType>
  inline void format_array_output(std::ostringstream & ss, const std::vector<ValueType> & value_vec)
  {
    ss << "[ ";
    for(const auto & value : value_vec) {
      ss << value << " ";
    }
    ss << "]";

    return;
  }

  // Make sure the client and the server are connected through the Service.
  bool wait_param_server_ready();

  /*
  * Read the parameter value specified by `param_name`.
  * @param param_name The name of parameter.
  * @param parameter The vector that holds the read result.
  * @return Operation as expected or not.
  */
  bool read_parameter(const std::string & param_name, std::vector<rclcpp::Parameter> & parameter);

  /*
  * Change the value of `param_name` to `param_value`.
  * The principle is to update if param exists, otherwise insert.
  *
  * @param param_name The name of parameter.
  * @param parameter_value The parameter value that you want to set.
  * @return Operation as expected or not.
  */
  template <typename ValueType>
  bool modify_parameter(const std::string & param_name, const ValueType & param_value)
  {
    bool ret = true;
    std::vector<rclcpp::Parameter> parameters;

    parameters.push_back(rclcpp::Parameter(param_name, rclcpp::ParameterValue(param_value)));
    auto set_param_result = sync_param_client_->set_parameters(parameters);
    for (auto & result : set_param_result)
    {
      if (!result.successful)
      {        
        RCLCPP_INFO(node_->get_logger(), "SET OPERATION : Failed to set parameter: %s", result.reason.c_str());
        return false;
      }
    }
    RCLCPP_INFO(node_->get_logger(), "SET OPERATION : Set parameter %s successfully.", param_name.c_str());

    return ret;
  }

  template <typename ValueType>
  void async_set_parameter(const std::string & param_name, const ValueType & param_value)
  {
    if (async_param_client_->service_is_ready())
    {
      RCLCPP_INFO_STREAM(node_->get_logger(), "set " << param_name << " to " << param_value);    

      std::vector<rclcpp::Parameter> parameters;
      parameters.push_back(rclcpp::Parameter(param_name, rclcpp::ParameterValue(param_value)));

      async_param_client_->set_parameters(parameters,
        [this](std::shared_future<std::vector<rcl_interfaces::msg::SetParametersResult>> future)
        {
          future.wait();

          auto results = future.get();

          if (results.size() != 1) 
          {
            RCLCPP_ERROR_STREAM(node_->get_logger(), "expected 1 result, got " << results.size());
          }
          else 
          {
            if (results[0].successful) 
            {
              RCLCPP_INFO(node_->get_logger(), "success");
            } 
            else 
            {
              RCLCPP_ERROR(node_->get_logger(), "failure");
            }
          }
        });
    } 
    else 
    {
      RCLCPP_ERROR(node_->get_logger(), "remote parameter server is not ready");
    }
  }

private:
  rclcpp::Node *node_;
  std::unique_ptr<rclcpp::SyncParametersClient> sync_param_client_;
  std::unique_ptr<rclcpp::AsyncParametersClient> async_param_client_;
};

/**
 * NoServerError
 *
 * The client will wait 5 seconds for the server to be ready.
 * If timeout, then throw an exception to terminate the endless waiting.
 */
struct NoServerError : public std::runtime_error
{
  public:
    NoServerError()
      : std::runtime_error("cannot connect to server"){}
};

/* 
 * SetOperationError
 *
 * When executing `set_parameter`, if the set operation failed, 
 * throw an exception to ignore the subsequent test.
 */
struct SetOperationError : public std::runtime_error
{
  public:
    SetOperationError()
      : std::runtime_error("set operation failed"){}
};

#endif // __PERSIST_PARAMETER_CLIENT_H__
