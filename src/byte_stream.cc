#include "byte_stream.hh"
#include <stdexcept>

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity )
{
  buffer_.reserve( capacity );
}

void Writer::push( string data )
{
  auto len = min( data.size(), capacity_ - buffer_.size() );
  buffer_ += data.substr( 0, len );
  push_count_ += len;
}

void Writer::close()
{
  closed_ = true;
}

bool Writer::is_closed() const
{
  return closed_;
}

uint64_t Writer::available_capacity() const
{
  return capacity_ - buffer_.size();
}

uint64_t Writer::bytes_pushed() const
{
  return push_count_;
}

string_view Reader::peek() const
{
  return buffer_;
}

void Reader::pop( uint64_t len )
{
  if ( len > buffer_.size() ) {
    throw std::runtime_error( "pop out of range" );
  }
  buffer_.erase( 0, len );
  pop_count_ += len;
}

bool Reader::is_finished() const
{
  return closed_ && buffer_.empty();
}

uint64_t Reader::bytes_buffered() const
{
  return buffer_.size(); 
}

uint64_t Reader::bytes_popped() const
{
  return pop_count_;
}
