#include <Poco/FileStream.h>
#include <Poco/Net/HTMLForm.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/StringPartSource.h>
#include <Poco/StreamCopier.h>
#include <Poco/String.h>
#include <jsoncpp/json/json.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <rics_data_service/JsonHelper.h>
#include <rics_data_service/Logging.h>
#include <rics_data_service/data_collect_service/adapter/HttpProcessor.h>
#include <rics_data_service/data_collect_service/domain/Typedefine.h>

#include <boost/algorithm/string.hpp>
#include <boost/shared_ptr.hpp>
#include <memory>

using namespace std;
using namespace rics;
using namespace rics::data_collect;
using namespace Poco::Net;

string GetFileNameProcessor::Method() { return HTTPRequest::HTTP_GET; }

GetFileNameProcessor::GetFileNameProcessor() { Init(); }

void GetFileNameProcessor::Init() {
  m_objInjection = ObjectInjection::Getinstance();
  m_FileRepository =
      boost::any_cast<std::shared_ptr<IFileRepository>>(m_objInjection->GetObject(k_FileOp));
}

string GetFileNameProcessor::URL() { return "/api-gateway/gateway/cache/checkFile/"; }

string GetFileNameProcessor::ContentType() { return "application/json"; }

string GetFileNameProcessor::Serialize(const boost::any& command, HTTPRequest& req) {
  (void)command;
  string md5 = "";
  string filename = "";
  m_FileRepository->GetCompressedTag(md5, filename);
  req.setContentType(this->ContentType());
  req.setURI(this->URL() + md5);
  req.setMethod(this->Method());
  return {};
}

boost::any GetFileNameProcessor::Deserialize(const std::string& strData) {
  try {
    Json::Reader reader;
    Json::Value rootObj;
    if (!reader.parse(strData, rootObj) || !rootObj.isObject()) {
      // RICS_DEBUG("Deserialize cannot parse message");
      return {};
    }
    auto event = std::make_shared<ApplyofFileRequest>();
    json_helper::GetValue(event->code, "code", rootObj);
    json_helper::GetValue(event->message, "message", rootObj);

    return event;
  } catch (...) {
    // RICS_DEBUG("please check httpServer is connected ");
    return {};
  }
}

PostFileProcessor::PostFileProcessor() { Init(); }

void PostFileProcessor::Init() {
  m_objInjection = ObjectInjection::Getinstance();
  m_FileRepository =
      boost::any_cast<std::shared_ptr<IFileRepository>>(m_objInjection->GetObject(k_FileOp));
  m_ricsBusinessOption =
      boost::any_cast<RicsBusinessOption>(m_objInjection->GetObject(k_ConfigRics));
  m_dataCollectOption =
      boost::any_cast<DataCollectOption>(m_objInjection->GetObject(k_ConfigDataCollect));
}

string PostFileProcessor::Method() { return HTTPRequest::HTTP_POST; }

string PostFileProcessor::URL() { return "/api-gateway/gateway/cache/upload"; }

string PostFileProcessor::ContentType() { return "multipart/form-data"; }

string PostFileProcessor::Serialize(const boost::any& anyCommand, HTTPRequest& req) {
  (void)anyCommand;
  // 文件内容序列化
  HTMLForm formData(HTMLForm::ENCODING_MULTIPART);
  string md5 = "";
  string filepath = "";
  m_FileRepository->GetCompressedTag(md5, filepath);

  ostringstream out_str;
  Poco::FileInputStream i_str(filepath);
  Poco::StreamCopier::copyStream(i_str, out_str);

  int ipos = filepath.rfind('/');
  formData.set("fileName", filepath.substr(ipos + 1));
  ipos = filepath.find('-');
  string topic = filepath.substr(ipos + 1);
  boost::algorithm::erase_all(topic, ".bz2");
  formData.set("topic", topic);
  formData.set("sn", m_ricsBusinessOption.robotConfig.strSN);

  req.setContentType(this->ContentType());
  req.setURI(this->URL());
  req.setMethod(this->Method());

  formData.addPart(
      "file", new Poco::Net::StringPartSource(out_str.str(), "application/octet-stream", filepath));
  formData.prepareSubmit(req);
  ostringstream ss;
  formData.write(ss);

  return ss.str();
}

boost::any PostFileProcessor::Deserialize(const std::string& strData) {
  try {
    Json::Reader reader;
    Json::Value rootObj;
    if (!reader.parse(strData, rootObj) || !rootObj.isObject()) {
      // RICS_DEBUG("Deserialize cannot parse message");
      return {};
    }
    auto event = std::make_shared<ApplyofFileRequest>();
    json_helper::GetValue(event->code, "code", rootObj);
    json_helper::GetValue(event->message, "message", rootObj);
    return event;
  } catch (...) {
    // RICS_DEBUG("please check httpServer is connected ");
    return {};
  }
}
