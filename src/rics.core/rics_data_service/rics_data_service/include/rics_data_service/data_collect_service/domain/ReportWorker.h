#pragma once
#include <rics_data_service/data_collect_service/adapter/ObjectInjection.h>
#include <rics_data_service/data_collect_service/port/IDataCollectService.h>

namespace rics {
namespace data_collect {
class ReportWorker {
 public:
  ReportWorker();
  ~ReportWorker();

  void Init();

  /**
   * @brief 上报在线数据
   * @param
   * @return
   */
  void ReportData();

  /**
   * @brief 上报压缩文件
   * @param
   * @return
   */
  void UploadFile();

 private:
  std::shared_ptr<ObjectInjection> m_objInjection;

  std::shared_ptr<IDataCollectService> m_dataCollectService;
};

}  // namespace data_collect

}  // namespace rics