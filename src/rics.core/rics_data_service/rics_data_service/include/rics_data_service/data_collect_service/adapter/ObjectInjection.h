#pragma once
#include <rics_data_service/data_collect_service/domain/Typedefine.h>

#include <boost/any.hpp>
#include <map>
#include <memory>
namespace rics {
namespace data_collect {
class ObjectInjection {
 public:
  ~ObjectInjection();

  static std::shared_ptr<ObjectInjection> Getinstance();

  void InsertObject(const InjectionKey key, const boost::any obj);

  boost::any GetObject(const InjectionKey key);

 private:
  std::map<int, const boost::any> m_injectionObj;

  static std::shared_ptr<ObjectInjection> m_instance;
};

}  // namespace data_collect
}  // namespace rics