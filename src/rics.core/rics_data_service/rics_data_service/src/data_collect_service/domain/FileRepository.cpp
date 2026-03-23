#include <dirent.h>
#include <fcntl.h>
#include <jsoncpp/json/json.h>
#include <rics_data_service/Logging.h>
#include <rics_data_service/RicsUtils.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <iostream>
#include <list>
#include <sstream>
#include <string>
// #include <brclcpp/brclcpp.h>
// #include <brclcpp/time/time.h>
#include <rics_data_service/data_collect_service/domain/FileRepository.h>
#include <rics_data_service/data_collect_service/domain/Typedefine.h>

#include <boost/algorithm/hex.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/filesystem.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/filter/bzip2.hpp>
#include <boost/iostreams/filtering_stream.hpp>

using namespace std;
using namespace rics::data_collect;
using namespace boost::gregorian;
using namespace boost::posix_time;

#define EVENT_SIZE (sizeof(struct inotify_event))
#define EVENT_BUF_LEN (5 * (EVENT_SIZE + 16))

FileRepository::FileRepository(const DataCollectOption &dcOption,
                               const RicsBusinessOption &ricsBusinessoption)
    : m_dcOption(dcOption), m_ricsBusinessOption(ricsBusinessoption) {
  NotifyInit();
  SyncSendbleFilesList();
}

FileRepository::~FileRepository() {
  m_loopNotify = false;
  inotify_rm_watch(m_notifyFd, m_notifyWd);
  if (m_notifyThread.joinable()) {
    m_notifyThread.join();
  }
}

string FileRepository::GetDir(const string &filepath) {
  int pos = filepath.rfind('/');
  return filepath.substr(0, pos);
}

int FileRepository::Compress2File(const string &srcPath, const string &outPath) {
  std::lock_guard<std::mutex> lk(m_CompressMt);
  string dir = GetDir(outPath);
  if (access(dir.c_str(), F_OK) < 0) {
    mkdir(dir.c_str(), S_IRWXU | S_IRWXG | S_IRWXO);
  }
  vector<char> content;
  ifstream file(srcPath.c_str(), std::ios::binary);
  if (!file.is_open()) {
    RICS_WARN("can not open file[%s]!", srcPath.c_str());
    return -1;
  }
  file.seekg(0, std::ios::end);
  size_t size = file.tellg();
  file.seekg(0);

  content.resize(size);
  file.read(&(content.front()), size);
  file.close();

  vector<char> compressed;
  boost::iostreams::filtering_streambuf<boost::iostreams::output> compress_out;
  compress_out.push(boost::iostreams::bzip2_compressor());
  compress_out.push(boost::iostreams::back_inserter(compressed));
  boost::iostreams::copy(boost::make_iterator_range(content), compress_out);

  int writeSize = compressed.size();
  char *errMsg = nullptr;
  int fd = open(outPath.c_str(), O_CREAT | O_SYNC | O_WRONLY, 0664);
  if (fd == -1) {
    errMsg = strerror(errno);
    RICS_ERROR("creat compress file failed.(%s)", errMsg);
    return -1;
  }
  RICS_INFO("compress file fd: %d, size: %d", fd, writeSize);
  long hasWriteSize = write(fd, &(compressed.front()), writeSize);
  close(fd);
  errMsg = strerror(errno);
  RICS_INFO("write to compress file(%d) %s", hasWriteSize, errMsg);
  return hasWriteSize;
}

bool FileRepository::CalcMd5File(const string &filepath, string &outMd5) {
  if (access(filepath.c_str(), F_OK) < 0) {
    RICS_WARN("GenerateFileMd5 error ,filepath = %s ,File is not exist!", filepath.c_str());
    return false;
  }
  outMd5 = RicsUtils::GenerateFileMd5(filepath);
  return true;
}

int FileRepository::GetCurrFileSize(const string &filepath) {
  ifstream file;
  file.open(filepath, std::ios::in);
  struct stat buf;
  if (stat(filepath.c_str(), &buf) == -1) return 0;
  file.seekg(0, ios_base::end);
  int isize = file.tellg();
  file.close();
  return isize;
}

bool FileRepository::FindEarliestFile(const string &dir, string &filename) {
  (void)dir;
  std::lock_guard<std::mutex> lk(m_SendbleFilesMtx);
  if (m_SendbleFiles.size() <= 1) return false;
  auto iter = m_SendbleFiles.begin();
  if (iter == m_SendbleFiles.end()) return false;
  filename = iter->second;
  return true;
}

