#pragma once
#include <pthread.h>
#include <rics_data_service/data_collect_service/adapter/ObjectInjection.h>
#include <rics_data_service/data_collect_service/domain/DataCollectRepository.h>
#include <rics_data_service/data_collect_service/domain/Defines.h>
#include <rics_data_service/data_collect_service/domain/FileRepository.h>
#include <rics_data_service/data_collect_service/domain/Typedefine.h>
#include <rics_data_service/data_collect_service/port/IDataCollectService.h>
#include <rics_data_service/data_collect_service/port/IDataReporter.h>
#include <rics_data_service/data_collect_service/port/IFileUpLoader.h>

#include <memory>
#include <mutex>
#include <string>

namespace rics {
namespace data_collect {
class DataCollectService : public IDataCollectService {
 public:
  DataCollectService();
  ~DataCollectService();
  void Init() override;

  void PushData(MqttSimple& pMessage) override;

  void ListFile(std::vector<rics::OfflineDataInfo>& filelist) override;

  void UpLoadFile() override;

  void ReportData() override;

  void ClearCache(const std ::string& compressFilename = "") override;

  std::string DeleteFiles(const std::string& filepath) override;

 private:
  void CachePro(const std::string& compressFilename = "");

  std::shared_ptr<ObjectInjection> m_objInjection;
  std::shared_ptr<IDataCollectRepository> m_DataCollectRepository;
  std::shared_ptr<IFileRepository> m_FileRepository;
  std::shared_ptr<IDataReporter> m_DataReporter;
  std::shared_ptr<IFileUpLoader> m_FileUpLoader;
  DataCollectOption m_dataCollectOption;
  std::mutex g_mutex_lock;
};

}  // namespace data_collect
}  // namespace rics