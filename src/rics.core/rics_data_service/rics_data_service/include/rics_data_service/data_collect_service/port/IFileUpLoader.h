#pragma once
#include <functional>
#include <boost/any.hpp>

namespace rics
{
    namespace data_collect
    {
        class IFileUpLoader
        {
        public:
           /**
            * @brief 上传文件
            * @param 
            * @return
            */           
            virtual int 
            UpLoadFile() = 0;

           /**
            * @brief 
            * @param 
            * @return
            */           
            virtual void 
            Init() = 0;
        };
        
        
    }
}