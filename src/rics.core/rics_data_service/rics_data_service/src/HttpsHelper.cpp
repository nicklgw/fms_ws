// HttpsHelper.cpp
#include "rics_data_service/HttpsHelper.h"

#include <Poco/Net/AcceptCertificateHandler.h>
#include <Poco/Net/SSLManager.h>
#include <Poco/Net/SecureServerSocket.h>

#include "rics_data_service/Logging.h"

using namespace Poco::Net;

namespace rics {

static std::once_flag g_oc_flag;
std::unique_ptr<HttpsHelper> HttpsHelper::m_pInstance = nullptr;
RicsKeyFileHandler::RicsKeyFileHandler(bool server, const std::string& privateKey)
    : PrivateKeyPassphraseHandler(server), m_strPrivateKey(privateKey) {}

RicsKeyFileHandler::~RicsKeyFileHandler() {}

void RicsKeyFileHandler::onPrivateKeyRequested(const void* pSender, std::string& privateKey) {
  (void)pSender;
  privateKey = m_strPrivateKey;
}

HttpsHelper::HttpsHelper(const std::string& pemPath, const std::string& privateKey) {
  SSLManager::InvalidCertificateHandlerPtr pClientCertHandler(new AcceptCertificateHandler(false));
  m_pClientContext = new Context(Context::TLSV1_2_CLIENT_USE, "", Context::VERIFY_NONE);
  SSLManager::instance().initializeClient(nullptr, pClientCertHandler, m_pClientContext);

  SSLManager::PrivateKeyPassphraseHandlerPtr pSrvKeyHandler(
      new RicsKeyFileHandler(true, privateKey));
  SSLManager::InvalidCertificateHandlerPtr pSrvCertHandler(new AcceptCertificateHandler(true));

  std::string serverKey = pemPath + "server_key_https.pem";
  std::string serverCrt = pemPath + "server_crt_https.pem";
  std::string caCrt = pemPath + "ca_crt_https.pem";

  m_pServerContext =
      new Context(Context::TLSV1_2_SERVER_USE, serverKey, serverCrt, caCrt, Context::VERIFY_RELAXED,
                  9, false, "HIGH:!ADH:!RC4:!LOW:!EXP:!MD5:@STRENGTH");

  SSLManager::instance().initializeServer(pSrvKeyHandler, pSrvCertHandler, m_pServerContext);
}

HttpsHelper::~HttpsHelper() {}

HttpsHelper& HttpsHelper::Instance(const std::string& pemPath, const std::string& privateKey) {
  std::call_once(g_oc_flag, [&]() {
    // std::cout << "HttpsHelper::Instance() called" << std::endl;
    RICS_INFO("HttpsHelper::Instance() called");
    m_pInstance.reset(new HttpsHelper(pemPath, privateKey));
  });
  return *m_pInstance;
}

std::shared_ptr<HTTPSClientSession> HttpsHelper::CreateHttpsClient(const std::string& host,
                                                                   uint16_t port) {
  auto pSession = std::make_shared<HTTPSClientSession>(m_pClientContext);
  pSession->setHost(host);
  pSession->setPort(port);
  return pSession;
}

std::shared_ptr<Poco::Net::HTTPServer> HttpsHelper::CreateHttpsServer(
    Poco::Net::HTTPRequestHandlerFactory::Ptr pFactory, uint16_t port,
    Poco::Net::HTTPServerParams::Ptr pParams) {
  Poco::Net::SecureServerSocket svs(port, 64, m_pServerContext);
  auto pServer = std::make_shared<Poco::Net::HTTPServer>(pFactory, svs, pParams);
  return pServer;
}

}  // namespace rics