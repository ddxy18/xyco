#include "event/net/socket.h"

#include <gtest/gtest.h>

TEST(SocketTest, into_c_fd) {
  Socket socket(-1);  // prevent destructor closing the fd

  ASSERT_EQ(socket.into_c_fd(), -1);
}

TEST(SocketTest, new_v4) {
  const char *local_host = "127.0.0.1";
  const auto http_port = 80;

  auto sock_addr = SocketAddr::new_v4(Ipv4Addr(local_host), http_port);
  const auto *raw_addr = static_cast<const sockaddr_in *>(
      static_cast<const void *>(sock_addr.into_c_addr()));

  ASSERT_EQ(raw_addr->sin_addr.s_addr, 0b00000001000000000000000001111111);
  ASSERT_EQ(raw_addr->sin_port, http_port);
}

TEST(SocketTest, new_v6) {
  const char *local_host = "::1";
  const auto http_port = 80;

  auto sock_addr = SocketAddr::new_v6(Ipv6Addr(local_host), http_port);
  const auto *raw_addr = static_cast<const sockaddr_in6 *>(
      static_cast<const void *>(sock_addr.into_c_addr()));

  ASSERT_EQ(raw_addr->sin6_addr.s6_addr[15], 1);
  ASSERT_EQ(raw_addr->sin6_port, http_port);
}

TEST(SocketTest, is_v4) {
  const auto http_port = 80;

  auto sock_addrv4 = SocketAddr::new_v4(Ipv4Addr("127.0.0.1"), http_port);
  ASSERT_EQ(sock_addrv4.is_v4(), true);

  auto sock_addrv6 = SocketAddr::new_v6(Ipv6Addr("::1"), http_port);
  ASSERT_EQ(sock_addrv6.is_v4(), false);
}