#pragma once
#include <rics_data_service/RicsDefines.h>

#include <vector>

namespace rics {

namespace data_collect {
class IDataCollectService {
 public:
  virtual void Init() = 0;

  /**
   * @brief 入数据
   * @param
   * @return
   */
  virtual void PushData(MqttSimple& pMessage) = 0;

  /**
   * @brief 返回文件列表
   * @param
   * @return
   */
  virtual void ListFile(std::vector<rics::OfflineDataInfo>& filelist) = 0;

  /**
   * @brief 上传文件
   * @param
   * @return
   */
  virtual void UpLoadFile() = 0;

  /**
   * @brief 上传数据
   * @param
   * @return
   */
  virtual void ReportData() = 0;

  /**
   * @brief 清缓存
   * @param
   * @return
   */
  virtual void ClearCache(const std::string& compressFilename = "") = 0;

  /**
   * @brief 删除指定文件
   * @param
   * @return
   */
  virtual std::string DeleteFiles(const std::string& filepath) = 0;
};

}  // namespace data_collect
}  // namespace rics