#pragma once
#include <rics_data_service/RicsDefines.h>
namespace rics {
namespace data_collect {
class IDataCollectRepository {
 public:
  virtual void Init() = 0;
  /**
   * @brief 入队列
   * @param
   * @return
   */
  virtual bool Enqueue(MqttSimple& pMessage) = 0;

  /**
   * @brief 取当前数据
   * @param
   * @return
   */
  virtual void Peek(MqttSimple& pMessage) = 0;

  /**
   * @brief 出队操作
   * @param
   * @return
   */
  virtual void Pop(bool popAll = false) = 0;

  /**
   * @brief 队列是否有数据
   * @param
   * @return
   */
  virtual bool HasData() = 0;

  /**
   * @brief 检测队列是否满
   * @param
   * @return
   */
  virtual bool Full() = 0;
};

}  // namespace data_collect
}  // namespace rics