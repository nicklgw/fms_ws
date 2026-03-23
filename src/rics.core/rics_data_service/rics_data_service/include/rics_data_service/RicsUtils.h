#ifndef ___RICS_UTILS_HEADER___
#define ___RICS_UTILS_HEADER___

#include <Poco/Net/DNS.h>
#include <openssl/evp.h>
#include <openssl/md5.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <uuid/uuid.h>

#include <array>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <regex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "rics_data_service/Logging.h"

namespace rics {

namespace fs = std::filesystem;
// 文件过滤函数类型定义
using FileFilter = std::function<bool(const fs::path&)>;

class RicsUtils {
 public:
  /**
   * @brief 构造函数
   * @param
   * @return
   */
  RicsUtils() = default;

  /**
   * @brief 析构函数
   * @param
   * @return
   */
  ~RicsUtils() = default;

  /**
   * @brief 生成文件MD5
   * @param fileName 文件全路径名
   * @return 文件MD5值
   */
  static std::string GenerateFileMd5(const std::string& fileName);

  /**
   * @brief 生成数据MD5
   * @param data 数据
   * @return 数据MD5值
   */
  static std::string GenerateDataMd5(const std::string& data);

  /**
   * @brief 获取当前时间戳
   * @return
   */
  static std::int64_t GetTimestamp();

  /**
   * @brief 生成uuid
   * @param
   * @return uuid
   */
  //   static std::string GenerateUuid();

  /**
   * @brief 递归创建目录
   * @param path 路径
   * @param mode 权限
   * @return 是否成功
   */
  static bool RecurseCreateDir(const std::string& path, __mode_t mode);

  /**
   * @brief 判断文件/目录是否存在
   * @param fileOrDir 文件/目录
   * @return 是否存在
   */
  static bool IsFileOrDirExist(const std::string& fileOrDir);

  /**
   * @brief 更改文件/目录权限
   * @param fileOrDir 文件/目录
   * @return 是否更改成功
   */
  static bool ChangeMode(const std::string& fileOrDir, __mode_t mode);

  /**
   * @brief 执行指令
   * @param cmd 指令内容
   * @return 执行结果
   */
  static int ExecuteCmd(const std::string& cmd);

  /**
   * @brief 执行指令并返回日志
   * @param cmd 指令内容
   * @param log 日志
   * @return 执行结果
   */
  static int ExecuteCmd(const std::string& cmd, std::string& log);

  /**
   * @brief 异步执行指令
   * @param cmd 指令内容
   * @param callBack 执行完成后的结果回调
   * @return
   */
  static void ExecuteCmdAsync(const std::string& cmd,
                              std::function<void(const std::string&)> callBack = nullptr);

  /**
   * @brief 格式化路径名，保证路径最后携带/
   * @param path 待格式化的路径名/格式化后的路径名
   * @return
   */
  static void FormatPath(std::string& path);

  /**
   * @brief 获取指定路径下符合格式的文件数量
   * @param path 路径
   * @param filePattern 文件格式
   * @return
   */
  static uint32_t GetFileCount(const std::string& path, const std::string& filePattern);

  /**
   * @brief 打印日志
   * @param values 需要打印的日志集合
   */
  static void PrintScriptsLog(const std::string& scriptFile,
                              const std::map<std::string, std::string>& values);

  /**
   * @brief 获取程序编译时间戳
   * @param
   * @return 时间戳（秒）
   */
  static int64_t GetBuildTime();

  /**
   * @brief Returns an ip string for the given domain name
   * @param addr the given name
   * @return ip string
   */
  static std::string HostByName(const std::string& name);

  /**
   * @brief 获取系统默认用户名
   * @param
   * @return 用户名
   */
  static std::string GetSystemDefaultUser();

  /**
   * @brief 获取指定目录下符合条件的所有文件路径
   * @param path 目录路径
   * @param filter 过滤函数
   * @param recursive 是否递归搜索
   * @return 文件路径列表
   */
  static std::vector<fs::path> GetFiles(const std::string& path, const FileFilter& filter,
                                        bool recursive = true);

  /**
   * @brief 获取指定目录下符合条件的所有文件路径
   * @param path 目录路径
   * @param filter 过滤函数
   * @param recursive 是否递归搜索
   * @return 文件路径列表
   */
  static std::vector<std::string> GetFilesAsStrings(const std::string& path,
                                                    const FileFilter& filter,
                                                    bool recursive = true);
};

}  // namespace rics

#endif  // ___RICS_UTILS_HEADER___