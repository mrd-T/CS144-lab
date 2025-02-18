#include "byte_stream.hh"
#include <algorithm>
#include <string_view>

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ) {}

void Writer::push( string data )
{
  if ( Writer::is_closed() || Writer::available_capacity() == 0 || data.empty() ) {
    return;
  }

  if ( data.size() > Writer::available_capacity() ) {
    data.resize( Writer::available_capacity() );
  }
  total_pushed_ += data.size();
  total_buffered_ += data.size();

  stream_.emplace( move( data ) );
}

void Writer::close()
{
  closed_=true;
  // Your code here.
}

bool Writer::is_closed() const
{
  return closed_; // Your code here.
}

uint64_t Writer::available_capacity() const
{
  return capacity_ - total_buffered_;
}

uint64_t Writer::bytes_pushed() const
{
  return total_pushed_;
}

string_view Reader::peek() const
{
  // return {}; // Your code here.
  return stream_.empty() ? string_view{} : string_view{stream_.front()}.substr(removed_prefix_);
}

void Reader::pop( uint64_t len )
{
  total_buffered_ -= len;
  total_popped_ += len;
  // (void)len; // Your code here. 
  while ( !stream_.empty() && len > 0 ) {
    if ( len >= stream_.front().size() - removed_prefix_ ) {
      len -= stream_.front().size() - removed_prefix_;
      removed_prefix_ = 0;
      stream_.pop();
    } else {
      removed_prefix_ += len;
      len = 0;
    }
  }
}

bool Reader::is_finished() const
{
  return closed_ and total_buffered_ == 0;
}

uint64_t Reader::bytes_buffered() const
{
  return total_buffered_;
}

uint64_t Reader::bytes_popped() const
{
    return total_popped_;
}

