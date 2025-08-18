#include "router.hh"
#include "debug.hh"

#include <iostream>

using namespace std;

// route_prefix: The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
// prefix_length: For this route to be applicable, how many high-order (most-significant) bits of
//    the route_prefix will need to match the corresponding bits of the datagram's destination address?
// next_hop: The IP address of the next hop. Will be empty if the network is directly attached to the router (in
//    which case, the next hop address should be the datagram's final destination).
// interface_num: The index of the interface to send the datagram out on.
void Router::add_route( const uint32_t route_prefix,
                        const uint8_t prefix_length,
                        const optional<Address> next_hop,
                        const size_t interface_num )
{
  if ( prefix_length > 32 ) {
    throw invalid_argument( "prefix_length should be LE than 32" );
  }
  if ( prefix_length == 0 ) {
    routing_table_[0] = { interface_num, next_hop };
    return;
  }
  auto shft = 32 - prefix_length;
  uint32_t prefix = route_prefix >> shft;
  routing_table_[prefix] = { interface_num, next_hop };
  shift_set_.insert( shft );
}

// Go through all the interfaces, and route every incoming datagram to its proper outgoing interface.
void Router::route()
{
  for ( auto& i : interfaces_ ) {
    if ( !i ) {
      continue;
    }

    auto& recv = i->datagrams_received();
    while ( !recv.empty() ) {
      auto d = recv.front();
      recv.pop();

      // If the TTL was zero already, or hits zero after the decrement, the router should drop the datagram.
      if ( d.header.ttl <= 1 ) {
        continue;
      }
      --d.header.ttl;
      d.header.compute_checksum();

      if ( auto m = longest_prefix_match( d.header.dst ) ) {
        auto& [j, oaddr] = routing_table_[*m];
        if ( j >= interfaces_.size() || !interfaces_[j] ) {
          continue;
        }

        if ( oaddr ) {
          interfaces_[j]->send_datagram( d, *oaddr );
        } else {
          auto addr = Address::from_ipv4_numeric( d.header.dst );
          interfaces_[j]->send_datagram( d, addr );
        }
      }
    }
  }
}

optional<uint32_t> Router::longest_prefix_match( uint32_t ip_num )
{
  for ( auto s : shift_set_ ) {
    auto pre = ip_num >> s;
    if ( pre == 0 ) {
      break;
    }
    if ( routing_table_.contains( pre ) ) {
      return pre;
    }
  }

  if ( routing_table_.contains( 0 ) ) {
    return 0;
  }
  return nullopt;
}
