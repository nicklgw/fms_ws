#include <rics_data_service/data_collect_service/adapter/HttpProcessor.h>
#include <rics_data_service/data_collect_service/domain/FileUploader.h>
#include <rics_data_service/data_collect_service/domain/Typedefine.h>

using namespace std;
using namespace rics::data_collect;

FileUploader::FileUploader(const DataCollectOption &dcOption) : m_dataCollectOption(dcOption) {}

FileUploader::~FileUploader() {}

void FileUploader::Init() {
  m_HttpTransPort = make_shared<HttpTransPort>(m_dataCollectOption.uploadFmsConfig.Host,
                                               m_dataCollectOption.uploadFmsConfig.Port);
  m_FileNamePost = make_shared<GetFileNameProcessor>();
  m_FilePost = make_shared<PostFileProcessor>();
}

int FileUploader::PostFileName() {
  struct PostFileRequest postFileData;
  string replyBody;
  bool bret = m_HttpTransPort->SendCmd(m_FileNamePost, postFileData, replyBody);
  if (!bret) {
    return -1;
  }
  try {
    auto any = m_FileNamePost->Deserialize(replyBody);
    if (any.empty()) {
      return -1;
    }
    auto reply = boost::any_cast<shared_ptr<ApplyofFileRequest>>(any);
    return reply->code;
  } catch (...) {
    return -1;
  }
}

int FileUploader::PostFile() {
  struct PostFileRequest postFileData;
  string replyBody;
  bool bret = m_HttpTransPort->SendCmd(m_FilePost, postFileData, replyBody);
  if (!bret) {
    return -1;
  }
  try {
    auto any = m_FilePost->Deserialize(replyBody);
    if (any.empty()) {
      return -1;
    }
    auto reply = boost::any_cast<shared_ptr<ApplyofFileRequest>>(any);
    return reply->code;
  } catch (...) {
    return -1;
  }
}

int FileUploader::UpLoadFile() {
  int ret = PostFileName();
  if (ret != 0) return ret;
  ret = PostFile();
  return ret;
}