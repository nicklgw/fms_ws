#pragma once
#include <rics_data_service/data_collect_service/adapter/HttpTransPort.h>
#include <rics_data_service/data_collect_service/domain/Defines.h>
#include <rics_data_service/data_collect_service/port/IFileUpLoader.h>
#include <rics_data_service/data_collect_service/port/IHttpProcessor.h>

#include <boost/asio.hpp>
#include <boost/type_index.hpp>

namespace rics {
namespace data_collect {
class FileUploader : public IFileUpLoader {
 public:
  FileUploader(const DataCollectOption &dcOption);
  ~FileUploader();

  int UpLoadFile() override;

  void Init() override;

 private:
  DataCollectOption m_dataCollectOption;

  std::shared_ptr<HttpTransPort> m_HttpTransPort;
  std::shared_ptr<IHttpProcessor> m_FileNamePost, m_FilePost;

  int PostFileName();
  int PostFile();
};

}  // namespace data_collect
}  // namespace rics