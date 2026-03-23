#pragma once
#include <string>
namespace rics
{
    namespace data_collect
    {
        class IDataReporter
        {
        public:
            virtual void 
            Init() = 0;

           /**
            * @brief 是否允许发送
            * @param 
            * @return
            */           
            virtual bool 
            IsConnected() = 0;

            /**
             * @brief:是否恢复链接  
             * @param:  
             * @return:
             */
            virtual bool 
            IsReConnect() =0;

           /**
            * @brief  发送消息
            * @param 
            * @return
            */           
            virtual bool 
            SendMsg(const std::string&topicName,const  std::string& msg)=0;
                  
            
          
        };

        
    }
}