std::string FileRepository::DelFile(const string &filepath) {
  boost::system::error_code errCode;
  if (boost::filesystem::remove(filepath, errCode)) {
    return std::string();
  } else {
    return std::string(errCode.message());
  }
}

bool FileRepository::HasSendableFile() {
  std::lock_guard<std::mutex> lk(m_SendbleFilesMtx);
  bool bret = (m_SendbleFiles.size() <= 0) ? false : true;
  return bret;
}

string FileRepository::CreateNewFileName(const string &topicName) {
  std::lock_guard<std::mutex> lk(m_NewCompressFileMt);
  ptime timeTemp = second_clock::local_time();
  string dateStr = to_iso_string(timeTemp);
  return dateStr + "-" + topicName + ".bz2";
}

void FileRepository::GetSendbleFiles(const string &dir, vector<rics::OfflineDataInfo> &filesList) {
  (void)dir;
  string filepath;
  rics::OfflineDataInfo offLineDatainfo;
  std::lock_guard<std::mutex> lk(m_SendbleFilesMtx);
  for (auto &iter : m_SendbleFiles) {
    offLineDatainfo.md5 = iter.first;
    filepath = iter.second;
    int ipos = filepath.rfind('/');
    offLineDatainfo.name = filepath.substr(ipos + 1);
    offLineDatainfo.size = GetCurrFileSize(filepath);
    filesList.push_back(offLineDatainfo);
  }
}

void FileRepository::GetCompressibleFiles(const std::string &dir,
                                          std::vector<rics::OfflineDataInfo> &filesList) {
  (void)dir;
  string md5, compressfileName;
  int ipos = 0;
  list<FilenameInfo> tempFileList;

  if (!GetAllFilesFromSpecDir(m_dcOption.cacheFilePath + TempFilePath, tempFileList)) return;

  rics::OfflineDataInfo offLineDatainfo;

  for (auto &i : tempFileList) {
    // 过滤掉迁移文件
    if (i.filePath[i.filePath.length() - 1] == '0') continue;
    if (CalcMd5File(i.filePath, md5)) {
      offLineDatainfo.md5 = md5;
      ipos = i.filePath.rfind('/');
      compressfileName = CreateNewFileName(i.filePath.substr(ipos + 1));
      offLineDatainfo.name = compressfileName;
      // 根据压缩率6%  估算出压缩文件大小
      offLineDatainfo.size = GetCurrFileSize(i.filePath) * 6 / 100;
      filesList.push_back(offLineDatainfo);
    }
    RICS_INFO("compressfileName=%s ", compressfileName.c_str());
  }
}

void FileRepository::SetCompressedTag(const string &md5, const string &filename) {
  std::lock_guard<std::mutex> lk(m_SendbleFilesMtx);
  m_SendbleFiles.insert(make_pair(md5, filename));
}

void FileRepository::GetCompressedTag(string &md5, string &filename) {
  std::lock_guard<std::mutex> lk(m_SendbleFilesMtx);
  if (m_SendbleFiles.size() > 0) {
    auto iter = m_SendbleFiles.begin();
    md5 = (*iter).first;
    filename = (*iter).second;
  }
}

void FileRepository::EraseCompressedTag(const string &compfile) {
  std::lock_guard<std::mutex> lk(m_SendbleFilesMtx);
  if (m_SendbleFiles.size() <= 0 || compfile.empty()) {
    return;
  }

  for (auto &iter : m_SendbleFiles) {
    if (iter.second == compfile) {
      m_SendbleFiles.erase(iter.first);
      break;
    }
  }
}

double FileRepository::Str2Double(const string &strstamp) {
  stringstream ss;
  ss << strstamp;
  double lstamp;
  ss >> lstamp;
  return lstamp;
}

string FileRepository::GetFilenameFromPath(const string &filepath) {
  int pos = filepath.rfind('/');
  return filepath.substr(pos + 1);
}

bool FileRepository::GetAllFilesFromSpecDir(const string &specdir, list<FilenameInfo> &fileList) {
  FilenameInfo fileinfo;
  string filepath;
  boost::filesystem::path dir(specdir);

  if (!boost::filesystem::exists(dir)) {
    RICS_ERROR("error: GetAllFilesFromSpecDir :Dir(%s) not exist  ", specdir.c_str());
    return false;
  }
  boost::filesystem::directory_iterator endIter;
  for (boost::filesystem::directory_iterator iter(dir); iter != endIter; iter++) {
    if (boost::filesystem::is_directory(*iter)) {
    } else {
      filepath = iter->path().string();
      fileinfo.filePath = filepath;
      fileinfo.fileName = GetFilenameFromPath(filepath);
      fileinfo.stamp = Str2Double(fileinfo.fileName);
      fileList.push_back(fileinfo);
    }
  }
  return true;
}

