#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/Net/WebSocket.h>
#include <Poco/URI.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/sha.h>
#include <rics_data_service/HttpsHelper.h>
#include <rics_data_service/Logging.h>
#include <rics_data_service/data_collect_service/adapter/HttpProcessor.h>
#include <rics_data_service/data_collect_service/adapter/HttpTransPort.h>

#include <boost/date_time/local_time/local_time.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <iostream>

using namespace std;
using namespace rics::data_collect;
using namespace Poco;
using namespace Poco::Net;
using namespace rics;

HttpTransPort::HttpTransPort(const string& host, uint16_t port) : m_host(host), m_port(port) {
  m_objInjection = ObjectInjection::Getinstance();
  m_dataCollectOption =
      boost::any_cast<DataCollectOption>(m_objInjection->GetObject(k_ConfigDataCollect));
  m_authentication = make_shared<Authentication>(m_dataCollectOption.regFmsConfig.Key,
                                                 m_dataCollectOption.regFmsConfig.Usrname);
}

HttpTransPort::~HttpTransPort() {}

bool HttpTransPort::SendCmd(const std::shared_ptr<IHttpProcessor> processor,
                            const boost::any& serializeData, string& replyBody) {
  HTTPRequest req;
  auto reqBody = std::make_shared<std::string>(std::move(processor->Serialize(serializeData, req)));
  try {
    auto pSession = HttpsHelper::Instance().CreateHttpsClient(m_host, m_port);
    pSession->setTimeout({2, 0}, {2, 0}, {2, 0});
    // ����ǩ��
    string now = m_authentication->GetNowGMT();
    req.add("date", now);
    unsigned char hashBody[SHA256_DIGEST_LENGTH];
    string hashBodyStr, bodySignature;
    memset(hashBody, 0, sizeof(hashBody));
    if (!reqBody->empty()) {
      m_authentication->Sha256(*reqBody, hashBody);
      hashBodyStr = m_authentication->Base64Encode(hashBody, SHA256_DIGEST_LENGTH);
      bodySignature = "SHA-256=" + hashBodyStr;
      req.add("digest", bodySignature);
    }
    string authorization =
        m_authentication->GetSignature(req.getMethod(), req.getURI(), bodySignature, now);
    req.add("authorization", authorization);
    if (reqBody->empty()) {
      pSession->sendRequest(req);
    } else {
      req.setContentLength(reqBody->length());
      pSession->sendRequest(req) << (*reqBody);
    }
    HTTPResponse res;
    auto& is = pSession->receiveResponse(res);
    string reply(std::istreambuf_iterator<char>(is), {});
    replyBody = reply;
    return true;
  } catch (const Poco::Exception& ex) {
    RICS_WARN("receive from [%s] failed(PocoException): %s", processor->URL().c_str(), ex.what());
  } catch (const std::exception& ex) {
    RICS_WARN("receive from [%s] failed(stdException): %s", processor->URL().c_str(), ex.what());
  }
  return false;
}
