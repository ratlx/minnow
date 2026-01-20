#include "tcp_sender.hh"
#include "debug.hh"
#include "tcp_config.hh"

#include <algorithm>
#include <stdexcept>

using namespace std;

// This function is for testing only; don't add extra state to support it.
uint64_t TCPSender::sequence_numbers_in_flight() const
{
  uint64_t sum = 0;
  for ( auto& seg : segments_ ) {
    sum += seg.sequence_length();
  }
  return sum;
}

// This function is for testing only; don't add extra state to support it.
uint64_t TCPSender::consecutive_retransmissions() const
{
  return retransmissions_;
}

void TCPSender::fast_retransmit( const TransmitFunction& transmit )
{
  if ( segments_.empty() ) {
    throw runtime_error( "nothing can be retransmitted" );
  }
  transmit( segments_[0] );
  ++retransmissions_;
  current_RTO_ms_ = initial_RTO_ms_;
  timer_ = 0;
}

void TCPSender::push( const TransmitFunction& transmit )
{
  if ( input_.has_error() ) {
    transmit( { seqno_, false, "", false, true } );
    return;
  }

  auto sz = max<uint64_t>( window_size_, 1 );
  if ( cc_ ) {
    sz = min( sz, cc_->get_cwnd() );
  }
  auto fill = ackno_ ? seqno_.offset( *ackno_ ) : seqno_.offset( isn_ );
  sz = sz > fill ? sz - fill : 0;
  while ( sz > 0 ) {
    if ( FIN_ ) {
      // reach end
      return;
    }

    bool SYN = seqno_ == isn_;
    sz -= SYN;
    auto len = min( TCPConfig::MAX_PAYLOAD_SIZE, sz );
    len = min( len, input_.reader().bytes_buffered() );
    auto payload = std::string { input_.reader().peek().substr( 0, len ) };
    input_.reader().pop( len );
    sz -= len;
    // FIN push only once
    FIN_ = input_.reader().is_finished() && sz > 0;
    sz -= FIN_;

    TCPSenderMessage message { seqno_, SYN, std::move( payload ), FIN_, false };
    if ( message.sequence_length() == 0 ) {
      return;
    }
    transmit( message );
    segments_.push_back( message );

    seqno_ = seqno_ + message.sequence_length();

    // start timer
    if ( !timer_ ) {
      timer_ = 0;
    }
  }
}

TCPSenderMessage TCPSender::make_empty_message() const
{
  return { seqno_, false, "", false, input_.has_error() };
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  if ( msg.RST ) {
    input_.set_error();
    return;
  }
  auto ackno = msg.ackno;
  if ( ackno == ackno_ || seqno_ == isn_ ) {
    // update new window size
    window_size_ = msg.window_size;
    return;
  }
  if ( !ackno ) {
    return;
  }

  auto it = find_if( segments_.begin(), segments_.end(), [&]( const TCPSenderMessage& x ) {
    return x.seqno + x.sequence_length() == *ackno;
  } );

  if ( it != segments_.end() ) {
    // update congestion contral
    if ( cc_ ) {
      cc_->new_ack( it - segments_.begin() + 1 );
    }
    segments_.erase( segments_.begin(), it + 1 );
    // if it is a new ackno, we update the window size.
    window_size_ = msg.window_size;
    ackno_ = ackno;
    // (a) Set the RTO back to its “initial value.”
    current_RTO_ms_ = initial_RTO_ms_;
    // (b) If the sender has any outstanding data, restart the retransmission timer so that it
    // will expire after RTO milliseconds (for the current value of RTO).
    if ( !segments_.empty() ) {
      timer_ = 0;
    } else {
      // When all outstanding data has been acknowledged, stop the retransmission timer.
      timer_ = nullopt;
    }
    // ( c ) Reset the count of “consecutive retransmissions” back to zero.
    retransmissions_ = 0;
  }
}

void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit )
{
  if ( !timer_ ) {
    return;
  }
  *timer_ += ms_since_last_tick;
  // retransmission timer has expired
  if ( *timer_ >= current_RTO_ms_ ) {
    if ( cc_ ) {
      cc_->timeout();
    }
    // Retransmit the earliest (lowest sequence number) segment
    transmit( segments_[0] );

    if ( window_size_ > 0 ) {
      ++retransmissions_;
      current_RTO_ms_ *= 2;
    }

    // Reset the retransmission timer
    *timer_ = 0;
  }
}
