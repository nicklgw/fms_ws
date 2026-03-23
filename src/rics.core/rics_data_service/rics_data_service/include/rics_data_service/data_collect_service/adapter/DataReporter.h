#pragma once
#include <rics_data_service/ITransport.h>
#include <rics_data_service/RicsDefines.h>
#include <rics_data_service/data_collect_service/adapter/ObjectInjection.h>
#include <rics_data_service/data_collect_service/port/IDataReporter.h>

namespace rics {
namespace data_collect {
class DataReporter : public IDataReporter {
 public:
  DataReporter();
  ~DataReporter();

  bool IsConnected() override;

  bool IsReConnect() override;

  bool SendMsg(const std::string &topicName, const std::string &msg) override;

  void Init() override;

 private:
  // 对象注入器
  std::shared_ptr<ObjectInjection> m_objInjection;
  // MQTT 接口
  std::shared_ptr<rics::ITransport> m_TransPort;

  RicsBusinessOption m_ricsBusinessOption;
  bool m_LastConnectStu;
  int m_LastMessageId;
};

}  // namespace data_collect
}  // namespace rics