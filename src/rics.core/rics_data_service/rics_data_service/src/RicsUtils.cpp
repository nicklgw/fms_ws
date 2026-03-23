#include "rics_data_service/RicsUtils.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <regex>
#include <vector>

using Poco::Net::DNS;
using Poco::Net::HostEntry;
using Poco::Net::IPAddress;

namespace rics {
std::string RicsUtils::GenerateFileMd5(const std::string& fileName) {
  std::ifstream fin(fileName.c_str(), std::ios::in | std::ios::binary);
  if (!fin.is_open()) {
    return "";
  }

  EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
  EVP_DigestInit_ex(mdctx, EVP_md5(), NULL);

  std::array<char, 1024> buffer;
  std::streamsize iReadLen;
  while (fin.read(buffer.data(), buffer.size()).gcount() > 0) {
    iReadLen = fin.gcount();
    EVP_DigestUpdate(mdctx, buffer.data(), iReadLen);
  }
  fin.close();

  unsigned char hash[EVP_MAX_MD_SIZE];
  unsigned int hash_len;
  EVP_DigestFinal_ex(mdctx, hash, &hash_len);
  EVP_MD_CTX_free(mdctx);

  std::stringstream ss;
  for (unsigned int i = 0; i < hash_len; ++i) {
    ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
  }
  return ss.str();
}

std::string RicsUtils::GenerateDataMd5(const std::string& data) {
  EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
  EVP_DigestInit_ex(mdctx, EVP_md5(), NULL);
  EVP_DigestUpdate(mdctx, data.c_str(), data.length());

  unsigned char hash[EVP_MAX_MD_SIZE];
  unsigned int hash_len;
  EVP_DigestFinal_ex(mdctx, hash, &hash_len);
  EVP_MD_CTX_free(mdctx);

  std::stringstream ss;
  for (unsigned int i = 0; i < hash_len; ++i) {
    ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
  }
  return ss.str();
}

std::int64_t RicsUtils::GetTimestamp() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
             std::chrono::system_clock::now().time_since_epoch())
      .count();
}

// std::string RicsUtils::GenerateUuid() {
//   uuid_t uu;
//   uuid_generate(uu);
//   std::string strUuid;
//   for (size_t i = 0; i < sizeof(uu) / sizeof(uu[0]); ++i) {
//     std::ostringstream oss;
//     oss << std::hex << static_cast<int>(uu[i]);
//     std::string strId = oss.str();
//     if (strId.length() < 2) {
//       strId = "0" + strId;
//     }
//     strUuid += strId;
//   }
//   return strUuid;
// }

bool RicsUtils::RecurseCreateDir(const std::string& path, __mode_t mode) {
  auto beginPos = path.find('/');
  if (std::string::npos == beginPos) {
    return false;
  }
  ++beginPos;
  std::string::size_type findPos;
  /**< 逐级创建目录 */
  while ((findPos = path.find('/', beginPos)) != std::string::npos) {
    std::string curPath = path.substr(0, findPos);
    /**< 目录不存在则创建 */
    if (!IsFileOrDirExist(curPath)) {
      if (0 != mkdir(curPath.c_str(), mode)) {
        /**< 打印具体错误原因 */
        RICS_ERROR("mkdir error: %s\n", strerror(errno));
        return false;
      }
    }
    beginPos = findPos + 1;
  }
  /**< 最后一级目录 */
  if (!IsFileOrDirExist(path)) {
    if (0 != mkdir(path.c_str(), mode)) {
      return false;
    }
  }
  return true;
}

bool RicsUtils::IsFileOrDirExist(const std::string& fileOrDir) {
  if (0 != access(fileOrDir.c_str(), F_OK)) {
    return false;
  }
  return true;
}

bool RicsUtils::ChangeMode(const std::string& fileOrDir, __mode_t mode) {
  if (0 != chmod(fileOrDir.c_str(), mode)) {
    return false;
  }
  return true;
}

int RicsUtils::ExecuteCmd(const std::string& cmd) { return system(cmd.c_str()); }

int RicsUtils::ExecuteCmd(const std::string& cmd, std::string& log) {
  FILE* fp = popen(cmd.c_str(), "r");
  if (!fp) {
    return -1;
  }

  log = "";
  std::array<char, 4096> buff = {0};
  while (!feof(fp)) {
    if (fgets(buff.begin(), buff.size(), fp)) {
      log += buff.data();
    }
  }
  return pclose(fp);
}

void RicsUtils::ExecuteCmdAsync(const std::string& cmd,
                                std::function<void(const std::string&)> callBack) {
  std::thread([=]() {
    std::string strLog;
    int iRet = ExecuteCmd(cmd, strLog);
    if (0 != iRet) {
      strLog = "ExecuteCmd fail! ret=" + std::to_string(iRet);
    }
    if (callBack) {
      callBack(strLog);
    }
  }).detach();
}

void RicsUtils::FormatPath(std::string& path) {
  if (path.length() > 0 && path[path.length() - 1] != '/') {
    path += "/";
  }
}

