#pragma once
#include <memory>
#include <string>
namespace rics
{
    namespace data_collect
    {
        class Authentication
        {
        public:
            explicit Authentication(std::string key,std::string usrname);
            ~Authentication();

           /**
            * @brief sha256 加密
            * @param  加密字符串， 输出 加密完成内容字节数组
            * @return
            */
            void 
            Sha256(const std::string str, unsigned char* hash);

           /**
            * @brief HMAC256 加密 
            * @param data加密字符串 、加密key  、输出加密完成内容 字节数组
            * @return 加密完成后内容长度
            */
            int 
            HMAC256(std::string data, std::string key, unsigned char* out);

            std::string 
            Base64Encode(const unsigned char* hash, int len, bool newLine = false);

            std::string 
            GetNowGMT();

            std::string 
            GetSignature(const std::string& method, const std::string& uri, 
                         const std::string& bodySignature,
                         const std::string nowUtc);
        private:
            //签名使用
            std::string m_Key,m_UsrName;
        };

       
    }
}  // namespace rics