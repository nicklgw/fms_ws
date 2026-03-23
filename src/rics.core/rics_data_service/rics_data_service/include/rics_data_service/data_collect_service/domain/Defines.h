#pragma once
#include <string>
namespace rics
{
    struct UploadFMSOption
    {
        std::string Host;
        int16_t Port;
    };

    struct RegisterFMSOption
    {
        std::string Usrname;
        std::string Key;
    };

    struct DataCollectOption
    {
        /**< FMS 上传配置、FMS登录配置、APP采集配置 */
        std::string cacheFilePath;
        int diskFreePercent;
        UploadFMSOption uploadFmsConfig;
        RegisterFMSOption regFmsConfig;        
    };

    // struct OfflineDataInfo
    // {
    //     std::string name;
    //     std::int64_t size;
    //     std::string md5;
    // };
}