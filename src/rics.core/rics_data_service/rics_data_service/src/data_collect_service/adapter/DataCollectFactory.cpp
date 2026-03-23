#include <unistd.h>

#include <iostream>
#include <string>
// #include <transport/TransportFactory.h>
#include <rics_data_service/data_collect_service/adapter/DataCollectFactory.h>
#include <rics_data_service/data_collect_service/adapter/DataReporter.h>
#include <rics_data_service/data_collect_service/adapter/ObjectInjection.h>
#include <rics_data_service/data_collect_service/adapter/ThreadPool.h>
#include <rics_data_service/data_collect_service/domain/DataCollectRepository.h>
#include <rics_data_service/data_collect_service/domain/DataCollectService.h>
#include <rics_data_service/data_collect_service/domain/FileRepository.h>
#include <rics_data_service/data_collect_service/domain/FileUploader.h>
#include <rics_data_service/data_collect_service/domain/ReportWorker.h>
#include <rics_data_service/data_collect_service/domain/Typedefine.h>

using namespace std;
using namespace rics;
using namespace rics::data_collect;
// using namespace boost::gregorian;
DataCollectFactory::DataCollectFactory() {}

DataCollectFactory::~DataCollectFactory() {}

std::shared_ptr<IDataCollectService> DataCollectFactory::GetDataCollectService() {
  // 依赖注入（从ObjectInjection获取IDataCollectService）
  auto objInjection = ObjectInjection::Getinstance();
  auto obj = objInjection->GetObject(k_DataCollectService);
  if (obj.empty()) {
    throw std::runtime_error("IDataCollectService not found in ObjectInjection");
  }
  auto service = boost::any_cast<std::shared_ptr<IDataCollectService>>(obj);
  return service;
}

void DataCollectFactory::CreateDataCollectObj(const std::shared_ptr<ITransport>& pTransport,
                                              const DataCollectOption& dcoptipn,
                                              const RicsBusinessOption& ricsBusinessOption) {
  // 注入器
  auto pinjector = ObjectInjection::Getinstance();

  shared_ptr<IDataCollectService> pdataCollectService = make_shared<DataCollectService>();

  shared_ptr<IDataReporter> pdataReporter = make_shared<DataReporter>();

  shared_ptr<IFileUpLoader> pfileUploader = make_shared<FileUploader>(dcoptipn);

  shared_ptr<IDataCollectRepository> pdataCollectRepository = make_shared<DataCollectRepository>();

  shared_ptr<IFileRepository> pfileRepository =
      make_shared<FileRepository>(dcoptipn, ricsBusinessOption);

  m_pdataEventConsumer = make_shared<DataEventConsumer>();

  pinjector->InsertObject(k_ConfigDataCollect, dcoptipn);
  pinjector->InsertObject(k_ConfigRics, ricsBusinessOption);

  pinjector->InsertObject(k_TransPort, pTransport);
  pinjector->InsertObject(k_DataReport, pdataReporter);
  pinjector->InsertObject(k_FileUpload, pfileUploader);
  pinjector->InsertObject(k_CacheOp, pdataCollectRepository);
  pinjector->InsertObject(k_FileOp, pfileRepository);
  pinjector->InsertObject(k_DataCollectService, pdataCollectService);
  auto preportWorker = make_shared<ReportWorker>();

  pfileUploader->Init();
  pdataCollectService->Init();
  pdataReporter->Init();
  pdataCollectRepository->Init();
  preportWorker->Init();
  m_pdataEventConsumer->Init();

  m_pthreadPool = make_shared<ThreadPool>();

  m_pthreadPool->AddTask(bind([preportWorker]() {
                           preportWorker->ReportData();
                           preportWorker->UploadFile();
                         }),
                         100000000, true);
}
