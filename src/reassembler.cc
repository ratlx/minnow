#include <cassert>

#include "debug.hh"
#include "reassembler.hh"

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  // debug( "unimplemented insert({}, {}, {}) called", first_index, data, is_last_substring );
  if ( writer().is_closed() || writer().available_capacity() <= 0 ) {
    return;
  }

  if ( first_index < first_unassembled_index_ ) {
    // if already contained, return
    if ( first_index + data.size() <= first_unassembled_index_ ) {
      return;
    }
    data.erase( 0, first_unassembled_index_ - first_index );
    first_index = first_unassembled_index_;
  }

  auto data_len = data.length();
  auto last_index = first_index + data_len;
  if ( last_index > first_unassembled_index_ + writer().available_capacity() ) {
    last_index = first_unassembled_index_ + writer().available_capacity();
    // compeltely out of capacity
    if ( last_index <= first_index ) {
      return;
    }
    data.erase( last_index - first_index );
    data_len = last_index - first_index;

    // is_last_substring must be false, because we've cut it.
    is_last_substring = false;
  }

  // data will be inserted below
  if ( is_last_substring ) {
    last_byte_inserted_ = true;
  }

  auto merge_back_it = buffer_.upper_bound( first_index );
  // merge front
  if ( merge_back_it != buffer_.begin() ) {
    auto merge_front_it = prev( merge_back_it );
    auto start = merge_front_it->first;
    auto end = start + buffer_[start].size();
    auto& str = merge_front_it->second;

    // update back
    if ( end >= first_index ) {
      if (last_index > end) {
        str.erase( first_index - start );
        data.insert( 0, str );
        buffer_.erase( merge_front_it );
        first_index = start;
      } else {
        return;
      }
    }
  }

  // merge back
  // we should delete substrings from (cur_idx, merge_back_idx), because we've covered them
  while ( merge_back_it != buffer_.end() && merge_back_it->first <= last_index ) {
    auto start = merge_back_it->first;
    auto& str = merge_back_it->second;
    auto end = start + str.size();
    if ( end > last_index ) {
      str.erase( 0, last_index - start );
      data += str;
      last_index = end;
    }
    merge_back_it = buffer_.erase( merge_back_it );
  }

  // push to writer
  if ( first_index == first_unassembled_index_ ) {
    first_unassembled_index_ += data.size();
    output_.writer().push( std::move( data ) );

    if ( last_byte_inserted_ && buffer_.empty() ) {
      output_.writer().close();
    }
  } else {
    buffer_[first_index] = std::move( data );
  }
}

// How many bytes are stored in the Reassembler itself?
// This function is for testing only; don't add extra state to support it.
uint64_t Reassembler::count_bytes_pending() const
{
  // debug( "unimplemented count_bytes_pending() called" );
  uint64_t sum = 0;
  for ( const auto& [idx, str] : buffer_ ) {
    sum += str.size();
  }
  return sum;
}
