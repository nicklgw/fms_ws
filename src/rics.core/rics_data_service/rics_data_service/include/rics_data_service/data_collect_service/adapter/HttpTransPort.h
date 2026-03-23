#pragma once
#include <rics_data_service/RicsDefines.h>
#include <rics_data_service/data_collect_service/adapter/ObjectInjection.h>
#include <rics_data_service/data_collect_service/domain/Authentication.h>
#include <rics_data_service/data_collect_service/domain/Defines.h>
#include <rics_data_service/data_collect_service/domain/Typedefine.h>
#include <rics_data_service/data_collect_service/port/IHttpProcessor.h>

#include <atomic>
#include <boost/asio.hpp>
#include <boost/type_index.hpp>
#include <cstdint>
#include <map>
#include <memory>

namespace rics {
namespace data_collect {
class HttpTransPort {
 public:
  explicit HttpTransPort(const std::string& host, std::uint16_t port);
  ~HttpTransPort();

  bool SendCmd(const std::shared_ptr<IHttpProcessor> processor, const boost::any& serializeData,
               std::string& replyBody);

 private:
  std::string m_host;
  std::uint16_t m_port;

  std::shared_ptr<ObjectInjection> m_objInjection;
  DataCollectOption m_dataCollectOption;
  std::shared_ptr<IHttpProcessor> processor;
  std::shared_ptr<Authentication> m_authentication;
};
}  // namespace data_collect
}  // namespace rics