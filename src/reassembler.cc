#include "reassembler.hh"
#include <iostream>

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  uint64_t last_index = first_index + data.size(); // 计算数据片段的结束索引
  uint64_t first_unacceptable
    = first_unassembled_index_ + output_.writer().available_capacity(); // 计算不可接受数据的起始索引
  // 如果是最后一个数据片段
  if ( is_last_substring ) {
    final_index_ = last_index;
  }
  // 如果数据片段的起始索引在未组装数据的起始索引之前
  if ( first_index < first_unassembled_index_ ) {
    if ( first_index + data.size() <= first_unassembled_index_ ) { // 如果整个数据片段都在未组装数据的起始索引之前
      check_push();                                                // 检查并推送可发送的数据
      return;
    } else {
      data.erase( 0, first_unassembled_index_ - first_index ); // 移除数据片段前面的部分
      first_index = first_unassembled_index_;                  // 更新数据片段的起始索引
    }
  }
  // 如果数据片段的起始索引超出了可接受的范围
  if ( first_unacceptable <= first_index ) {
    return;
  }
  // 如果数据片段的结束索引超出了可接受的范围
  if ( last_index > first_unacceptable ) {
    data.erase( first_unacceptable - first_index ); // 移除数据片段后面的部分
  }
  // 处理segments_容器中的数据，segments_可能存储了之前插入的数据片段
  if ( !segments_.empty() ) {
    auto cur = segments_.lower_bound( Seg( first_index, "" ) ); // 在segments_中查找合适的插入位置
    if ( cur != segments_.begin() ) {
      cur--;
      if ( cur->first_index + cur->data.size() > first_index ) { // 如果前一个元素与当前数据片段重叠
        data.erase( 0, cur->first_index + cur->data.size() - first_index ); // 移除当前数据片段前面的重叠部分
        first_index += cur->first_index + cur->data.size() - first_index; // 更新起始索引
      }
    }
    // 重新查找插入位置
    cur = segments_.lower_bound( Seg( first_index, "" ) );
    while ( cur != segments_.end() && cur->first_index < last_index ) { // 循环处理所有重叠的数据片段
      if ( cur->first_index + cur->data.size() <= last_index ) { // 如果当前segments_中的片段完全在新数据片段内
        bytes_waiting_ -= cur->data.size();                      // 更新等待字节计数
        segments_.erase( cur );                                  // 从segments_中删除重叠的数据片段
        cur = segments_.lower_bound( Seg( first_index, "" ) );
      } else {
        data.erase( cur->first_index - first_index ); // 移除新数据片段前面的重叠部分
        break;
      }
    }
  }

  segments_.insert( Seg( first_index, data ) ); // 将新数据片段插入segments
  bytes_waiting_ += data.size();
  check_push();
}

uint64_t Reassembler::bytes_pending() const
{
  return bytes_waiting_;
}

// 检查并推送数据到输出
void Reassembler::check_push()
{
  while ( !segments_.empty() ) {
    auto seg = segments_.begin(); // 获取segments_的第一个元素
    // 如果第一个元素的索引与未组装的起始索引相同
    if ( seg->first_index == first_unassembled_index_ ) {
      output_.writer().push( seg->data );
      auto tmp = seg;
      first_unassembled_index_ += seg->data.size();
      bytes_waiting_ -= seg->data.size();
      segments_.erase( tmp );
      // 如果已达到最终索引
      if ( first_unassembled_index_ >= final_index_ ) {
        output_.writer().close();
      }
    } else {
      break;
    }
  }
}
