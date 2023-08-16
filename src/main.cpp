#include <asio.hpp>
#include <asio/experimental/awaitable_operators.hpp>
#include <coroutine>
#include <cstdint>
#include <exception>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "icmp_header.hpp"
#include "ipv4_header.hpp"

using namespace asio::experimental::awaitable_operators;
using asio::ip::icmp;
using namespace std::chrono_literals;
namespace chrono = asio::chrono;

template <typename T> using async = asio::awaitable<T>;

struct State
{
  State (icmp::socket sock, std::uint16_t pid, std::uint16_t seq_num,
         std::uint32_t max_pings, icmp::endpoint destination,
         asio::steady_timer& timer)
      : identifier (pid), sequence_number (seq_num), max_pings (max_pings),
        socket (std::move (sock)), destination (destination), timer (timer){};

  ~State ()
  {
    std::cout << "Cleaning up state...\n";
    socket.close ();
  }

  std::uint16_t identifier;
  std::uint16_t sequence_number;
  std::uint32_t max_pings;
  icmp::socket socket;
  icmp::endpoint destination;
  asio::steady_timer& timer;
  chrono::steady_clock::time_point time_sent;
};

using SharedStatePtr = std::shared_ptr<State>;
auto recieve_icmp (SharedStatePtr state) -> async<void>;

auto
send_icmp (SharedStatePtr state) -> async<void>
{
  if (state->sequence_number > state->max_pings)
    {
      co_return;
    }

  IcmpHeader header;
  header.type (IcmpHeader::MessageType::ECHO_REQUEST);
  header.code (0);
  header.identifier (state->identifier);
  header.sequence_number (state->sequence_number++);
  header.compute_checksum ();

  asio::streambuf request_buffer;
  std::ostream os (&request_buffer);
  os << header;

  state->timer.expires_at (chrono::steady_clock::now () + 1s);
  co_await state->timer.async_wait (asio::use_awaitable);

  state->time_sent = chrono::steady_clock::now ();
  co_await state->socket.async_send_to (
      request_buffer.data (), state->destination, asio::use_awaitable);

  co_await recieve_icmp (state);
}

auto
recieve_icmp (SharedStatePtr state) -> async<void>
{
  asio::streambuf reply_buffer;
  reply_buffer.consume (reply_buffer.size ());
  auto len = co_await state->socket.async_receive (
      reply_buffer.prepare (65536), asio::use_awaitable);

  reply_buffer.commit (len);
  std::istream is (&reply_buffer);
  Ipv4Header ipv4_header;
  IcmpHeader icmp_header;
  is >> ipv4_header >> icmp_header;

  if (is
      && icmp_header.type ()
             == static_cast<std::uint8_t> (IcmpHeader::MessageType::ECHO_REPLY)
      && icmp_header.identifier () == state->identifier)
    {
      chrono::steady_clock::time_point now = chrono::steady_clock::now ();
      chrono::steady_clock::duration elapsed = now - state->time_sent;
      std::cout
          << len - ipv4_header.header_length () << " bytes from "
          << ipv4_header.source_addr ()
          << ": icmp_seq=" << icmp_header.sequence_number ()
          << ", ttl=" << +ipv4_header.time_to_live () << ", time="
          << chrono::duration_cast<chrono::milliseconds> (elapsed).count () << " ms"
          << std::endl;
    }

  co_await send_icmp (state);
}

auto
resolve_host_addr (const std::string_view host)
    -> async<std::optional<std::pair<icmp::endpoint, icmp::socket> > >
{
  auto ctx = co_await asio::this_coro::executor;
  icmp::socket icmp_sock{ ctx };
  try
    {
      icmp::resolver query_resolver (ctx);
      auto endpoint_iter = co_await query_resolver.async_resolve (
          icmp::v4 (), host, "", asio::use_awaitable);

      icmp::endpoint destination{ *endpoint_iter };
      std::cout << "Discovered IPv4 address: " << destination.address() << "\n";

      icmp_sock.open (icmp::v4 ());
      co_return std::make_pair (std::move (destination),
                                std::move (icmp_sock));
    }
  catch (std::exception& e)
    {
      std::cout << "Errored out: " << e.what () << std::endl;
      icmp_sock.close ();
      co_return std::nullopt;
    }
}

auto
ping (std::string_view host, asio::steady_timer& timer) -> async<void>
{
  auto end_and_sock = co_await resolve_host_addr (host);
  if (!end_and_sock)
    {
      std::cerr << "Unable to acquire a socket. Exiting...\n";
      co_return;
    }

  std::cout << "Acquired ICMP socket...\n";
  auto& [dest, sock] = end_and_sock.value ();

  auto icmp_packet_seq_start{ 0 };
  auto icmp_max_packets_sent{ 10 };
  auto state{ std::make_shared<State> (
      std::move (sock), getpid (), icmp_packet_seq_start,
      icmp_max_packets_sent, std::move (dest), timer) };

  co_await (send_icmp (state) || recieve_icmp (state));
  std::cout << "Done running.\n";
}

auto
main () -> int
{
  try
    {
      std::cout << "Host: ";

      // Read in the host
      std::string host;
      std::getline (std::cin, host);

      asio::io_context ctx;
      asio::steady_timer timer{ ctx };
      asio::co_spawn (ctx, ping (host, timer), asio::detached);
      ctx.run ();
    }
  catch (std::exception& e)
    {
      std::cerr << "Exception: " << e.what () << "\n";
    }
}
