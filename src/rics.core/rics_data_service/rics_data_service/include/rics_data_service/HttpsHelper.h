#ifndef HTTPS_HELPER_H
#define HTTPS_HELPER_H

#include <iostream>
#include <memory>
#include <mutex>
#include <string>

// 引入Poco网络库的相关头文件
#include <Poco/Net/HTTPClientSession.h>
#include <Poco/Net/HTTPRequestHandlerFactory.h>
#include <Poco/Net/HTTPSClientSession.h>
#include <Poco/Net/HTTPServer.h>
#include <Poco/Net/HTTPServerParams.h>
#include <Poco/Net/PrivateKeyPassphraseHandler.h>
#include <Poco/Net/SSLManager.h>

namespace rics {

// RicsKeyFileHandler 类用于处理私钥请求，继承自 Poco::Net::PrivateKeyPassphraseHandler
class RicsKeyFileHandler : public Poco::Net::PrivateKeyPassphraseHandler {
 public:
  // 构造函数，server 参数表示是否是服务器端，privateKey 是私钥字符串
  RicsKeyFileHandler(bool server, const std::string& privateKey);
  ~RicsKeyFileHandler();

  // 当需要提供私钥时调用此方法
  void onPrivateKeyRequested(const void* pSender, std::string& privateKey) override;

 private:
  std::string m_strPrivateKey;  // 存储私钥字符串
};

// HttpsHelper 类提供了创建 HTTPS 客户端和服务器的方法
class HttpsHelper {
 public:
  // 获取单例实例的方法，pemPath 是证书路径，privateKey 是私钥
  static HttpsHelper& Instance(const std::string& pemPath = "", const std::string& privateKey = "");

  // 创建一个 HTTPS 客户端会话
  std::shared_ptr<Poco::Net::HTTPSClientSession> CreateHttpsClient(const std::string& host,
                                                                   uint16_t port);

  // 创建一个 HTTPS 服务器
  std::shared_ptr<Poco::Net::HTTPServer> CreateHttpsServer(
      Poco::Net::HTTPRequestHandlerFactory::Ptr pFactory, uint16_t port,
      Poco::Net::HTTPServerParams::Ptr pParams);

  ~HttpsHelper();  // 析构函数

 private:
  HttpsHelper(const std::string& pemPath, const std::string& privateKey);  // 私有构造函数

 private:
  Poco::Net::Context::Ptr m_pClientContext;  // 客户端 SSL 上下文
  Poco::Net::Context::Ptr m_pServerContext;  // 服务器 SSL 上下文

  static std::unique_ptr<HttpsHelper> m_pInstance;  // 单例实例指针
};

}  // namespace rics

#endif  // HTTPS_HELPER_H