#pragma once
#include <rics_data_service/RicsDefines.h>
#include <rics_data_service/data_collect_service/adapter/ObjectInjection.h>
#include <rics_data_service/data_collect_service/domain/Defines.h>
#include <rics_data_service/data_collect_service/port/IFileRepository.h>
#include <rics_data_service/data_collect_service/port/IHttpProcessor.h>

namespace rics {
namespace data_collect {
class GetFileNameProcessor : public IHttpProcessor {
 public:
  GetFileNameProcessor();
  void Init();

  std::string Method() override;

  std::string URL() override;

  std::string ContentType() override;

  std::string Serialize(const boost::any& data, Poco::Net::HTTPRequest& req) override;

  boost ::any Deserialize(const std::string& strdata) override;

 private:
  std::shared_ptr<ObjectInjection> m_objInjection;
  std::shared_ptr<IFileRepository> m_FileRepository;
};

class PostFileProcessor : public IHttpProcessor {
 public:
  PostFileProcessor();
  void Init();

  std::string Method() override;

  std::string URL() override;

  std::string ContentType() override;

  std::string Serialize(const boost::any& data, Poco::Net::HTTPRequest& req) override;

  boost ::any Deserialize(const std::string& strdata) override;

 private:
  std::shared_ptr<ObjectInjection> m_objInjection;
  RicsBusinessOption m_ricsBusinessOption;
  DataCollectOption m_dataCollectOption;
  std::shared_ptr<IFileRepository> m_FileRepository;
};

}  // namespace data_collect
}  // namespace rics