void FileRepository::SyncSendbleFilesList() {
  string md5 = "";
  list<FilenameInfo> fileList;
  if (!GetAllFilesFromSpecDir(m_dcOption.cacheFilePath + CompressedFilePath, fileList)) return;
  fileList.sort(
      [=](const FilenameInfo &m1, const FilenameInfo &m2) { return m1.stamp < m2.stamp; });

  for (auto i : fileList) {
    if (CalcMd5File(i.filePath, md5)) SetCompressedTag(md5, i.filePath);
  }
}

int FileRepository::WriteSome(const string &filepath, const string &note, int wsize) {
  string dir = GetDir(filepath);
  if (access(dir.c_str(), F_OK) < 0) {
    mkdir(dir.c_str(), S_IRWXU | S_IRWXG | S_IRWXO);
  }
  ofstream save;
  save.open(filepath, std::ios::binary | std::ios::app | std::ios::out);
  save.write(note.c_str(), wsize);
  int fsize = save.tellp();
  save.close();
  RICS_INFO("Cache to temp File :%s!", filepath.c_str());
  return fsize;
}

bool FileRepository::PopSortoutDatas(string &topic, string &msg) {
  map<string, string>::iterator iter;
  if (m_SortoutDatas.size() > 0) {
    iter = m_SortoutDatas.begin();
    topic = iter->first;
    msg = iter->second;
    m_SortoutDatas.erase(iter);
    return true;
  }
  return false;
}

void FileRepository::DistributeCache(const string &topic, const string &msg) {
  auto iter = m_SortoutDatas.find(topic);
  if (m_SortoutDatas.end() != iter)
    iter->second = iter->second + msg;
  else
    m_SortoutDatas.insert(pair<string, string>(topic, msg));
}

void FileRepository::SortoutFromCache(const string &topic, const string &message) {
  // if (!IsTopicNeedCache(topic))
  //     return ;
  Json::Reader reader;
  Json::Value rootObj;
  Json::Value del;
  try {
    if (reader.parse(message, rootObj) && rootObj.isObject()) {
      if (!rootObj["MsgID"].isNull()) {
        rootObj.removeMember("MsgID", &del);
      }
      if (!rootObj["DeviceCode"].isNull()) {
        rootObj.removeMember("DeviceCode", &del);
      }
    } else {
      RICS_WARN("It's not a complete message :%s ", message.c_str());
    }
  } catch (const std::exception &) {
    RICS_WARN("It's not a complete message :%s ", message.c_str());
  }

  string msg = rootObj.toStyledString();
  FormatMsg(msg);
  DistributeCache(topic, msg);
}

void FileRepository::WriteTempFile(std::list<std::string> &topicList) {
  int ipos = -1;
  string topic, msg;
  int sortoutDataSize = m_SortoutDatas.size();
  if (sortoutDataSize > 0) {
    for (int i = 0; i < sortoutDataSize; i++) {
      PopSortoutDatas(topic, msg);
      ipos = topic.rfind('/');
      topic = topic.substr(ipos + 1);
      WriteSome(m_dcOption.cacheFilePath + TempFilePath + topic, msg, msg.length());
      topicList.push_back(topic);
    }
  }
}

void FileRepository::CacheProc(const string &compressFilename) {
  string topic, msg;
  int cacheFilesize = -1;
  int ipos = -1;
  // 压缩文件已存在
  if (GetCurrFileSize(m_dcOption.cacheFilePath + CompressedFilePath + compressFilename) > 0) return;

  list<string> topicList;
  WriteTempFile(topicList);
  list<CompressInfo> listCompressInfo;
  CompressInfo compressinfo;
  listCompressInfo.clear();
  if (compressFilename.empty()) {
    for (auto &i : topicList) {
      topic = i;
      cacheFilesize = GetCurrFileSize(m_dcOption.cacheFilePath + TempFilePath + topic);
      if (cacheFilesize < BEFCOMPRESSSIZE) continue;
      compressinfo.cmpFilePath =
          m_dcOption.cacheFilePath + CompressedFilePath + CreateNewFileName(topic);
      compressinfo.tempFilePath = m_dcOption.cacheFilePath + TempFilePath + topic;
      listCompressInfo.push_back(compressinfo);
    }
  } else {
    ipos = compressFilename.find('-');
    topic = compressFilename.substr(ipos + 1, compressFilename.length() - ipos - 5);
    compressinfo.cmpFilePath = m_dcOption.cacheFilePath + CompressedFilePath + compressFilename;
    compressinfo.tempFilePath = m_dcOption.cacheFilePath + TempFilePath + topic;
    listCompressInfo.push_back(compressinfo);
  }
  CompressHandler(listCompressInfo);
  // 释放磁盘空间
  FreeDisk();
}

