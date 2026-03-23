#ifndef RICS_JSONHELPER_H
#define RICS_JSONHELPER_H

#include <sstream>
#include <memory>
#include <jsoncpp/json/json.h>

namespace json_helper
{
    template<class T> inline void
    GetValue(T& t, const std::string& strKey, const Json::Value& node)
    {
        if (std::is_same<T, int8_t>::value ||
            std::is_same<T, int16_t>::value ||
            std::is_same<T, int32_t>::value)
        {
            if (node[strKey].isInt())
            {
                t = node[strKey].asInt();
                return;
            }
        }
        else if (std::is_same<T, int64_t>::value)
        {
            if (node[strKey].isInt64())
            {
                t = node[strKey].asInt64();
                return;
            }
        }
        else if (std::is_same<T, uint8_t>::value ||
            std::is_same<T, uint16_t>::value ||
            std::is_same<T, uint32_t>::value)
        {
            if (node[strKey].isUInt())
            {
                t = node[strKey].asUInt();
                return;
            }
        }
        else if (std::is_same<T, uint64_t>::value)
        {
            if (node[strKey].isUInt64())
            {
                t = node[strKey].asUInt64();
                return;
            }
        }
        else if (std::is_same<T, float>::value ||
                 std::is_same<T, double>::value)
        {
            if (node[strKey].isDouble())
            {
                t = node[strKey].asDouble();
                return ;
            }
        }
        else if (std::is_same<T, bool>::value)
        {
            if (node[strKey].isBool())
            {
                t = node[strKey].asBool();
                return ;
            }
        }

        std::string strError("cannot parse key: ");
        strError.append(strKey);
        throw std::runtime_error(strError.c_str());
    }

    template<class T> inline void
    GetValue(T& t, const std::string& strKey, const Json::Value& node, const T& defValue)
    {
        try
        {
            return GetValue(t, strKey, node);
        }
        catch (...)
        {
            t = defValue;
        }
    }

    template<> inline void
    GetValue(std::string& strValue,
             const std::string& strKey,
             const Json::Value& node)
    {
        /**< 获取 “key”: "string value"这种形式 */
        if (node[strKey].isString())
        {
            strValue = node[strKey].asString();
            return ;
        }
        /**< 获取 “key”: {...} 或者 [...]这种形式，后面整个object作为字符串输出 */
        else if (node[strKey].isObject() || node[strKey].isArray())
        {
            Json::FastWriter writer;
            strValue = writer.write(node[strKey]);
            return;
        }

        std::string strError("cannot parse key: ");
        strError.append(strKey);
        throw std::runtime_error(strError.c_str());
    }

    template<> inline void
    GetValue(std::string& strValue, const std::string& strKey,
             const Json::Value& node, const std::string& strDefault)
    {
        try
        {
            return GetValue(strValue, strKey, node);
        }
        catch (...)
        {
            strValue = strDefault;
        }

        strValue = strDefault;
    }

    template<> inline void
    GetValue(Json::Value& node,
              const std::string& strKey,
              const Json::Value& root)
    {
        if (root[strKey].isObject())
        {
            node = root[strKey];
            return ;
        }

        std::string strError("cannot parse key: ");
        strError.append(strKey);
        throw std::runtime_error(strError.c_str());
    }

    inline void
    GetArray(Json::Value& node,
             const std::string& strKey,
             const Json::Value& root)
    {
        if (root[strKey].isArray())
        {
            node = root[strKey];
            return ;
        }

        std::string strError("cannot parse key: ");
        strError.append(strKey);
        throw std::runtime_error(strError.c_str());
    }

    inline std::string
    JsonToString(const Json::Value & root)
    {
    	static Json::Value def = []() {
    		Json::Value val;
    		Json::StreamWriterBuilder::setDefaults(&val);
    		/**< 设置emitUTF8 解决json中包含中文被转换成\uxxxx */
    		val["emitUTF8"] = true;
    		return val;
    	}();

    	std::ostringstream stream;
    	Json::StreamWriterBuilder stream_builder;
    	stream_builder.settings_ = def;
    	std::unique_ptr<Json::StreamWriter> writer(stream_builder.newStreamWriter());
    	writer->write(root, &stream);
    	return stream.str();
    }

}

#endif //RICS_JSONHELPER_H
