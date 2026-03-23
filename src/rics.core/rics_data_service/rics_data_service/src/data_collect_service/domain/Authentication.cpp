#include <openssl/core_names.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/sha.h>
#include <rics_data_service/data_collect_service/domain/Authentication.h>

#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/date_time/local_time/local_time.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <iostream>

using namespace std;
using namespace rics::data_collect;
using namespace boost::gregorian;
using namespace boost::archive::iterators;

Authentication::Authentication(string key, string usrname) : m_Key(key), m_UsrName(usrname) {}

Authentication::~Authentication() {}

// void Authentication::Sha256(const std::string str, unsigned char* hash) {
//   SHA256_CTX sha256;
//   SHA256_Init(&sha256);
//   SHA256_Update(&sha256, str.c_str(), str.size());
//   SHA256_Final(hash, &sha256);
// }

// int Authentication::HMAC256(std::string data, std::string key, unsigned char* out) {
//   HMAC_CTX* ctx = HMAC_CTX_new();  // 改用指针方式
//   unsigned int len;
//   HMAC_Init_ex(ctx, key.c_str(), key.length(), EVP_sha256(), NULL);
//   HMAC_Update(ctx, (unsigned char*)data.c_str(), data.length());
//   HMAC_Final(ctx, out, &len);
//   HMAC_CTX_free(ctx);  // 替换 cleanup 方法
//   return len;
// }

void Authentication::Sha256(const std::string str, unsigned char* hash) {
  EVP_MD_CTX* ctx = EVP_MD_CTX_new();
  if (!ctx) throw std::runtime_error("EVP_MD_CTX_new failed");
  if (EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) != 1)
    throw std::runtime_error("EVP_DigestInit_ex failed");
  if (EVP_DigestUpdate(ctx, str.c_str(), str.size()) != 1)
    throw std::runtime_error("EVP_DigestUpdate failed");
  unsigned int len = 0;
  if (EVP_DigestFinal_ex(ctx, hash, &len) != 1)
    throw std::runtime_error("EVP_DigestFinal_ex failed");
  EVP_MD_CTX_free(ctx);
}

int Authentication::HMAC256(std::string data, std::string key, unsigned char* out) {
#if OPENSSL_VERSION_NUMBER >= 0x30000000L
  // OpenSSL 3.0+ 推荐方式
  EVP_MAC* mac = EVP_MAC_fetch(NULL, "HMAC", NULL);
  if (!mac) throw std::runtime_error("EVP_MAC_fetch failed");
  EVP_MAC_CTX* ctx = EVP_MAC_CTX_new(mac);
  if (!ctx) {
    EVP_MAC_free(mac);
    throw std::runtime_error("EVP_MAC_CTX_new failed");
  }

  OSSL_PARAM params[2];
  params[0] = OSSL_PARAM_construct_utf8_string("digest", (char*)"SHA256", 0);
  params[1] = OSSL_PARAM_construct_end();

  if (EVP_MAC_init(ctx, reinterpret_cast<const unsigned char*>(key.c_str()), key.length(),
                   params) != 1) {
    EVP_MAC_CTX_free(ctx);
    EVP_MAC_free(mac);
    throw std::runtime_error("EVP_MAC_init failed");
  }

  if (EVP_MAC_update(ctx, reinterpret_cast<const unsigned char*>(data.c_str()), data.length()) !=
      1) {
    EVP_MAC_CTX_free(ctx);
    EVP_MAC_free(mac);
    throw std::runtime_error("EVP_MAC_update failed");
  }

  size_t out_len = 0;
  if (EVP_MAC_final(ctx, out, &out_len, EVP_MAX_MD_SIZE) != 1) {
    EVP_MAC_CTX_free(ctx);
    EVP_MAC_free(mac);
    throw std::runtime_error("EVP_MAC_final failed");
  }

  EVP_MAC_CTX_free(ctx);
  EVP_MAC_free(mac);
  return static_cast<int>(out_len);
#else
  // OpenSSL 1.1 及以下
  unsigned int len = 0;
  HMAC_CTX* ctx = HMAC_CTX_new();
  if (!ctx) throw std::runtime_error("HMAC_CTX_new failed");
  if (HMAC_Init_ex(ctx, key.c_str(), key.length(), EVP_sha256(), nullptr) != 1)
    throw std::runtime_error("HMAC_Init_ex failed");
  if (HMAC_Update(ctx, reinterpret_cast<const unsigned char*>(data.c_str()), data.length()) != 1)
    throw std::runtime_error("HMAC_Update failed");
  if (HMAC_Final(ctx, out, &len) != 1) throw std::runtime_error("HMAC_Final failed");
  HMAC_CTX_free(ctx);
  return len;
#endif
}

string Authentication::Base64Encode(const unsigned char* hash, int len, bool newLine) {
  BIO* bmem = NULL;
  BIO* b64 = NULL;
  BUF_MEM* bptr;

  b64 = BIO_new(BIO_f_base64());
  if (!newLine) {
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
  }
  bmem = (BIO_new(BIO_s_mem()));
  b64 = BIO_push(b64, bmem);

  BIO_write(b64, hash, len);
  BIO_flush(b64);
  BIO_get_mem_ptr(b64, &bptr);
  BIO_set_close(b64, BIO_NOCLOSE);

  char* buff = (char*)malloc(bptr->length + 1);
  memcpy(buff, bptr->data, bptr->length);
  buff[bptr->length] = 0;
  BIO_free_all(b64);

  string str = string(buff, bptr->length);
  free(buff);
  return str;
}

std::string Authentication::GetNowGMT() {
  string dateTimeStr = "";
  date today = day_clock::local_day();
  dateTimeStr += today.day_of_week().as_short_string();
  dateTimeStr += ", ";
  string day = to_string(today.day().as_number());
  day = day.length() == 1 ? ("0" + day) : day;
  dateTimeStr += day + " ";
  dateTimeStr += today.month().as_short_string();
  dateTimeStr += " ";

  string strPosixTime =
      boost::posix_time::to_iso_string(boost::posix_time::second_clock::local_time());
  int pos = strPosixTime.find('T');
  dateTimeStr += strPosixTime.substr(0, pos - 4);
  dateTimeStr += " ";
  string timeStr = strPosixTime.substr(pos + 1);
  timeStr.insert(2, ":");
  timeStr.insert(5, ":");
  dateTimeStr += timeStr + " GMT";
  return dateTimeStr;
}

string Authentication::GetSignature(const std::string& method, const std::string& uri,
                                    const std::string& bodySignature, const std::string nowUtc) {
  unsigned char out[EVP_MAX_MD_SIZE];
  string requestLine = method + " " + uri + " HTTP/1.1";
  string content = "date: " + nowUtc + "\n" + requestLine;
  string headers = "date request-line";
  if (!bodySignature.empty()) {
    content += "\n";
    content += "digest: " + bodySignature;
    headers += " digest";
  }
  int len = HMAC256(content, m_Key, out);
  string signature = Base64Encode(out, len);
  return "hmac username=\"" + m_UsrName + "\", algorithm=\"hmac-sha256\", headers=\"" + headers +
         "\", signature=\"" + signature + "\"";
}
