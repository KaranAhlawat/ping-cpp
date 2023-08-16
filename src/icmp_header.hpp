#pragma once
// ICMP header for both IPv4 and IPv6.
//
// The wire format of an ICMP header is:
//
// 0               8               16                             31
// +---------------+---------------+------------------------------+      ---
// |               |               |                              |       ^
// |     type      |     code      |          checksum            |       |
// |               |               |                              |       |
// +---------------+---------------+------------------------------+    8 bytes
// |                               |                              |       |
// |          identifier           |       sequence number        |       |
// |                               |                              |       v
// +-------------------------------+------------------------------+      ---

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <istream>

using u8 = std::uint8_t;
using u16 = std::uint16_t;

class IcmpHeader
{
public:
  enum class MessageType
  {
    ECHO_REQUEST = 8,
    ECHO_REPLY = 0
  };

  IcmpHeader () { std::fill (rep_, rep_ + sizeof (rep_), 0); }
  IcmpHeader (IcmpHeader&) = delete;
  IcmpHeader (IcmpHeader&&) = delete;

  u8
  type () const
  {
    return rep_[0];
  }
  u8
  code () const
  {
    return rep_[1];
  }
  u16
  checksum () const
  {
    return decode (2, 3);
  }
  u16
  identifier () const
  {
    return decode (4, 5);
  }
  u16
  sequence_number () const
  {
    return decode (6, 7);
  }

  void
  type (MessageType msg)
  {
    rep_[0] = static_cast<uint8_t> (msg);
  }
  void
  code (u8 n)
  {
    rep_[1] = n;
  }
  void
  checksum (u16 n)
  {
    encode (2, 3, n);
  }
  void
  identifier (u16 n)
  {
    encode (4, 5, n);
  }
  void
  sequence_number (u16 n)
  {
    encode (6, 7, n);
  }

  void
  compute_checksum ()
  {
    // Zero out the checksum
    u16 sum = 0;
    sum += (static_cast<uint8_t> (type ()) << 8) + code () + checksum ()
           + identifier () + sequence_number ();

    // Take care of any carries First operation we get any carries
    // from the lower 8 bits Second operation we get the lower 8 bits
    // and add to the upper 8 bits
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    checksum (~sum);
  }

  friend std::istream&
  operator>> (std::istream& is, IcmpHeader& header)
  {
    is.read (reinterpret_cast<char*> (header.rep_), 8);
    return is;
  }

  friend std::ostream&
  operator<< (std::ostream& os, const IcmpHeader& header)
  {
    return os.write (reinterpret_cast<const char*> (header.rep_), 8);
  }

private:
  u16
  decode (int a, int b) const
  {
    return (rep_[a] << 8) + rep_[b];
  }

  void
  encode (int a, int b, u16 n)
  {
    rep_[a] = static_cast<u8> (n >> 8);
    rep_[b] = static_cast<u8> (n & 0xFF);
  }

  u8 rep_[8];
};
