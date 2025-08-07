#include "wrapping_integers.hh"
#include "debug.hh"

using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  auto n32 = static_cast<uint32_t>( n );
  return Wrap32 { zero_point + n32 };
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  uint64_t offset;
  uint64_t m64 = UINT32_MAX + 1ULL;
  if ( zero_point.raw_value_ > raw_value_ ) {
    offset = m64 + raw_value_ - zero_point.raw_value_;
  } else {
    offset = raw_value_ - zero_point.raw_value_;
  }
  auto h32 = checkpoint & ( UINT64_MAX - UINT32_MAX );
  auto l32 = checkpoint & UINT32_MAX;
  if ( offset > l32 ) {
    if ( h32 < m64 || offset - l32 < l32 + m64 - offset ) {
      return h32 + offset;
    }
    return h32 - m64 + offset;
  } else {
    if ( l32 - offset <= offset + m64 - l32 ) {
      return h32 + offset;
    }
    return h32 + m64 + offset;
  }
}

uint64_t Wrap32::offset( Wrap32 rhs ) const
{
  auto l = static_cast<uint64_t>( raw_value_ );
  auto r = static_cast<uint64_t>( rhs.raw_value_ );
  if ( l < r ) {
    l += UINT32_MAX + 1ULL;
  }
  return l - r;
}