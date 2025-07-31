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
    // compelety out of capacity
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

  // size_t cur_idx;
  auto merge_front_it = end_index_.lower_bound( first_index );
  // merge front
  if ( merge_front_it != end_index_.end() ) {
    auto old_start = *prev( start_index_.upper_bound( *merge_front_it ) );
    auto old_end = *merge_front_it;
    auto cur_start = old_start;
    auto cur_end = old_end;
    // update front
    if ( first_index < old_start && last_index >= old_start ) {
      auto str = std::move( buffer_[old_start] );
      buffer_.erase( old_start );
      str.insert( 0, data.substr( 0, old_start - first_index ) );

      start_index_.erase( old_start );
      start_index_.insert( first_index );
      cur_start = first_index;
      buffer_[first_index] = std::move( str );
    }

    // update back
    if ( last_index > old_end ) {
      end_index_.erase( old_end );
      end_index_.insert( last_index );
      cur_end = last_index;
      buffer_[cur_start] += data.substr( old_end - first_index );
    }

    // the substring is completely front of the cur_idx, we should insert it front
    if ( last_index < old_start ) {
      start_index_.insert( first_index );
      end_index_.insert( last_index );
      buffer_[first_index] = std::move( data );
    } else {
      first_index = cur_start;
      last_index = cur_end;
    }
  } else {
    start_index_.insert( first_index );
    end_index_.insert( last_index );
    buffer_[first_index] = std::move( data );
  }

  // merge back
  // we should delete substrings from (cur_idx, merge_back_idx), because we've covered them
  auto merge_back_it = start_index_.upper_bound( first_index );
  while ( merge_back_it != start_index_.end() && *merge_back_it <= last_index ) {
    auto start = *merge_back_it;
    auto end = start + buffer_[start].size();
    if ( end > last_index ) {
      buffer_[first_index] += buffer_[start].substr( last_index - start );
      end_index_.erase( last_index );
      last_index = end;
    }
    merge_back_it = start_index_.erase( merge_back_it );
    // may overlay
    if ( end != last_index ) {
      end_index_.erase( end );
    }
    buffer_.erase( start );
  }

  // push to writer
  if ( first_index == first_unassembled_index_ ) {
    first_unassembled_index_ += buffer_[first_index].size();
    output_.writer().push( std::move( buffer_[first_index] ) );

    buffer_.erase( first_index );
    start_index_.erase( first_index );
    end_index_.erase( last_index );

    if ( last_byte_inserted_ && buffer_.empty() ) {
      output_.writer().close();
    }
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
