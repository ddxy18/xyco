#include <gtest/gtest.h>
#include <netinet/in.h>

import xyco.test.utils;
import xyco.net;
import xyco.libc;

TEST(SocketTest, into_c_fd) {
  xyco::net::Socket socket(-1);  // prevent destructor closing the fd

  ASSERT_EQ(socket.into_c_fd(), -1);
}

TEST(SocketTest, new_v4) {
  const char *local_host = "127.0.0.1";
  const auto http_port = 80;

  auto sock_addr = xyco::net::SocketAddr::new_v4(xyco::net::Ipv4Addr(local_host), http_port);
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  const auto *raw_addr = reinterpret_cast<const xyco::libc::sockaddr_in *>(sock_addr.into_c_addr());

  ASSERT_EQ(raw_addr->sin_addr.s_addr, 0b00000001000000000000000001111111);
  ASSERT_EQ(::ntohs(raw_addr->sin_port), http_port);
}

TEST(SocketTest, new_v6) {
  const char *local_host = "::1";
  const auto http_port = 80;

  auto sock_addr = xyco::net::SocketAddr::new_v6(xyco::net::Ipv6Addr(local_host), http_port);
  const auto *raw_addr =
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
      reinterpret_cast<const xyco::libc::sockaddr_in6 *>(sock_addr.into_c_addr());

  ASSERT_EQ(raw_addr->sin6_addr.s6_addr[15], 1);
  ASSERT_EQ(raw_addr->sin6_port, http_port);
}

TEST(SocketTest, is_v4) {
  const auto http_port = 80;

  auto sock_addrv4 = xyco::net::SocketAddr::new_v4(xyco::net::Ipv4Addr("127.0.0.1"), http_port);
  ASSERT_EQ(sock_addrv4.is_v4(), true);

  auto sock_addrv6 = xyco::net::SocketAddr::new_v6(xyco::net::Ipv6Addr("::1"), http_port);
  ASSERT_EQ(sock_addrv6.is_v4(), false);
}