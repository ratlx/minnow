#include <algorithm>
#include <cassert>

#include "debug.hh"
#include "reassembler.hh"

using namespace std;

optional<size_t> Reassembler::get_lb_index( const std::vector<uint64_t>& vec, uint64_t value )
{
  auto it = lower_bound( vec.begin(), vec.end(), value );
  if ( it != vec.end() ) {
    return distance( vec.begin(), it );
  }
  return nullopt;
}

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

  size_t cur_idx;
  // merge front
  if ( auto merge_front_it = get_lb_index( end_index_, first_index ) ) {
    cur_idx = *merge_front_it;

    auto old_start = start_index_[cur_idx];
    // update front
    if ( first_index < old_start && last_index >= old_start ) {
      auto str = std::move( buffer_[old_start] );
      buffer_.erase( old_start );
      str.insert( 0, data.substr( 0, old_start - first_index ) );

      start_index_[cur_idx] = first_index;
      buffer_[first_index] = std::move( str );
    }

    // update back
    auto cur_start = start_index_[cur_idx];
    auto old_end = end_index_[cur_idx];
    if ( last_index > old_end ) {
      end_index_[cur_idx] = last_index;
      buffer_[cur_start] += data.substr( old_end - first_index );
    }

    // the substring is completely front of the cur_idx, we should insert it front
    if ( last_index < old_start ) {
      start_index_.insert( start_index_.begin() + cur_idx, first_index );
      end_index_.insert( end_index_.begin() + cur_idx, last_index );
      buffer_[first_index] = std::move( data );
    }
  } else {
    cur_idx = start_index_.size();

    start_index_.push_back( first_index );
    end_index_.push_back( last_index );
    buffer_[first_index] = data;
  }

  first_index = start_index_[cur_idx];
  last_index = end_index_[cur_idx];

  size_t merge_back_idx;
  // merge back
  if ( auto merge_back_it = get_lb_index( start_index_, last_index ) ) {
    merge_back_idx = start_index_[*merge_back_it] > last_index ? *merge_back_it - 1 : *merge_back_it;
  } else {
    merge_back_idx = start_index_.size() - 1;
  }

  // we should delete substrings from (cur_idx, merge_back_idx), because we've covered them
  if ( cur_idx + 1 < merge_back_idx ) {
    auto from_idx = cur_idx + 1;
    for ( auto i = from_idx; i < merge_back_idx; ++i ) {
      buffer_.erase( start_index_[i] );
    }
    start_index_.erase( start_index_.begin() + from_idx, start_index_.begin() + merge_back_idx );
    end_index_.erase( end_index_.begin() + from_idx, end_index_.begin() + merge_back_idx );
  }

  // now we can merge substrings in cur_idx and cur_idx + 1
  if ( cur_idx < merge_back_idx ) {
    merge_back_idx = cur_idx + 1;
    auto old_start = start_index_[merge_back_idx];
    auto old_end = end_index_[merge_back_idx];

    // merge and update
    if ( old_end > last_index ) {
      buffer_[first_index] += buffer_[old_start].substr( last_index - old_start );
      end_index_[cur_idx] = last_index = old_end;
    }

    // delete
    buffer_.erase( old_start );
    start_index_.erase( start_index_.begin() + merge_back_idx );
    end_index_.erase( end_index_.begin() + merge_back_idx );
  }

  // push to writer
  if ( first_index == first_unassembled_index_ ) {
    first_unassembled_index_ += buffer_[first_index].size();
    output_.writer().push( std::move( buffer_[first_index] ) );
    end_index_[0] = last_index;

    buffer_.erase( first_index );
    start_index_.erase( start_index_.begin() + cur_idx );
    end_index_.erase( end_index_.begin() + cur_idx );

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
