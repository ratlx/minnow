#include <iostream>
#include <vector>

#include "arp_message.hh"
#include "debug.hh"
#include "ethernet_frame.hh"
#include "ethernet_header.hh"
#include "exception.hh"
#include "helpers.hh"
#include "ipv4_datagram.hh"
#include "network_interface.hh"

using namespace std;

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface( string_view name,
                                    shared_ptr<OutputPort> port,
                                    const EthernetAddress& ethernet_address,
                                    const Address& ip_address )
  : name_( name )
  , port_( notnull( "OutputPort", move( port ) ) )
  , ethernet_address_( ethernet_address )
  , ip_address_( ip_address )
{
  cerr << "DEBUG: Network interface has Ethernet address " << to_string( ethernet_address_ ) << " and IP address "
       << ip_address.ip() << "\n";
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but
//! may also be another host if directly connected to the same network as the destination) Note: the Address type
//! can be converted to a uint32_t (raw 32-bit IP address) by using the Address::ipv4_numeric() method.
void NetworkInterface::send_datagram( const InternetDatagram& dgram, const Address& next_hop )
{
  auto ip_num = next_hop.ipv4_numeric();
  if ( ip2ethernet.contains( ip_num ) ) {
    EthernetHeader eaddr { ip2ethernet[ip_num].addr, ethernet_address_, EthernetHeader::TYPE_IPv4 };
    EthernetFrame frame { eaddr, serialize( dgram ) };
    transmit( frame );
  } else {
    // queue the IP datagram so it can be sent after the ARP reply is received
    datagram2send.emplace_back( dgram, ip_num );

    // broadcast an ARP request for the next hop’s Ethernet address
    if ( ARP_requests.contains( ip_num ) ) {
      return;
    }

    ARP_requests[ip_num] = 0;
    EthernetHeader eaddr { ETHERNET_BROADCAST, ethernet_address_, EthernetHeader::TYPE_ARP };
    ARPMessage arp;
    arp.opcode = ARPMessage::OPCODE_REQUEST;
    arp.sender_ip_address = ip_address_.ipv4_numeric();
    arp.sender_ethernet_address = ethernet_address_;
    arp.target_ip_address = ip_num;
    EthernetFrame frame { eaddr, serialize( arp ) };
    transmit( frame );
  }
}

//! \param[in] frame the incoming Ethernet frame
void NetworkInterface::recv_frame( EthernetFrame frame )
{
  if ( frame.header.dst != ETHERNET_BROADCAST && frame.header.dst != ethernet_address_ ) {
    return;
  }

  if ( frame.header.type == EthernetHeader::TYPE_IPv4 ) {
    IPv4Datagram dgram;
    auto s = parse( dgram, frame.payload );
    if ( !s ) {
      return;
    }

    datagrams_received_.push( std::move( dgram ) );
  } else if ( frame.header.type == EthernetHeader::TYPE_ARP ) {
    ARPMessage arp;
    auto s = parse( arp, frame.payload );
    if ( !s ) {
      return;
    }

    auto s_ip = arp.sender_ip_address;
    auto s_eth = arp.sender_ethernet_address;

    ip2ethernet[s_ip] = { s_eth, 0 };
    ARP_requests.erase( s_ip );

    for ( auto& t : datagram2send ) {
      if ( t.ip_num == s_ip ) {
        EthernetHeader eaddr { s_eth, ethernet_address_, EthernetHeader::TYPE_IPv4 };
        EthernetFrame frame2 { eaddr, serialize( t.dgram ) };
        transmit( frame2 );
      }
    }
    erase_if( datagram2send, [&]( const auto& t ) { return t.ip_num == s_ip; } );

    if ( arp.opcode == ARPMessage::OPCODE_REQUEST ) {
      if ( arp.target_ip_address == ip_address_.ipv4_numeric() ) {
        EthernetHeader eaddr { s_eth, ethernet_address_, EthernetHeader::TYPE_ARP };
        ARPMessage reply;
        reply.opcode = ARPMessage::OPCODE_REPLY;
        reply.sender_ip_address = ip_address_.ipv4_numeric();
        reply.sender_ethernet_address = ethernet_address_;
        reply.target_ip_address = s_ip;
        reply.target_ethernet_address = s_eth;
        EthernetFrame frame2 { eaddr, serialize( reply ) };
        transmit( frame2 );
      }
    }
  }
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick( const size_t ms_since_last_tick )
{
  for ( auto& [i, j] : ip2ethernet ) {
    j.tick += ms_since_last_tick;
  }

  for ( auto& [i, j] : ARP_requests ) {
    j += ms_since_last_tick;
  }

  erase_if( ip2ethernet, []( const auto& t ) {
    // the mapping between the sender’s IP address and Ethernet address for 30 seconds
    return t.second.tick >= 30000;
  } );

  erase_if( ARP_requests, []( const auto& t ) {
    // If the network interface already sent an ARP request about the same IP address in the last
    // five seconds, don’t send a second request—just wait for a reply to the first one
    return t.second >= 5000;
  } );

  erase_if( datagram2send, [&]( const auto& t ) { return !ARP_requests.contains( t.ip_num ); } );
}
