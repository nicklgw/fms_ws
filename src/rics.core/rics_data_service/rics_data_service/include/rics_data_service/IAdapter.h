#ifndef RICS_IADAPTER_H
#define RICS_IADAPTER_H

#include <rics_data_service/FunctionTraits.h>

#include <boost/any.hpp>
#include <boost/optional.hpp>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
namespace rics {

using EventHandler = std::function<boost::any(const boost::any&)>;
using ApplyHandler = std::function<void(const boost::any&)>;
using TypeId = boost::typeindex::stl_type_index;

class IAdapter {
 public:
  explicit IAdapter() = default;

  virtual ~IAdapter() = default;

  /**
   * 订阅事件
   * @tparam Callable 回调类型，支持lambda与function,
   * 回调函数必须提供返回值，无返回值返回boost::any
   * @param callback 回调函数
   * @return
   */
  template <typename Callable>
  IAdapter& OnEvent(Callable callback) {
    using paramType = typename function_traits<Callable>::template args<0>::type;
    // using returnType = typename function_traits<Callable>::return_type;
    return OnEventAny(typeid(paramType), [=](const boost::any& any) -> boost::any {
      auto concreteParam = boost::any_cast<paramType>(any);
      return callback(concreteParam);
    });
  }

  /**
   * 设置
   * @tparam T 命令
   * @param t
   */
  template <typename T>
  void Set(const T& t) {
    SendAny(t);
  }

  /**
   * 发送消息
   * @tparam T 命令
   * @param t
   */
  template <typename T>
  void Send(const T& t) {
    SendAny(t);
  }

  /**
   * 获取消息
   * @tparam T 消息类型
   */
  template <typename T>
  T Get() {
    auto any = GetAny(typeid(T));
    return boost::any_cast<T>(any);
  }

  /**
   * 同步请求
   * @tparam T
   * @tparam R
   * @param t
   * @return
   */
  template <typename T, typename R>
  bool ApplySync(const T& t, R& r, std::int64_t timeoutMS = 3000) {
    auto anyResult = ApplySyncAny(t, timeoutMS);
    if (!anyResult.empty()) {
      r = boost::any_cast<R>(anyResult);
      return true;
    }
    return false;
  }

  /**
   * 异步请求
   * @tparam T 命令类型
   * @tparam Callable 回调函数类型，将从回调函数中萃取参数类型
   * @param t 命令
   * @param callback 回调函数
   */
  template <typename T, typename Callable>
  void ApplyAsync(const T& t, Callable callback) {
    using paramType = typename function_traits<Callable>::template args<0>::type;
    // using returnType = typename function_traits<Callable>::return_type;
    ApplyAsyncAny(t, [=](const boost::any& any) -> boost::any {
      auto concreteParam = boost::any_cast<paramType>(any);
      callback(concreteParam);
      return boost::any();
    });
  }

 protected:
  /**
   * 注册事件回调
   * @param eventType 事件类型
   * @param handler 事件回调
   * @return
   */
  IAdapter& OnEventAny(const std::type_info& eventType, const EventHandler& handler) {
    std::lock_guard<std::mutex> lock(m_eventMutex);
    m_eventHandlers[eventType] = handler;
    return *this;
  }

  /**
   * 获取消息
   * @param type
   * @return
   */
  virtual boost::any GetAny(TypeId type) {
    (void)type;
    return {};
  }

  /**
   * 发送消息
   * @param any
   */
  virtual void SendAny(const boost::any& any) { (void)any; }

  /**
   * 发送同步请求
   * @param command 请求命令
   * @param timeoutMS 超时时间
   * @return
   */
  virtual boost::any ApplySyncAny(const boost::any& command, std::int64_t timeoutMS) {
    (void)command;
    (void)timeoutMS;
    return {};
  }

  /**
   * 发送异步请求
   * @param command 请求命令
   * @param handler 返回的处理函数
   * @return 当handler=nullptr时， 返回结果
   */
  virtual void ApplyAsyncAny(const boost::any& command, const ApplyHandler& handler) {
    (void)command;
    (void)handler;
  }

 protected:
  std::mutex m_eventMutex;
  std::map<TypeId, EventHandler> m_eventHandlers;
};

}  // namespace rics

#endif  // RICS_IADAPTER_H
