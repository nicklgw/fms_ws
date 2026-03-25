#pragma once
#include <rics_data_service/RicsDefines.h>
#include <rics_data_service/data_collect_service/domain/Defines.h>
#include <rics_data_service/data_collect_service/domain/Typedefine.h>
#include <rics_data_service/data_collect_service/port/IFileRepository.h>
#include <sys/inotify.h>

#include <boost/atomic.hpp>
#include <boost/lockfree/spsc_queue.hpp>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

namespace rics {
namespace data_collect {
class FileRepository : public IFileRepository {
 public:
  explicit FileRepository(const DataCollectOption& dcOption,
                          const RicsBusinessOption& ricsBusinessoption);
  ~FileRepository();

  std::string DelFile(const std::string& filepath) override;

  bool HasSendableFile() override;

  void GetSendbleFiles(const std::string& dir,
                       std::vector<rics::OfflineDataInfo>& filelist) override;

  void GetCompressibleFiles(const std::string& dir,
                            std::vector<rics::OfflineDataInfo>& filelist) override;

  void GetCompressedTag(std::string& md5, std::string& filename) override;

  void CacheProc(const std::string& compressFilename = "") override;

  void SortoutFromCache(const std::string& topic, const std::string& msg) override;

 private:
  std::map<std::string, std::string> m_SortoutDatas;
  int m_notifyFd, m_notifyWd;
  bool m_loopNotify;

  bool FindEarliestFile(const std::string& dir, std::string& filename);

  void DistributeCache(const std::string& topic, const std::string& msg);

  bool PopSortoutDatas(std::string& topic, std::string& msg);

  double Str2Double(const std::string& stamp);

  bool CalcMd5File(const std::string& filepath, std::string& outMd5);

  /**
   * @brief 将磁盘上的待发送文件信息同步到可发送文件信息表中
   * @param
   * @return
   */
  void SyncSendbleFilesList();

  int GetCurrFileSize(const std::string& filepath);

  void SetCompressedTag(const std::string& md5, const std::string& filename);

  void EraseCompressedTag(const std::string& compFileName = "");

  int Compress2File(const std::string& srcpath, const std::string& outPath);

  std::string CreateNewFileName(const std::string& topicName);

  /**
   * @brief 获取磁盘剩余空间
   * @param
   * @return 返回剩余空间占总空间的百分比
   */
  int GetDiskFreeSpace();

  void FreeDisk();

  DataCollectOption m_dcOption;

  RicsBusinessOption m_ricsBusinessOption;

  // 可操作压缩文件
  std::map<std::string, std::string> m_SendbleFiles;

  std::mutex m_SendbleFilesMtx, m_NewCompressFileMt, m_CompressMt;

  std::thread m_notifyThread;

  struct FilenameInfo {
    std::string filePath;
    std::string fileName;
    double stamp;
  };

  struct CompressInfo {
    std::string tempFilePath;
    std::string cmpFilePath;
  };

  void CompressHandler(std::list<CompressInfo>& cmpressinfo);

  std::string GetFilenameFromPath(const std::string& path);

  int WriteSome(const std::string& filepath, const std::string& note, int wsize);

  void WriteTempFile(std::list<std::string>& topicList);

  std::string GetDir(const std::string& filepath);

  /**
   * @brief 获取指定目录下的所有文件
   * @param
   * @return
   */
  bool GetAllFilesFromSpecDir(const std::string& dir, std::list<FilenameInfo>& fileList);

  //  bool
  //  IsTopicNeedCache(const std::string& topic);

  void FormatMsg(std::string& msg);

  /**
   * @brief inotify 初始化
   * @param
   * @return
   */
  void NotifyInit();

  /**
   * @brief 读取监控事件
   * @param
   * @return
   */
  int ReadEvents(char* buf);

  /**
   * @brief 处理监控事件
   * @param
   * @return
   */
  void HandleEvents(struct inotify_event* in);

  void NotifyEvents();

 private:
  static constexpr int kResyncThreshold = 1000;  // 100秒周期。每调用 N 次触发一次全量扫描
  int m_call_counter;                           // 调用次数计数器
};

}  // namespace data_collect
}  // namespace rics