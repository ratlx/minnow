#include <algorithm>
#include <cstdint>

#include "debug.hh"
#include "tcp_receiver.hh"

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message )
{
  if ( reassembler_.bs_has_error() ) {
    return;
  }
  if ( message.RST ) {
    reassembler_.bs_set_error();
    return;
  }
  if ( message.SYN ) {
    zero_point_ = message.seqno;
  }
  if ( zero_point_ ) {
    auto checkpoint = reassembler_.first_unassembled_index() + 1;
    auto first_index = message.seqno.unwrap( *zero_point_, checkpoint );
    if ( first_index ) {
      --first_index;
    } else if ( !message.SYN ) {
      // invalid seqno
      return;
    }
    reassembler_.insert( first_index, std::move( message.payload ), message.FIN );
  }
}

TCPReceiverMessage TCPReceiver::send() const
{
  optional<Wrap32> ackno;
  if ( zero_point_ ) {
    auto n = reassembler_.first_unassembled_index() + 1 + reassembler_.writer().is_closed();
    ackno = Wrap32::wrap( n, *zero_point_ );
  }
  auto cap = reassembler_.writer().available_capacity();
  auto window_size = static_cast<uint16_t>( min( cap, static_cast<uint64_t>( UINT16_MAX ) ) );
  return { ackno, window_size, reassembler_.bs_has_error() };
}
