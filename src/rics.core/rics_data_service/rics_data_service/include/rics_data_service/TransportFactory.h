#ifndef RICS_TRANSPORTFACTORY_H
#define RICS_TRANSPORTFACTORY_H
#include <rics_data_service/ITransport.h>
#include <rics_data_service/RicsDefines.h>

#include "rics_data_service/fms_adapter/MqttContext.h"
#include "rics_data_service/fms_adapter/MqttTransport.h"

namespace rics {
class TransportFactory {
 public:
  static std::shared_ptr<ITransport> CreateTransport(const RicsTransportOption& option) {
    if ("Mqtt" == option.strType) {
      auto pContext = std::make_shared<MqttContext>();

      // TODO logic issue here
      if (13 == option.nProtocol) {
        return std::make_shared<MqttTransport>(option.mqttOptions, pContext);
      } else if (20 == option.nProtocol) {
        return std::make_shared<MqttTransport>(option.mqttOptions, pContext);
      }
    }

    std::string strError = "Cannot create RICSTransport with type: " + option.strType +
                           ", protocol: " + std::to_string(option.nProtocol);
    throw std::runtime_error(strError.c_str());
  }
};
}  // namespace rics

#endif  // RICS_TRANSPORTFACTORY_H