uint32_t RicsUtils::GetFileCount(const std::string& path, const std::string& filePattern) {
  std::string strCmd = "find " + path + " -name " + filePattern + " | wc -l";
  std::string strOut;
  if (0 != ExecuteCmd(strCmd, strOut)) {
    return 0;
  }

  return atoi(strOut.c_str());
}

void RicsUtils::PrintScriptsLog(const std::string& scriptFile,
                                const std::map<std::string, std::string>& values) {
  for (auto& value : values) {
    if (value.first == "debug") {
      RICS_DEBUG("(%s) %s", scriptFile.c_str(), value.second.c_str());
    } else if (value.first == "info") {
      RICS_INFO("(%s) %s", scriptFile.c_str(), value.second.c_str());
    } else if (value.first == "warn") {
      RICS_WARN("(%s) %s", scriptFile.c_str(), value.second.c_str());
    } else if (value.first == "error" || value.first == "critical") {
      RICS_ERROR("(%s) %s", scriptFile.c_str(), value.second.c_str());
    }
  }
}

int64_t RicsUtils::GetBuildTime() {
  std::string strTime = __DATE__;
  strTime += __TIME__;
  struct tm buildTime;
  if (nullptr == strptime(strTime.c_str(), "%b %d %Y%H:%M:%S", &buildTime)) {
    return 0;
  }
  return mktime(&buildTime);
}
#include <iostream>
std::string RicsUtils::HostByName(const std::string& name) {
  try {
    const HostEntry& entry = DNS::hostByName(name);
    const HostEntry::AddressList& addrs = entry.addresses();

    if (!addrs.empty()) {
      return addrs.begin()->toString();
    }
  } catch (const std::exception& e) {
    return "";
  }
  return "";
}

std::string RicsUtils::GetSystemDefaultUser() {
  std::string strUser;
  /**< 优先获取bzlrobot及nvidia用户 */
  if (0 != RicsUtils::ExecuteCmd(
               R"(getent passwd | grep 'bzlrobot\|nvidia' | tail -n 1 | cut -d : -f 1)", strUser)) {
    return "";
  }

  /**< 获取id为1000的用户名 */
  if (strUser.length() <= 1) {
    RicsUtils::ExecuteCmd(R"(getent passwd | grep '1000' | tail -n 1 | cut -d : -f 1)", strUser);
  }

  /**< 去掉最后的换行符 */
  if (strUser.length() > 0) {
    strUser = strUser.substr(0, strUser.length() - 1);
  }

  return strUser;
}

// 获取指定目录下符合条件的所有文件路径
std::vector<fs::path> RicsUtils::GetFiles(const std::string& path, const FileFilter& filter,
                                          bool recursive) {
  std::vector<fs::path> result;
  fs::path dirPath(path);

  // 检查路径是否存在且为目录
  if (!fs::exists(dirPath) || !fs::is_directory(dirPath)) {
    // std::cerr << "错误: 路径不存在或不是目录 - " << path << std::endl;
    RICS_ERROR("错误: 路径不存在或不是目录 - %s", path.c_str());
    return result;
  }

  try {
    if (recursive) {
      // 递归模式
      for (const auto& entry : fs::recursive_directory_iterator(dirPath)) {
        try {
          const fs::path filePath = entry.path();

          // 跳过目录，只处理普通文件
          if (!entry.is_regular_file()) {
            continue;
          }

          // 应用过滤条件
          if (filter(filePath)) {
            result.push_back(filePath);
          }
        } catch (const fs::filesystem_error& e) {
          // std::cerr << "处理文件时出错: " << e.what() << " - " << entry.path() << std::endl;
          RICS_ERROR("处理文件时出错: %s - %s", e.what(), entry.path().c_str());
        }
      }
    } else {
      // 非递归模式
      for (const auto& entry : fs::directory_iterator(dirPath)) {
        try {
          const fs::path filePath = entry.path();

          // 跳过目录，只处理普通文件
          if (!entry.is_regular_file()) {
            continue;
          }

          // 应用过滤条件
          if (filter(filePath)) {
            result.push_back(filePath);
          }
        } catch (const fs::filesystem_error& e) {
          // std::cerr << "处理文件时出错: " << e.what() << " - " << entry.path() << std::endl;
          RICS_ERROR("处理文件时出错: %s - %s", e.what(), entry.path().c_str());
        }
      }
    }
  } catch (const fs::filesystem_error& e) {
    // std::cerr << "访问目录时出错: " << e.what() << " - " << path << std::endl;
    RICS_ERROR("访问目录时出错: %s - %s", e.what(), path.c_str());
  }

  return result;
}

std::vector<std::string> RicsUtils::GetFilesAsStrings(const std::string& path,
                                                      const FileFilter& filter, bool recursive) {
  auto paths = GetFiles(path, filter, recursive);
  std::vector<std::string> result;
  result.reserve(paths.size());
  for (const auto& p : paths) {
    result.push_back(p.string());
  }
  return result;
}

}  // namespace rics