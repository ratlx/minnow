#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <netdb.h>

#include "tcp_config.hh"

class CongestionControl
{
public:
  enum class State
  {
    SLOW_START,
    CONGESTION_AVOIDANCE,
    FAST_RECOVERY
  };

  static constexpr auto MSS = TCPConfig::MAX_PAYLOAD_SIZE;

  CongestionControl() = default;

  uint64_t get_cwnd()
  {
    std::cout << "window size: " << cwnd_ << "\n";
    return static_cast<uint64_t>( cwnd_ );
  }

  void new_ack( int num )
  {
    std::cout << "new ack\n";
    if ( num <= 0 ) {
      return;
    }

    dup_ack_cnt_ = 0;
    if ( state_ == State::SLOW_START ) {
      ss_new_ack( num );
    } else if ( state_ == State::CONGESTION_AVOIDANCE ) {
      ca_new_ack( num );
    } else {
      fr_new_ack( num );
    }
  }

  void timeout()
  {
    std::cout << "timeout\n";
    ssthresh_ = std::max(cwnd_ / 2, static_cast<double>(2 * MSS));
    cwnd_ = MSS;
    dup_ack_cnt_ = 0;
    state_ = State::SLOW_START;
  }

  void duplicate_ack()
  {
    std::cout << "duplicate ack\n";
    if ( ++dup_ack_cnt_ >= 3 && state_ != State::FAST_RECOVERY ) {
      state_ = State::FAST_RECOVERY;
      ssthresh_ = std::max(cwnd_ / 2, static_cast<double>(2 * MSS));
      cwnd_ = ssthresh_ + 3 * MSS;
    } else if (state_ == State::FAST_RECOVERY) {
      cwnd_ += MSS;
    }
  }

protected:
  void ss_new_ack( int num )
  {
    if ( cwnd_ + num * MSS < ssthresh_ ) {
      cwnd_ += num * MSS;
      return;
    }

    int mul = std::ceil( ( ssthresh_ - cwnd_ ) / MSS );
    cwnd_ += mul * MSS;

    state_ = State::CONGESTION_AVOIDANCE;
    if ( num - mul > 0 ) {
      ca_new_ack( num - mul );
    }
  }

  virtual void ca_new_ack( int num ) = 0;

  void fr_new_ack( int num )
  {
    state_ = State::CONGESTION_AVOIDANCE;
    cwnd_ = ssthresh_;
    if ( num > 1 ) {
      ca_new_ack(num - 1);
    }
  }

  double ssthresh_ { TCPConfig::DEFAULT_CAPACITY };
  double cwnd_ { MSS };
  State state_ { State::SLOW_START };
  uint8_t dup_ack_cnt_ { 0 };
};

class Reno : public CongestionControl
{
public:
  Reno() = default;

protected:
  void ca_new_ack( int num ) override { cwnd_ += num * MSS * ( MSS / cwnd_ ); }
};