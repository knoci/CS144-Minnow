#include "byte_stream.hh"

#include <utility>

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ) {}

bool Writer::is_closed() const
{
  return closed_;
}

void Writer::push( string data )
{
  // 状态判断
  if ( Writer::is_closed() or Writer::available_capacity() == 0 or data.empty() ) {
    return;
  }
  // 只能推送可用容量允许的数据量
  if ( data.size() > Writer::available_capacity() ) {
    data.resize( Writer::available_capacity() );
  }
  total_pushed_ += data.size();
  total_buffered_ += data.size();
  // emplace放入，move表示 data 可以被移动而不是被复制
  stream_.emplace( move( data ) );
  return;
}

void Writer::close()
{
  closed_ = true;
}

uint64_t Writer::available_capacity() const
{
  return capacity_ - total_buffered_;
}

uint64_t Writer::bytes_pushed() const
{
  return total_pushed_;
}

bool Reader::is_finished() const
{
  return closed_ and total_buffered_ == 0; // 关闭状态并且缓冲区空
}

uint64_t Reader::bytes_popped() const
{
  return total_popped_;
}

string_view Reader::peek() const
{
  // 判断队列是否为空，是就返回空string_view，不是就构造一个新的string_view，该对象是 stream_.front()
  // 的子视图，并且截取到offset_
  return stream_.empty() ? string_view {} // std::string_view dependents on the initializer through its lifetime.
                         : string_view { stream_.front() }.substr( offset_ );
}

void Reader::pop( uint64_t len )
{
  total_buffered_ -= len;
  total_popped_ += len;
  /* 当 len 小于队列前端元素剩余的大小时（即 len < size），只需更新 offset_
     而不是弹出元素。这避免了在每次迭代中都弹出元素，只有在必要时才进行 pop 操作 stream_.front()
     返回队列中的第一个元素的引用，在 std::string 的情况下，这个大小就是字符串的长度。*/
  while ( len != 0U ) {
    const uint64_t& size { stream_.front().size() - offset_ };
    if ( len < size ) {
      offset_ += len;
      break; // with len = 0;
    }
    stream_.pop();
    offset_ = 0;
    len -= size;
  }
}

uint64_t Reader::bytes_buffered() const
{
  return total_buffered_;
}
