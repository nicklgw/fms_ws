#include <rics_data_service/Logging.h>
#include <rics_data_service/data_collect_service/domain/DataCollectService.h>
#include <rics_data_service/data_collect_service/domain/ErrorCodeDefine.h>

using namespace std;
using namespace rics::data_collect;

DataCollectService::DataCollectService() {}

DataCollectService::~DataCollectService() {}

void DataCollectService::Init() {
  m_objInjection = ObjectInjection::Getinstance();
  m_FileRepository =
      boost::any_cast<shared_ptr<IFileRepository>>(m_objInjection->GetObject(k_FileOp));
  m_DataReporter =
      boost::any_cast<shared_ptr<IDataReporter>>(m_objInjection->GetObject(k_DataReport));
  m_FileUpLoader =
      boost::any_cast<shared_ptr<IFileUpLoader>>(m_objInjection->GetObject(k_FileUpload));
  m_dataCollectOption =
      boost::any_cast<DataCollectOption>(m_objInjection->GetObject(k_ConfigDataCollect));
  m_DataCollectRepository =
      boost::any_cast<shared_ptr<IDataCollectRepository>>(m_objInjection->GetObject(k_CacheOp));
}

void DataCollectService::PushData(MqttSimple& pMessage) {
  if (m_DataReporter->IsReConnect()) {
    vector<rics::OfflineDataInfo> fileList;
    m_FileRepository->GetCompressibleFiles(m_dataCollectOption.cacheFilePath + CompressedFilePath,
                                           fileList);
    for (auto& it : fileList) {
      CachePro(it.name);
    }
  }

  {
    std::lock_guard<std::mutex> lk(g_mutex_lock);
    m_DataCollectRepository->Enqueue(pMessage);
    // 缓存处理
    if (!m_DataCollectRepository->Full()) return;
  }

  CachePro();
}

void DataCollectService::ListFile(std::vector<rics::OfflineDataInfo>& fileList) {
  // 获取压缩文件列表
  m_FileRepository->GetSendbleFiles(m_dataCollectOption.cacheFilePath + CompressedFilePath,
                                    fileList);
  // 获取未压缩文件目录中可压缩的压缩列表
  m_FileRepository->GetCompressibleFiles(m_dataCollectOption.cacheFilePath + TempFilePath,
                                         fileList);
}

void DataCollectService::ReportData() {
  static int idx = 1;
  if (!m_DataReporter->IsConnected()) return;
  MqttSimple reportData;
  string topicname, msg;
  std::lock_guard<std::mutex> lk(g_mutex_lock);
  for (int cnt = 0; cnt < CYCLEMAX; cnt++) {
    if (m_DataCollectRepository->HasData()) {
      m_DataCollectRepository->Peek(reportData);
      topicname = reportData.topic;
      msg = reportData.message;
      if (m_DataReporter->SendMsg(topicname, msg)) {
        m_DataCollectRepository->Pop();
        if (++idx % 100 == 0) {
          RICS_INFO("ReportData:%s ", topicname.c_str());
          idx = 0;
        }
      }
    } else
      break;
  }
}

void DataCollectService::UpLoadFile() {
  string md5, filepath;
  if (!m_FileRepository->HasSendableFile()) return;
  int iret = m_FileUpLoader->UpLoadFile();
  if (iret == SUCCEED || iret == REPORTED) {
    // 发送成功 或者已上报 需要删除此压缩文件及待发送文件列表
    m_FileRepository->GetCompressedTag(md5, filepath);
    auto ret = DeleteFiles(filepath);
    if (!ret.empty()) RICS_WARN("delete files(%s) failed. %s", filepath.c_str(), ret.c_str());
    RICS_INFO("UpLoadFile:%s", filepath.c_str());
  }
}

void DataCollectService::ClearCache(const string& compressFilename) {
  RICS_INFO("put cache to  files ! (temp files or compress files)");
  CachePro(compressFilename);
}

void DataCollectService::CachePro(const string& compressFilename) {
  MqttSimple pOutMessage;
  {
    std::lock_guard<std::mutex> lk(g_mutex_lock);
    for (int i = 0; i < QUEUEMAX; i++) {
      if (m_DataCollectRepository->HasData()) {
        m_DataCollectRepository->Peek(pOutMessage);
        m_DataCollectRepository->Pop();
        m_FileRepository->SortoutFromCache(pOutMessage.topic, pOutMessage.message);
      }
    }
  }
  m_FileRepository->CacheProc(compressFilename);
}

std::string DataCollectService::DeleteFiles(const std::string& filepath) {
  return m_FileRepository->DelFile(filepath);
}
