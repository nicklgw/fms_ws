#ifndef RICS_ITRANSPORTCONTEXT_H
#define RICS_ITRANSPORTCONTEXT_H

#include <string>
#include <vector>

namespace rics {
class ITransportContext {
 public:
  virtual bool SetSendCmd(int sendCmd) {
    (void)sendCmd;
    return true;
  }

  // TODO(mwj): 透传用
  virtual bool SetSendTopic(const std::string& strTopic) {
    (void)strTopic;
    return true;
  }
};
}  // namespace rics

#endif  // RICS_ITRANSPORTCONTEXT_H
