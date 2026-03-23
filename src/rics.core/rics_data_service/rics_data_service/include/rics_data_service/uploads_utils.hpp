#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <mutex>
#include <string>
#include <vector>

namespace fs = std::filesystem;

/**
 * 默认的从元数据文件中提取原始路径的函数
 * 假设元数据文件的第二行包含原始路径
 */
std::string defaultExtractPathFromMetadata(const std::string& metadataFilePath) {
  if (!fs::exists(metadataFilePath)) return "";

  std::ifstream file(metadataFilePath);
  if (!file.is_open()) return "";

  std::string originalName, originalPath;
  // 读取第一行（原始文件名）
  if (!std::getline(file, originalName)) return "";
  // 读取第二行（原始路径）
  if (!std::getline(file, originalPath)) return "";

  return originalPath;
}

/**
 * 加载或创建元数据文件，返回路径到元数据的映射
 * @param uploadsDir 元数据文件存放的基础目录
 * @param paths 需要处理的路径列表
 * @param isValidMetadataFile 元数据文件有效性验证函数，原型：bool(const std::string&
 * metadataFilePath)
 * @param generateMetadataFilename 生成元数据文件名的函数，原型：std::string(const std::string&
 * path)
 * @param extractOriginalPath 从元数据文件中提取原始路径的函数，原型：std::string(const std::string&
 * metadataFilePath)
 * @param cleanupOrphaned 是否清理无效的元数据文件
 * @return 路径到元数据文件路径的映射
 */
std::map<std::string, std::string> loadMetadataFiles(
    const std::string& uploadsDir, const std::vector<std::string>& paths,
    const std::function<bool(const std::string&)>& isValidMetadataFile,
    const std::function<std::string(const std::string&)>& generateMetadataFilename,
    const std::function<std::string(const std::string&)>& extractOriginalPath =
        defaultExtractPathFromMetadata,
    bool cleanupOrphaned = false, std::ostream& logStream = std::cout,
    std::ostream& errorStream = std::cerr) {
  std::map<std::string, std::string> pathToMetadataMap;
  static std::mutex managerMutex;
  std::lock_guard<std::mutex> lock(managerMutex);

  try {
    // 确保基础目录存在
    if (!fs::exists(uploadsDir)) {
      fs::create_directories(uploadsDir);
    }

    // 1. 处理传入的路径列表，加载或创建元数据文件
    for (const auto& path : paths) {
      // 生成元数据文件名
      std::string metadataFilename = generateMetadataFilename(path);
      if (metadataFilename.empty()) {
        errorStream << "无法为路径生成元数据文件名: " << path << std::endl;
        continue;
      }

      // 构建完整的元数据文件路径
      std::string metadataFilePath = (fs::path(uploadsDir) / metadataFilename).string();

      // 检查或创建元数据文件
      if (!fs::exists(metadataFilePath)) {
        // 创建元数据文件
        std::ofstream metadataFile(metadataFilePath);
        if (metadataFile.is_open()) {
          // 写入原始文件名
          metadataFile << fs::path(path).filename().string() << "\n";
          // 写入完整路径
          metadataFile << path << "\n";
          // 可以添加更多自定义元数据...

          metadataFile.close();
          logStream << "创建元数据文件: " << metadataFilePath << std::endl;
        } else {
          errorStream << "无法创建元数据文件: " << metadataFilePath << std::endl;
          continue;
        }
      }

      // 更新映射
      pathToMetadataMap[path] = metadataFilePath;
    }

    // 2. 清理无效的元数据文件（如果需要）
    if (cleanupOrphaned) {
      // 遍历基础目录中的所有文件
      for (const auto& entry : fs::directory_iterator(uploadsDir)) {
        if (!entry.is_regular_file()) continue;

        std::string metadataFilePath = entry.path().string();

        // 使用用户提供的函数验证元数据文件
        if (isValidMetadataFile(metadataFilePath)) {
          // 进一步检查元数据文件指向的源文件是否存在
          std::string originalPath = extractOriginalPath(metadataFilePath);
          if (originalPath.empty() || !fs::exists(originalPath)) {
            fs::remove(metadataFilePath);
            logStream << "清理无效元数据文件: " << metadataFilePath << std::endl;
          }
        }
      }
    }

  } catch (const std::exception& e) {
    errorStream << "处理元数据文件时出错: " << e.what() << std::endl;
  }

  return pathToMetadataMap;
}