void FileRepository::CompressHandler(list<CompressInfo> &listCompressInfo) {
  for (auto &i : listCompressInfo) {
    if (Compress2File(i.tempFilePath, i.cmpFilePath) > 0) {
      DelFile(i.tempFilePath);
    }
    RICS_INFO("Cache to compress File :%s ", i.cmpFilePath.c_str());
  }
}

void FileRepository::FreeDisk() {
  string earliestFilePath = "";
  int freeDiskPercent = GetDiskFreeSpace();
  if (freeDiskPercent <= (m_dcOption.diskFreePercent)) {
    if (FindEarliestFile(m_dcOption.cacheFilePath + CompressedFilePath, earliestFilePath)) {
      DelFile(earliestFilePath);
      RICS_WARN("Percentage of the remaining disk space  %d ,delete File= %s ", freeDiskPercent,
                earliestFilePath.c_str());
    }
  }
}

int FileRepository::GetDiskFreeSpace() {
  struct statfs diskInfo;
  string path = m_dcOption.cacheFilePath;
  statfs(path.c_str(), &diskInfo);

  // 每个block里包含的字节数
  unsigned long long blocksize = diskInfo.f_bsize;
  // 总的字节数，f_blocks为block的数目
  unsigned long long totalsize = blocksize * diskInfo.f_blocks;
  // totalsize b, totalsize>>10 kb,totalsize>>20 mb totalsize>>30 gb
  // 剩余空间的大小
  unsigned long long freeDisk = diskInfo.f_bfree * blocksize;
  // 已用空间大小
  // unsigned long long usedDisk = diskInfo.f_bavail * blocksize;

  return (int)(freeDisk * 100.0 / totalsize);
}

void FileRepository::FormatMsg(string &msg) {
  boost::algorithm::erase_all(msg, "\r");
  boost::algorithm::erase_all(msg, "\n");
  boost::algorithm::erase_all(msg, "\t");
  boost::algorithm::erase_all(msg, " ");
  msg += "\n";
}

void FileRepository::NotifyInit() {
  m_loopNotify = true;
  m_notifyFd = inotify_init();
  m_notifyWd =
      inotify_add_watch(m_notifyFd, (m_dcOption.cacheFilePath + CompressedFilePath).c_str(),
                        IN_CLOSE_WRITE | IN_DELETE);
  m_notifyThread = std::thread([this]() {
    while (m_loopNotify) {
      NotifyEvents();
    }
  });
}

int FileRepository::ReadEvents(char *buf) { return read(m_notifyFd, buf, EVENT_BUF_LEN); }

void FileRepository::HandleEvents(struct inotify_event *in) {
  string::size_type sswp, sswpx;
  string filename, md5;
  filename = in->name;
  sswp = filename.find("swp");
  sswpx = filename.find("swpx");
  if (in->mask & IN_CLOSE_WRITE) {
    if (sswp == string::npos && sswpx == string::npos) {
      if (CalcMd5File(m_dcOption.cacheFilePath + CompressedFilePath + filename, md5)) {
        SetCompressedTag(md5, m_dcOption.cacheFilePath + CompressedFilePath + filename);
        RICS_INFO("generate compress file :%s ", filename.c_str());
      }
    }
  } else if (in->mask & IN_DELETE) {
    if (sswp == string::npos && sswpx == string::npos) {
      EraseCompressedTag(m_dcOption.cacheFilePath + CompressedFilePath + filename);
      RICS_INFO("delete compress file :%s ", filename.c_str());
    }
  }
}

void FileRepository::NotifyEvents() {
  char buf[EVENT_BUF_LEN];
  char *p;
  struct inotify_event *event;
  memset(buf, 0, sizeof(buf));
  int cnts = ReadEvents(buf);
  for (p = buf; p < buf + cnts;) {
    event = (struct inotify_event *)p;
    HandleEvents(event);
    p += sizeof(struct inotify_event) + event->len;
  }
}
