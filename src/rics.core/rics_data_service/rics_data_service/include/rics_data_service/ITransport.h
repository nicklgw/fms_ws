#ifndef RICS_ITRANSPORT_H
#define RICS_ITRANSPORT_H

#include <string>
#include <functional>
#include <memory>
#include <future>

namespace rics
{
    class ITransportContext;

    class ITransport
    {
    public:

        using ReceiveCallback = std::function<void(const std::string&)>;

        virtual bool 
        Send(const std::string& strData) = 0;

        virtual bool 
        Send(const std::string &strData ,int& msgId) = 0;

        virtual bool 
        ConfirmLastData(int msgId) = 0; 

        virtual std::shared_ptr<ITransportContext>
        GetContext() = 0;

        virtual bool
        IsConnected() = 0;

        virtual bool
        Disconnect() { return true; };

    };
}

#endif //RICS_ITRANSPORT_H
