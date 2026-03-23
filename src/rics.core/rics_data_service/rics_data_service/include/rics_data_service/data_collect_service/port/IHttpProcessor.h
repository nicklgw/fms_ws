#pragma once
#include<string>
#include <boost/asio.hpp>
#include <boost/type_index.hpp>
#include <boost/any.hpp>
#include <Poco/Net/HTTPRequest.h>
namespace rics
{
    namespace data_collect
    {
        class  IHttpProcessor
        {
        public:
           /**
            * @brief 
            * @param 
            * @return
            */           
            virtual std::string 
            Method() = 0;

           /**
            * @brief 
            * @param 
            * @return
            */           
            virtual std::string 
            URL() = 0;

           /**
            * @brief 
            * @param 
            * @return
            */           
            virtual std::string 
            ContentType() = 0;

           /**
            * @brief 
            * @param 
            * @return
            */           
            virtual std::string 
            Serialize(const boost::any& command, Poco::Net::HTTPRequest& req) = 0;

           /**
            * @brief 
            * @param 
            * @return
            */           
            virtual boost::any 
            Deserialize(const std::string &strdata) = 0;

        };

    }
}