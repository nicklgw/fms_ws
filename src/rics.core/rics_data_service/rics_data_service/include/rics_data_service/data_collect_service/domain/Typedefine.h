#pragma once
#include <string>
namespace rics
{
    namespace data_collect
{
        #define QUEUEMAX 30
        #define CYCLEMAX 2
        #define BEFCOMPRESSSIZE 10*1024*1024

        

        const std::string TempFilePath = "uncompress/";
        const std::string CompressedFilePath = "compress/";
        /// <summary>
        /// 监控类型
        /// </summary>
        typedef enum _MonitorType
        {
            m_Disk = 0,
            m_NetWork = 1,
        } MonitorType;

        /// <summary>
        /// 离线操作状态
        /// </summary>
        typedef enum _AsyncOPStu
        {
            s_asyncOPing = 0,
            s_asyncOPed = 1,

        } AsyncOPstu;

        /// <summary>
        /// 
        /// </summary>
        typedef enum _TransProtoType
        {
            t_Mqtt = 0,
            t_Http = 1,

        } TransProtoType;

        /// <summary>
        ///  文件类型
        /// </summary>
        typedef enum _FileType
        {
            t_unCompress =0,
            t_Compress,
        }FileType;

        /// <summary>
        ///  注入对象key
        /// </summary>
        typedef enum _InjectionKey
        {
            k_TransPort =0,    //MQtt TransPort
            k_FileUpload ,     //上传文件
            k_Login,           //登录
            k_FileOp,          //文件操作
            k_DataReport,      // 在线消息上报
            k_CacheOp,         // 内存操作
            k_DataCollectService, //采集服务
            k_ConfigRics,          //配置文件
            k_ConfigDataCollect,  // DataCollect 配置文件
            k_DataReuestHandler,  //APP采集事件处理
            k_GpsInfo,            //GPS 信息
            k_RouterInfo,         //路由器 信息
        } InjectionKey;


        /// <summary>
        /// Http 发送命令类型
        /// </summary>
        typedef enum _HttpCommandType
        {
            C_FileName = 0,
            C_File,
            C_PubKey,
            C_Token
        } HttpCommandType;


        /// <summary>
        /// 请求上传文件名数据
        /// </summary>
        struct GetFileNameRequest
        {
            std::string filename;
        };

        /// <summary>
        ///  上传文件数据
        /// </summary>
        struct PostFileRequest
        {
            std::string token;
        };

        /// <summary>
        ///  上报FMS 的请求回应
        /// </summary>
        struct ApplyofFileRequest
        {
            int64_t code;
            std::string message;
        };

        /// <summary>
        /// 登录获取publickey 请求回应 
        /// </summary>
        struct ApplyofLoginRequest
        {
            int64_t code;
            std::string data[4];
            std::string message;
        };

        /// <summary>
        /// 登录 获取token 请求数据
        /// </summary>
        struct RequestOfTokenData
        {
            std::string publickey;
            std::string uuid;
        };

        /// <summary>
        /// 
        /// </summary>
        typedef enum _UploaderStateMachine
        {
            S_UploadRequest =0,
            S_UploadFile,
        }UploaderStateMachine;

        /// <summary>
        ///  GPS 信息
        /// </summary>
        struct GpsInfo
        {
            bool valid = false;
            int32_t state = 0;      // 状态 1=定位成功 0：未定位
            double lat = 0;         // 纬度 单位：度
            std::string latDirect;  // 纬度N（北纬）或S（南纬）
            double lon = 0;         // 经度 单位：度
            std::string lonDirect;  // 经度E（东经）或W（西经）
        };

        
           
} 

}


