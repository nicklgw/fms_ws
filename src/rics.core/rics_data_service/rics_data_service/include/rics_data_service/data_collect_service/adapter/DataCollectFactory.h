#pragma once
#include <rics_data_service/ITransport.h>
#include <rics_data_service/RicsDefines.h>
#include <rics_data_service/data_collect_service/adapter/DataEventConsumer.h>
#include <rics_data_service/data_collect_service/adapter/ThreadPool.h>
#include <rics_data_service/data_collect_service/domain/Defines.h>
#include <rics_data_service/data_collect_service/domain/ReportWorker.h>

namespace rics {
namespace data_collect {
class DataCollectFactory {
 public:
  DataCollectFactory();
  ~DataCollectFactory();

  std::shared_ptr<IDataCollectService> GetDataCollectService();

  void CreateDataCollectObj(const std::shared_ptr<ITransport> &pTransport,
                            const DataCollectOption &dcoptipn,
                            const RicsBusinessOption &ricsBusinessOption);

 private:
  std::shared_ptr<ThreadPool> m_pthreadPool;
  shared_ptr<DataEventConsumer> m_pdataEventConsumer;
};
}  // namespace data_collect
}  // namespace rics