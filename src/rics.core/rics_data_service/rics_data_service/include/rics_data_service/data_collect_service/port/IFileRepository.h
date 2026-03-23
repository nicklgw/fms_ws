#pragma once
#include <rics_data_service/RicsDefines.h>
#include <rics_data_service/data_collect_service/domain/Typedefine.h>

#include <list>
#include <string>
#include <vector>
/// #include "CacheReportor.pb.h"
namespace rics {
namespace data_collect {
class IFileRepository {
 public:
  /**
   * @brief 删除文件
   * @param
   * @return true 删除成功  false 删除失败
   */
  virtual std::string DelFile(const std::string& filepath) = 0;

  /**
   * @brief 是否有可发送文件
   * @param
   * @return
   */
  virtual bool HasSendableFile() = 0;

  /**
   * @brief  获取当前压缩文件目录下所有下所文件名
   * @param
   * @return
   */
  virtual void GetSendbleFiles(const std::string& dir,
                               std::vector<rics::OfflineDataInfo>& filelist) = 0;

  /**
   * @brief  获取当前缓存文件中目录可生成离线文件的离线文件名
   * @param
   * @return
   */
  virtual void GetCompressibleFiles(const std::string& dir,
                                    std::vector<rics::OfflineDataInfo>& filelist) = 0;

  /**
   * @brief 获取首个压缩标记
   * @param
   * @return
   */
  virtual void GetCompressedTag(std::string& md5, std::string& filename) = 0;

  /**
   * @brief  缓存 处理(缓存输入普通文件 。。。输入压缩文件)
   * @param
   * @return
   */
  virtual void CacheProc(const std::string& compressFilename = "") = 0;

  /**
   * @brief: 分拆缓存数据，按照topic 分拆合并
   * @param:
   * @return:
   */
  virtual void SortoutFromCache(const std::string& topic, const std::string& msg) = 0;
};

}  // namespace data_collect
}  // namespace rics