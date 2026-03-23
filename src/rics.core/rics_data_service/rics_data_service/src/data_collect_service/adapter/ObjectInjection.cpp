#include <rics_data_service/data_collect_service/adapter/ObjectInjection.h>

using namespace std;
using namespace rics::data_collect;

shared_ptr<ObjectInjection> ObjectInjection::m_instance = nullptr;

ObjectInjection::~ObjectInjection() {}

shared_ptr<ObjectInjection> ObjectInjection::Getinstance() {
  if (m_instance == NULL) m_instance = make_shared<ObjectInjection>();
  return m_instance;
}

void ObjectInjection::InsertObject(const InjectionKey key, boost::any obj) {
  int ikey = (int)key;
  map<int, const boost::any>::iterator iter = m_injectionObj.find(ikey);
  if (iter != m_injectionObj.end()) {
    m_injectionObj.erase(iter);
  }
  m_injectionObj.insert(pair<int, const boost::any>((int)key, obj));
}

boost::any ObjectInjection::GetObject(const InjectionKey key) {
  int ikey = (int)key;
  map<int, const boost::any>::iterator iter = m_injectionObj.find(ikey);
  if (iter != m_injectionObj.end()) {
    return iter->second;
  }
  return nullptr;
}
