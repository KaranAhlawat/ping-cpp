#pragma once

#include <algorithm>
#include <asio/ip/address_v4.hpp>
#include <cstdint>
#include <iostream>
#include <istream>

using u8 = std::uint8_t;
using u16 = std::uint16_t;

class Ipv4Header
{
public:
  Ipv4Header () { std::fill (rep_, rep_ + sizeof (rep_), 0); }
  Ipv4Header (Ipv4Header&) = delete;
  Ipv4Header (Ipv4Header&&) = delete;

  u8
  version () const
  {
    return (rep_[0] >> 4) & 0xF;
  }
  u16
  header_length () const
  {
    return (rep_[0] & 0xF) * 4;
  }
  u8
  type_of_service () const
  {
    return rep_[1];
  }
  u16
  total_length () const
  {
    return decode (2, 3);
  }
  u16
  identification () const
  {
    return decode (4, 5);
  }
  bool
  dont_fragment () const
  {
    return (rep_[6] & 0x40) != 0;
  }
  bool
  more_fragments () const
  {
    return (rep_[6] & 0x20) != 0;
  }
  u16
  fragment_offset () const
  {
    return decode (6, 7) & 0x1FFF;
  }
  u8
  time_to_live () const
  {
    return rep_[8];
  }
  u8
  protocol () const
  {
    return rep_[9];
  }
  u16
  header_checksum () const
  {
    return decode (10, 11);
  }

  asio::ip::address_v4
  source_addr () const
  {
    return asio::ip::address_v4{ { rep_[12], rep_[13], rep_[14], rep_[15] } };
  }
  asio::ip::address_v4
  destination_addr () const
  {
    return asio::ip::address_v4{ { rep_[16], rep_[17], rep_[18], rep_[19] } };
  }

  friend std::ostream&
  operator<< (std::ostream& os, Ipv4Header& header)
  {
    os << "IPv4 Header: Version=" << +header.version ()
       << ", HeaderLength=" << +header.header_length ()
       << ", TotalLength=" << +header.total_length ()
       << ", TTL=" << +header.time_to_live ()
       << ", Protocol=" << +header.protocol () << "\n";
    return os;
  }

  friend std::istream&
  operator>> (std::istream& is, Ipv4Header& header)
  {
    is.read (reinterpret_cast<char*> (header.rep_), 20);
    if (header.version () != 4)
      is.setstate (std::ios::failbit);
    std::streamsize options_length = header.header_length () - 20;
    if (options_length < 0 || options_length > 40)
      is.setstate (std::ios::failbit);
    else
      is.read (reinterpret_cast<char*> (header.rep_) + 20, options_length);
    return is;
  }

private:
  u16
  decode (int a, int b) const
  {
    return (rep_[a] << 8) + rep_[b];
  }

  u8 rep_[60];
};
