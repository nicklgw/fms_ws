#include <rics_data_service/data_collect_service/domain/ErrorCodeDefine.h>
#include <rics_data_service/data_collect_service/domain/ReportWorker.h>
#include <rics_data_service/data_collect_service/domain/Typedefine.h>

using namespace std;
using namespace rics::data_collect;

ReportWorker::ReportWorker() {}

ReportWorker::~ReportWorker() {}

void ReportWorker::ReportData() { m_dataCollectService->ReportData(); }

void ReportWorker::UploadFile() { m_dataCollectService->UpLoadFile(); }

void ReportWorker::Init() {
  m_objInjection = ObjectInjection::Getinstance();
  m_dataCollectService = boost::any_cast<std::shared_ptr<IDataCollectService>>(
      m_objInjection->GetObject(k_DataCollectService));
}
