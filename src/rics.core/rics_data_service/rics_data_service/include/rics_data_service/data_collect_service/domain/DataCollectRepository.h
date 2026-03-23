#pragma once
#include <rics_data_service/RicsDefines.h>
#include <rics_data_service/data_collect_service/adapter/ObjectInjection.h>
#include <rics_data_service/data_collect_service/domain/Typedefine.h>
#include <rics_data_service/data_collect_service/port/IDataCollectRepository.h>

#include <boost/asio.hpp>
#include <boost/atomic.hpp>
#include <boost/lockfree/spsc_queue.hpp>

namespace rics {
namespace data_collect {
class DataCollectRepository : public IDataCollectRepository {
 public:
  DataCollectRepository();
  ~DataCollectRepository();
  void Init() override;

  bool Enqueue(MqttSimple& pMessage) override;

  void Peek(MqttSimple& pMessage) override;

  void Pop(bool popAll = false) override;

  bool HasData() override;

  bool Full() override;

 private:
  /**
   * @brief 格式化订阅的消息去除回车、空格
   * @param
   * @return
   */
  void CorrectInfo(MqttSimple& pMessage);

  /**
   * @brief 修改topic 中间段sn
   * @param
   * @return
   */
  std ::string CorrectTopic(std::string& topic);

  /**
   * @brief 修改gps message 字段
   * @param
   * @return
   */
  std::string CorrectMessage(const std::string& topic, const std::string& msg);

  boost::lockfree::spsc_queue<MqttSimple, boost::lockfree::capacity<QUEUEMAX>> m_cache;

  boost::atomic_int m_queueRearIdx;

  RicsBusinessOption m_ricsBusinessOption;

  // std::shared_ptr<interface::IAdapter> m_Gpsinfo;
  // std::shared_ptr<interface::IAdapter> m_pRouterAdapter;

  std::shared_ptr<ObjectInjection> m_objInjection;
};
}  // namespace data_collect
}  // namespace rics