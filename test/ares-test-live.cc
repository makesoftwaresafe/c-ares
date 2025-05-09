/* MIT License
 *
 * Copyright (c) The c-ares project and its contributors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * SPDX-License-Identifier: MIT
 */
// This file includes tests that attempt to do real lookups
// of DNS names using the local machine's live infrastructure.
// As a result, we don't check the results very closely, to allow
// for varying local configurations.

#include "ares-test.h"

#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif

namespace ares {
namespace test {

// Use the address of CloudFlare's public DNS servers as example addresses that are
// likely to be accessible everywhere/everywhen.  We used to use google but they
// stopped returning reverse dns answers in Dec 2023
unsigned char cflare_addr4[4]  = { 0x01, 0x01, 0x01, 0x01 };
unsigned char cflare_addr6[16] = {
  0x26, 0x06, 0x47, 0x00, 0x47, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x11
};

MATCHER_P(IncludesAtLeastNumAddresses, n, "") {
  if(!arg)
    return false;
  int cnt = 0;
  for (const ares_addrinfo_node* ai = arg->nodes; ai != NULL; ai = ai->ai_next)
    cnt++;
  return cnt >= n;
}

MATCHER_P(OnlyIncludesAddrType, addrtype, "") {
  if(!arg)
    return false;
  for (const ares_addrinfo_node* ai = arg->nodes; ai != NULL; ai = ai->ai_next)
    if (ai->ai_family != addrtype)
      return false;
  return true;
}

MATCHER_P(IncludesAddrType, addrtype, "") {
  if(!arg)
    return false;
  for (const ares_addrinfo_node* ai = arg->nodes; ai != NULL; ai = ai->ai_next)
    if (ai->ai_family == addrtype)
      return true;
  return false;
}

//VIRT_NONVIRT_TEST_F(DefaultChannelTest, LiveGetAddrInfoV4) {
  //struct ares_addrinfo_hints hints = {0, 0, 0, 0};
  //hints.ai_family = AF_INET;
  //AddrInfoResult result;
  //ares_getaddrinfo(channel_, "www.google.com.", NULL, &hints, AddrInfoCallback, &result);
  //Process();
  //EXPECT_TRUE(result.done_);
  //EXPECT_EQ(ARES_SUCCESS, result.status_);
  //EXPECT_THAT(result.ai_, IncludesAtLeastNumAddresses(1));
  //EXPECT_THAT(result.ai_, OnlyIncludesAddrType(AF_INET));
//}

//VIRT_NONVIRT_TEST_F(DefaultChannelTest, LiveGetAddrInfoV6) {
  //struct ares_addrinfo_hints hints = {0, 0, 0, 0};
  //hints.ai_family = AF_INET6;
  //AddrInfoResult result;
  //ares_getaddrinfo(channel_, "www.google.com.", NULL, &hints, AddrInfoCallback, &result);
  //Process();
  //EXPECT_TRUE(result.done_);
  //EXPECT_EQ(ARES_SUCCESS, result.status_);
  //EXPECT_THAT(result.ai_, IncludesAtLeastNumAddresses(1));
  //EXPECT_THAT(result.ai_, OnlyIncludesAddrType(AF_INET6));
//}

//VIRT_NONVIRT_TEST_F(DefaultChannelTest, LiveGetAddrInfoUnspec) {
  //struct ares_addrinfo_hints hints = {0, 0, 0, 0};
  //hints.ai_family = AF_UNSPEC;
  //AddrInfoResult result;
  //ares_getaddrinfo(channel_, "www.google.com.", NULL, &hints, AddrInfoCallback, &result);
  //Process();
  //EXPECT_TRUE(result.done_);
  //EXPECT_EQ(ARES_SUCCESS, result.status_);
  //EXPECT_THAT(result.ai_, IncludesAtLeastNumAddresses(2));
  //EXPECT_THAT(result.ai_, IncludesAddrType(AF_INET6));
  //EXPECT_THAT(result.ai_, IncludesAddrType(AF_INET));
//}

VIRT_NONVIRT_TEST_F(DefaultChannelTest, LiveGetHostByNameV4) {
  HostResult result;
  ares_gethostbyname(channel_, "www.google.com.", AF_INET, HostCallback, &result);
  Process();
  EXPECT_TRUE(result.done_);
  EXPECT_EQ(ARES_SUCCESS, result.status_);
  EXPECT_LT(0, (int)result.host_.addrs_.size());
  EXPECT_EQ(AF_INET, result.host_.addrtype_);
}

VIRT_NONVIRT_TEST_F(DefaultChannelTest, LiveGetHostByNameV6) {
  HostResult result;
  ares_gethostbyname(channel_, "www.google.com.", AF_INET6, HostCallback, &result);
  Process();
  EXPECT_TRUE(result.done_);
  EXPECT_EQ(ARES_SUCCESS, result.status_);
  EXPECT_LT(0, (int)result.host_.addrs_.size());
  EXPECT_EQ(AF_INET6, result.host_.addrtype_);
}

VIRT_NONVIRT_TEST_F(DefaultChannelTest, LiveGetHostByAddrV4) {
  HostResult result;
  ares_gethostbyaddr(channel_, cflare_addr4, sizeof(cflare_addr4), AF_INET, HostCallback, &result);
  Process();
  EXPECT_TRUE(result.done_);
  EXPECT_EQ(ARES_SUCCESS, result.status_);
  EXPECT_LT(0, (int)result.host_.addrs_.size());
  EXPECT_EQ(AF_INET, result.host_.addrtype_);
}

VIRT_NONVIRT_TEST_F(DefaultChannelTest, LiveGetHostByAddrV6) {
  HostResult result;
  ares_gethostbyaddr(channel_, cflare_addr6, sizeof(cflare_addr6), AF_INET6, HostCallback, &result);
  Process();
  EXPECT_TRUE(result.done_);
  EXPECT_EQ(ARES_SUCCESS, result.status_);
  EXPECT_LT(0, (int)result.host_.addrs_.size());
  EXPECT_EQ(AF_INET6, result.host_.addrtype_);
}

VIRT_NONVIRT_TEST_F(DefaultChannelTest, LiveGetHostByNameFile) {
  struct hostent *host = nullptr;

  // Still need a channel even to query /etc/hosts.
  EXPECT_EQ(ARES_ENOTFOUND,
            ares_gethostbyname_file(nullptr, "localhost", AF_INET, &host));

  int rc = ares_gethostbyname_file(channel_, "bogus.mcname", AF_INET, &host);
  EXPECT_EQ(nullptr, host);
  EXPECT_EQ(ARES_ENOTFOUND, rc);

  rc = ares_gethostbyname_file(channel_, "localhost", AF_INET, &host);
  if (rc == ARES_SUCCESS) {
    EXPECT_NE(nullptr, host);
    ares_free_hostent(host);
  }
}

TEST_P(DefaultChannelModeTest, LiveGetLocalhostByNameV4) {
  HostResult result;
  ares_gethostbyname(channel_, "localhost", AF_INET, HostCallback, &result);
  Process();
  EXPECT_TRUE(result.done_);
  if (result.status_ != ARES_ECONNREFUSED) {
    EXPECT_EQ(ARES_SUCCESS, result.status_);
    EXPECT_EQ(1, (int)result.host_.addrs_.size());
    EXPECT_EQ(AF_INET, result.host_.addrtype_);
    EXPECT_NE(SIZE_MAX, result.host_.name_.find("localhost"));
  }
}

TEST_P(DefaultChannelModeTest, LiveGetLocalhostByNameV6) {
  HostResult result;
  ares_gethostbyname(channel_, "localhost", AF_INET6, HostCallback, &result);
  Process();
  EXPECT_TRUE(result.done_);
  if (result.status_ != ARES_ECONNREFUSED) {
    EXPECT_EQ(ARES_SUCCESS, result.status_);
    EXPECT_EQ(1, (int)result.host_.addrs_.size());
    EXPECT_EQ(AF_INET6, result.host_.addrtype_);
    std::stringstream ss;
    ss << HostEnt(result.host_);
    EXPECT_NE(SIZE_MAX, result.host_.name_.find("localhost"));
  }
}

TEST_P(DefaultChannelModeTest, LiveGetNonExistLocalhostByNameV4) {
  HostResult result;
  ares_gethostbyname(channel_, "idonotexist.localhost", AF_INET, HostCallback, &result);
  Process();
  EXPECT_TRUE(result.done_);
  if (result.status_ != ARES_ECONNREFUSED) {
    EXPECT_EQ(ARES_SUCCESS, result.status_);
    EXPECT_EQ(1, (int)result.host_.addrs_.size());
    EXPECT_EQ(AF_INET, result.host_.addrtype_);
    EXPECT_NE(SIZE_MAX, result.host_.name_.find("idonotexist.localhost"));
  }
}

TEST_P(DefaultChannelModeTest, LiveGetNonExistLocalhostByNameV6) {
  HostResult result;
  ares_gethostbyname(channel_, "idonotexist.localhost", AF_INET6, HostCallback, &result);
  Process();
  EXPECT_TRUE(result.done_);
  if (result.status_ != ARES_ECONNREFUSED) {
    EXPECT_EQ(ARES_SUCCESS, result.status_);
    EXPECT_EQ(1, (int)result.host_.addrs_.size());
    EXPECT_EQ(AF_INET6, result.host_.addrtype_);
    std::stringstream ss;
    ss << HostEnt(result.host_);
    EXPECT_NE(SIZE_MAX, result.host_.name_.find("idonotexist.localhost"));
  }
}

TEST_P(DefaultChannelModeTest, LiveGetLocalhostByNameIPV4) {
  HostResult result;
  ares_gethostbyname(channel_, "127.0.0.1", AF_INET, HostCallback, &result);
  Process();
  EXPECT_TRUE(result.done_);
  EXPECT_EQ(ARES_SUCCESS, result.status_);
  EXPECT_EQ(1, (int)result.host_.addrs_.size());
  EXPECT_EQ(AF_INET, result.host_.addrtype_);
  std::stringstream ss;
  ss << HostEnt(result.host_);
  EXPECT_EQ("{'127.0.0.1' aliases=[] addrs=[127.0.0.1]}", ss.str());
}

TEST_P(DefaultChannelModeTest, LiveGetLocalhostByNameIPV6) {
  HostResult result;
  ares_gethostbyname(channel_, "::1", AF_INET6, HostCallback, &result);
  Process();
  EXPECT_TRUE(result.done_);
  if (result.status_ != ARES_ENOTFOUND) {
    EXPECT_EQ(ARES_SUCCESS, result.status_);
    EXPECT_EQ(1, (int)result.host_.addrs_.size());
    EXPECT_EQ(AF_INET6, result.host_.addrtype_);
    std::stringstream ss;
    ss << HostEnt(result.host_);
    EXPECT_EQ("{'::1' aliases=[] addrs=[0000:0000:0000:0000:0000:0000:0000:0001]}", ss.str());
  }
}

TEST_P(DefaultChannelModeTest, LiveGetLocalhostFailFamily) {
  HostResult result;
  ares_gethostbyname(channel_, "127.0.0.1", AF_INET+AF_INET6, HostCallback, &result);
  Process();
  EXPECT_TRUE(result.done_);
  EXPECT_EQ(ARES_ENOTIMP, result.status_);
}

TEST_P(DefaultChannelModeTest, LiveGetLocalhostByAddrV4) {
  HostResult result;
  struct in_addr addr;
  addr.s_addr = htonl(INADDR_LOOPBACK);
  ares_gethostbyaddr(channel_, &addr, sizeof(addr), AF_INET, HostCallback, &result);
  Process();
  EXPECT_TRUE(result.done_);
  if (result.status_ != ARES_ENOTFOUND) {
    EXPECT_EQ(ARES_SUCCESS, result.status_);
    EXPECT_LT(0, (int)result.host_.addrs_.size());
    EXPECT_EQ(AF_INET, result.host_.addrtype_);
    // oddly, travis does not resolve to localhost, but a random hostname starting with travis-job
    if (result.host_.name_.find("travis-job") == SIZE_MAX) {
        EXPECT_NE(SIZE_MAX,
                  result.host_.name_.find("localhost"));
    }
  }
}

TEST_P(DefaultChannelModeTest, LiveGetLocalhostByAddrV6) {
  HostResult result;
  struct in6_addr addr;
  memset(&addr, 0, sizeof(addr));
  addr.s6_addr[15] = 1;  // in6addr_loopback
  ares_gethostbyaddr(channel_, &addr, sizeof(addr), AF_INET6, HostCallback, &result);
  Process();
  EXPECT_TRUE(result.done_);
  if (result.status_ != ARES_ENOTFOUND) {
    EXPECT_EQ(ARES_SUCCESS, result.status_);
    EXPECT_LT(0, (int)result.host_.addrs_.size());
    EXPECT_EQ(AF_INET6, result.host_.addrtype_);
    const std::string& name = result.host_.name_;
    EXPECT_TRUE(SIZE_MAX != name.find("localhost") ||
                SIZE_MAX != name.find("ip6-loopback"));
  }
}

TEST_P(DefaultChannelModeTest, LiveGetHostByAddrFailFamily) {
  HostResult result;
  unsigned char addr[4] = {8, 8, 8, 8};
  ares_gethostbyaddr(channel_, addr, sizeof(addr), AF_INET6+AF_INET,
                     HostCallback, &result);
  EXPECT_TRUE(result.done_);
  EXPECT_EQ(ARES_ENOTIMP, result.status_);
}

TEST_P(DefaultChannelModeTest, LiveGetHostByAddrFailAddrSize) {
  HostResult result;
  unsigned char addr[4] = {8, 8, 8, 8};
  ares_gethostbyaddr(channel_, addr, sizeof(addr) - 1, AF_INET,
                     HostCallback, &result);
  EXPECT_TRUE(result.done_);
  EXPECT_EQ(ARES_ENOTIMP, result.status_);
}

TEST_P(DefaultChannelModeTest, LiveGetHostByAddrFailAlloc) {
  HostResult result;
  unsigned char addr[4] = {8, 8, 8, 8};
  SetAllocFail(1);
  ares_gethostbyaddr(channel_, addr, sizeof(addr), AF_INET,
                     HostCallback, &result);
  EXPECT_TRUE(result.done_);
  EXPECT_EQ(ARES_ENOMEM, result.status_);
}

INSTANTIATE_TEST_SUITE_P(Modes, DefaultChannelModeTest,
                        ::testing::Values("f", "b", "fb", "bf"));

VIRT_NONVIRT_TEST_F(DefaultChannelTest, LiveSearchA) {
  SearchResult result;
  ares_search(channel_, "www.youtube.com.", C_IN, T_A,
              SearchCallback, &result);
  Process();
  EXPECT_TRUE(result.done_);
  EXPECT_EQ(ARES_SUCCESS, result.status_);
}

VIRT_NONVIRT_TEST_F(DefaultChannelTest, LiveSearchEmptyA) {
  SearchResult result;
  ares_search(channel_, "", C_IN, T_A,
              SearchCallback, &result);
  Process();
  EXPECT_TRUE(result.done_);
  EXPECT_NE(ARES_SUCCESS, result.status_);
}

VIRT_NONVIRT_TEST_F(DefaultChannelTest, LiveSearchNS) {
  SearchResult result;
  ares_search(channel_, "google.com.", C_IN, T_NS,
              SearchCallback, &result);
  Process();
  EXPECT_TRUE(result.done_);
  EXPECT_EQ(ARES_SUCCESS, result.status_);
}

VIRT_NONVIRT_TEST_F(DefaultChannelTest, LiveSearchMX) {
  SearchResult result;
  ares_search(channel_, "google.com.", C_IN, T_MX,
              SearchCallback, &result);
  Process();
  EXPECT_TRUE(result.done_);
  EXPECT_EQ(ARES_SUCCESS, result.status_);
}

VIRT_NONVIRT_TEST_F(DefaultChannelTest, LiveSearchTXT) {
  SearchResult result;
  ares_search(channel_, "google.com.", C_IN, T_TXT,
              SearchCallback, &result);
  Process();
  EXPECT_TRUE(result.done_);
  EXPECT_EQ(ARES_SUCCESS, result.status_);
}

VIRT_NONVIRT_TEST_F(DefaultChannelTest, LiveSearchSOA) {
  SearchResult result;
  ares_search(channel_, "google.com.", C_IN, T_SOA,
              SearchCallback, &result);
  Process();
  EXPECT_TRUE(result.done_);
  EXPECT_EQ(ARES_SUCCESS, result.status_);
}

VIRT_NONVIRT_TEST_F(DefaultChannelTest, LiveSearchSRV) {
  SearchResult result;
  ares_search(channel_, "_imap._tcp.gmail.com.", C_IN, T_SRV,
              SearchCallback, &result);
  Process();
  EXPECT_TRUE(result.done_);
  EXPECT_EQ(ARES_SUCCESS, result.status_);
}

VIRT_NONVIRT_TEST_F(DefaultChannelTest, LiveSearchANY) {
  SearchResult result;
  ares_search(channel_, "google.com.", C_IN, T_ANY,
              SearchCallback, &result);
  Process();
  EXPECT_TRUE(result.done_);
  EXPECT_EQ(ARES_SUCCESS, result.status_);
}

VIRT_NONVIRT_TEST_F(DefaultChannelTest, LiveGetNameInfoV4) {
  NameInfoResult result;
  struct sockaddr_in sockaddr;
  memset(&sockaddr, 0, sizeof(sockaddr));
  sockaddr.sin_family = AF_INET;
  sockaddr.sin_port = htons(53);
  sockaddr.sin_addr.s_addr = htonl(0x08080808);
  ares_getnameinfo(channel_, (const struct sockaddr*)&sockaddr, sizeof(sockaddr),
                   ARES_NI_LOOKUPHOST|ARES_NI_LOOKUPSERVICE|ARES_NI_UDP,
                   NameInfoCallback, &result);
  Process();
  EXPECT_TRUE(result.done_);
  EXPECT_EQ(ARES_SUCCESS, result.status_);
  if (verbose) std::cerr << "8.8.8.8:53 => " << result.node_ << "/" << result.service_ << std::endl;
}

VIRT_NONVIRT_TEST_F(DefaultChannelTest, LiveGetNameInfoV4NoPort) {
  NameInfoResult result;
  struct sockaddr_in sockaddr;
  memset(&sockaddr, 0, sizeof(sockaddr));
  sockaddr.sin_family = AF_INET;
  sockaddr.sin_port = htons(0);
  sockaddr.sin_addr.s_addr = htonl(0x08080808);
  ares_getnameinfo(channel_, (const struct sockaddr*)&sockaddr, sizeof(sockaddr),
                   ARES_NI_LOOKUPHOST|ARES_NI_LOOKUPSERVICE|ARES_NI_UDP,
                   NameInfoCallback, &result);
  Process();
  EXPECT_TRUE(result.done_);
  EXPECT_EQ(ARES_SUCCESS, result.status_);
  if (verbose) std::cerr << "8.8.8.8:0 => " << result.node_ << "/" << result.service_ << std::endl;
}

VIRT_NONVIRT_TEST_F(DefaultChannelTest, LiveGetNameInfoV4UnassignedPort) {
  NameInfoResult result;
  struct sockaddr_in sockaddr;
  memset(&sockaddr, 0, sizeof(sockaddr));
  sockaddr.sin_family = AF_INET;
  sockaddr.sin_port = htons(4);  // Unassigned at IANA
  sockaddr.sin_addr.s_addr = htonl(0x08080808);
  ares_getnameinfo(channel_, (const struct sockaddr*)&sockaddr, sizeof(sockaddr),
                   ARES_NI_LOOKUPHOST|ARES_NI_LOOKUPSERVICE|ARES_NI_UDP,
                   NameInfoCallback, &result);
  Process();
  EXPECT_TRUE(result.done_);
  EXPECT_EQ(ARES_SUCCESS, result.status_);
  if (verbose) std::cerr << "8.8.8.8:4 => " << result.node_ << "/" << result.service_ << std::endl;
}

VIRT_NONVIRT_TEST_F(DefaultChannelTest, LiveGetNameInfoV6Both) {
  NameInfoResult result;
  struct sockaddr_in6 sockaddr;
  memset(&sockaddr, 0, sizeof(sockaddr));
  sockaddr.sin6_family = AF_INET6;
  sockaddr.sin6_port = htons(53);
  memcpy(sockaddr.sin6_addr.s6_addr, cflare_addr6, 16);
  ares_getnameinfo(channel_, (const struct sockaddr*)&sockaddr, sizeof(sockaddr),
                   ARES_NI_TCP|ARES_NI_LOOKUPHOST|ARES_NI_LOOKUPSERVICE|ARES_NI_NOFQDN,
                   NameInfoCallback, &result);
  Process();
  EXPECT_TRUE(result.done_);
  EXPECT_EQ(ARES_SUCCESS, result.status_);
  if (verbose) std::cerr << "[2001:4860:4860::8888]:53 => " << result.node_ << "/" << result.service_ << std::endl;
}

VIRT_NONVIRT_TEST_F(DefaultChannelTest, LiveGetNameInfoV6Neither) {
  NameInfoResult result;
  struct sockaddr_in6 sockaddr;
  memset(&sockaddr, 0, sizeof(sockaddr));
  sockaddr.sin6_family = AF_INET6;
  sockaddr.sin6_port = htons(53);
  memcpy(sockaddr.sin6_addr.s6_addr, cflare_addr6, 16);
  ares_getnameinfo(channel_, (const struct sockaddr*)&sockaddr, sizeof(sockaddr),
                   ARES_NI_TCP|ARES_NI_NOFQDN,  // Neither specified => assume lookup host.
                   NameInfoCallback, &result);
  Process();
  EXPECT_TRUE(result.done_);
  EXPECT_EQ(ARES_SUCCESS, result.status_);
  if (verbose) std::cerr << "[2001:4860:4860::8888]:53 => " << result.node_ << "/" << result.service_ << std::endl;
}

VIRT_NONVIRT_TEST_F(DefaultChannelTest, LiveGetNameInfoV4Numeric) {
  NameInfoResult result;
  struct sockaddr_in sockaddr;
  memset(&sockaddr, 0, sizeof(sockaddr));
  sockaddr.sin_family = AF_INET;
  sockaddr.sin_port = htons(53);
  sockaddr.sin_addr.s_addr = htonl(0x08080808);
  ares_getnameinfo(channel_, (const struct sockaddr*)&sockaddr, sizeof(sockaddr),
                   ARES_NI_LOOKUPHOST|ARES_NI_LOOKUPSERVICE|ARES_NI_TCP|ARES_NI_NUMERICHOST,
                   NameInfoCallback, &result);
  Process();
  EXPECT_TRUE(result.done_);
  EXPECT_EQ(ARES_SUCCESS, result.status_);
  EXPECT_EQ("8.8.8.8", result.node_);
  if (verbose) std::cerr << "8.8.8.8:53 => " << result.node_ << "/" << result.service_ << std::endl;
}

VIRT_NONVIRT_TEST_F(DefaultChannelTest, LiveGetNameInfoV6Numeric) {
  NameInfoResult result;
  struct sockaddr_in6 sockaddr;
  memset(&sockaddr, 0, sizeof(sockaddr));
  sockaddr.sin6_family = AF_INET6;
  sockaddr.sin6_port = htons(53);
  memcpy(sockaddr.sin6_addr.s6_addr, cflare_addr6, 16);
  ares_getnameinfo(channel_, (const struct sockaddr*)&sockaddr, sizeof(sockaddr),
                   ARES_NI_LOOKUPHOST|ARES_NI_LOOKUPSERVICE|ARES_NI_DCCP|ARES_NI_NUMERICHOST,
                   NameInfoCallback, &result);
  Process();
  EXPECT_TRUE(result.done_);
  EXPECT_EQ(ARES_SUCCESS, result.status_);
  EXPECT_EQ("2606:4700:4700::1111%0", result.node_);
  if (verbose) std::cerr << "[2606:4700:4700::1111]:53 => " << result.node_ << "/" << result.service_ << std::endl;
}

VIRT_NONVIRT_TEST_F(DefaultChannelTest, LiveGetNameInfoV6LinkLocal) {
  NameInfoResult result;
  struct sockaddr_in6 sockaddr;
  memset(&sockaddr, 0, sizeof(sockaddr));
  sockaddr.sin6_family = AF_INET6;
  sockaddr.sin6_port = htons(53);
  unsigned char addr6[16] = {0xfe, 0x80, 0x01, 0x02, 0x01, 0x02, 0x00, 0x00,
                             0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x04};
  memcpy(sockaddr.sin6_addr.s6_addr, addr6, 16);
  ares_getnameinfo(channel_, (const struct sockaddr*)&sockaddr, sizeof(sockaddr),
                   ARES_NI_LOOKUPHOST|ARES_NI_LOOKUPSERVICE|ARES_NI_DCCP|ARES_NI_NUMERICHOST,
                   NameInfoCallback, &result);
  Process();
  EXPECT_TRUE(result.done_);
  EXPECT_EQ(ARES_SUCCESS, result.status_);
  EXPECT_EQ("fe80:102:102::304%0", result.node_);
  if (verbose) std::cerr << "[fe80:102:102::304]:53 => " << result.node_ << "/" << result.service_ << std::endl;
}

VIRT_NONVIRT_TEST_F(DefaultChannelTest, LiveGetNameInfoV4NotFound) {
  NameInfoResult result;
  struct sockaddr_in sockaddr;
  memset(&sockaddr, 0, sizeof(sockaddr));
  sockaddr.sin_family = AF_INET;
  sockaddr.sin_port = htons(4);  // Port 4 unassigned at IANA
  // RFC5737 says 192.0.2.0 should not be used publicly.
  sockaddr.sin_addr.s_addr = htonl(0xC0000200);
  ares_getnameinfo(channel_, (const struct sockaddr*)&sockaddr, sizeof(sockaddr),
                   ARES_NI_LOOKUPHOST|ARES_NI_LOOKUPSERVICE|ARES_NI_UDP,
                   NameInfoCallback, &result);
  Process();
  EXPECT_TRUE(result.done_);
  EXPECT_EQ(ARES_SUCCESS, result.status_);
  EXPECT_EQ("192.0.2.0", result.node_);
  if (verbose) std::cerr << "192.0.2.0:53 => " << result.node_ << "/" << result.service_ << std::endl;
}

VIRT_NONVIRT_TEST_F(DefaultChannelTest, LiveGetNameInfoV4NotFoundFail) {
  NameInfoResult result;
  struct sockaddr_in sockaddr;
  memset(&sockaddr, 0, sizeof(sockaddr));
  sockaddr.sin_family = AF_INET;
  sockaddr.sin_port = htons(53);
  // RFC5737 says 192.0.2.0 should not be used publicly.
  sockaddr.sin_addr.s_addr = htonl(0xC0000200);
  ares_getnameinfo(channel_, (const struct sockaddr*)&sockaddr, sizeof(sockaddr),
                   ARES_NI_LOOKUPHOST|ARES_NI_LOOKUPSERVICE|ARES_NI_UDP|ARES_NI_NAMEREQD,
                   NameInfoCallback, &result);
  Process();
  EXPECT_TRUE(result.done_);
  EXPECT_EQ(ARES_ENOTFOUND, result.status_);
}

VIRT_NONVIRT_TEST_F(DefaultChannelTest, LiveGetNameInfoV6NotFound) {
  NameInfoResult result;
  struct sockaddr_in6 sockaddr;
  memset(&sockaddr, 0, sizeof(sockaddr));
  sockaddr.sin6_family = AF_INET6;
  sockaddr.sin6_port = htons(53);
  // 2001:db8::/32 is only supposed to be used in documentation.
  unsigned char addr6[16] = {0x20, 0x01, 0x0d, 0xb8, 0x01, 0x02, 0x00, 0x00,
                             0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x04};
  memcpy(sockaddr.sin6_addr.s6_addr, addr6, 16);
  ares_getnameinfo(channel_, (const struct sockaddr*)&sockaddr, sizeof(sockaddr),
                   ARES_NI_LOOKUPHOST|ARES_NI_LOOKUPSERVICE|ARES_NI_UDP,
                   NameInfoCallback, &result);
  Process();
  EXPECT_TRUE(result.done_);
  EXPECT_EQ(ARES_SUCCESS, result.status_);
  EXPECT_EQ("2001:db8:102::304%0", result.node_);
  if (verbose) std::cerr << "[2001:db8:102::304]:53 => " << result.node_ << "/" << result.service_ << std::endl;
}

VIRT_NONVIRT_TEST_F(DefaultChannelTest, LiveGetNameInvalidFamily) {
  NameInfoResult result;
  struct sockaddr_in6 sockaddr;
  memset(&sockaddr, 0, sizeof(sockaddr));
  sockaddr.sin6_family = AF_INET6 + AF_INET;
  sockaddr.sin6_port = htons(53);
  memcpy(sockaddr.sin6_addr.s6_addr, cflare_addr6, 16);
  ares_getnameinfo(channel_, (const struct sockaddr*)&sockaddr, sizeof(sockaddr),
                   ARES_NI_LOOKUPHOST|ARES_NI_LOOKUPSERVICE|ARES_NI_UDP,
                   NameInfoCallback, &result);
  Process();
  EXPECT_TRUE(result.done_);
  EXPECT_EQ(ARES_ENOTIMP, result.status_);
}

VIRT_NONVIRT_TEST_F(DefaultChannelTest, LiveGetNameInvalidFlags) {
  NameInfoResult result;
  struct sockaddr_in6 sockaddr;
  memset(&sockaddr, 0, sizeof(sockaddr));
  sockaddr.sin6_family = AF_INET6;
  sockaddr.sin6_port = htons(53);
  memcpy(sockaddr.sin6_addr.s6_addr, cflare_addr6, 16);
  // Ask for both a name-required, and a numeric host.
  ares_getnameinfo(channel_, (const struct sockaddr*)&sockaddr, sizeof(sockaddr),
                   ARES_NI_LOOKUPHOST|ARES_NI_LOOKUPSERVICE|ARES_NI_UDP|ARES_NI_NUMERICHOST|ARES_NI_NAMEREQD,
                   NameInfoCallback, &result);
  Process();
  EXPECT_TRUE(result.done_);
  EXPECT_EQ(ARES_EBADFLAGS, result.status_);
}

VIRT_NONVIRT_TEST_F(DefaultChannelTest, LiveGetServiceInfo) {
  NameInfoResult result;
  struct sockaddr_in sockaddr;
  memset(&sockaddr, 0, sizeof(sockaddr));
  sockaddr.sin_family = AF_INET;
  sockaddr.sin_port = htons(53);
  sockaddr.sin_addr.s_addr = htonl(0x08080808);
  // Just look up service info
  ares_getnameinfo(channel_, (const struct sockaddr*)&sockaddr, sizeof(sockaddr),
                   ARES_NI_LOOKUPSERVICE|ARES_NI_SCTP,
                   NameInfoCallback, &result);
  Process();
  EXPECT_TRUE(result.done_);
  EXPECT_EQ(ARES_SUCCESS, result.status_);
  EXPECT_EQ("", result.node_);
}

VIRT_NONVIRT_TEST_F(DefaultChannelTest, LiveGetServiceInfoNumeric) {
  NameInfoResult result;
  struct sockaddr_in sockaddr;
  memset(&sockaddr, 0, sizeof(sockaddr));
  sockaddr.sin_family = AF_INET;
  sockaddr.sin_port = htons(53);
  sockaddr.sin_addr.s_addr = htonl(0x08080808);
  // Just look up service info
  ares_getnameinfo(channel_, (const struct sockaddr*)&sockaddr, sizeof(sockaddr),
                   ARES_NI_LOOKUPSERVICE|ARES_NI_SCTP|ARES_NI_NUMERICSERV,
                   NameInfoCallback, &result);
  Process();
  EXPECT_TRUE(result.done_);
  EXPECT_EQ(ARES_SUCCESS, result.status_);
  EXPECT_EQ("", result.node_);
  EXPECT_EQ("53", result.service_);
}

VIRT_NONVIRT_TEST_F(DefaultChannelTest, LiveGetNameInfoAllocFail) {
  NameInfoResult result;
  struct sockaddr_in sockaddr;
  memset(&sockaddr, 0, sizeof(sockaddr));
  sockaddr.sin_family = AF_INET;
  sockaddr.sin_port = htons(53);
  sockaddr.sin_addr.s_addr = htonl(0x08080808);
  SetAllocFail(1);
  ares_getnameinfo(channel_, (const struct sockaddr*)&sockaddr, sizeof(sockaddr),
                   ARES_NI_LOOKUPHOST|ARES_NI_LOOKUPSERVICE|ARES_NI_UDP,
                   NameInfoCallback, &result);
  Process();
  EXPECT_TRUE(result.done_);
  EXPECT_EQ(ARES_ENOMEM, result.status_);
}



TEST_F(DefaultChannelTest, LiveSetServers) {
  struct ares_addr_node server1;
  struct ares_addr_node server2;
  server1.next = &server2;
  server1.family = AF_INET;
  server1.addr.addr4.s_addr = htonl(0x01020304);
  server2.next = nullptr;
  server2.family = AF_INET;
  server2.addr.addr4.s_addr = htonl(0x02030405);

  HostResult result;
  ares_gethostbyname(channel_, "www.google.com.", AF_INET, HostCallback, &result);
  EXPECT_EQ(ARES_SUCCESS, ares_set_servers(channel_, &server1));
  ares_cancel(channel_);
}

TEST_F(DefaultChannelTest, LiveSetServersPorts) {
  struct ares_addr_port_node server1;
  struct ares_addr_port_node server2;
  server1.next = &server2;
  server1.family = AF_INET;
  server1.addr.addr4.s_addr = htonl(0x01020304);
  server1.udp_port = 111;
  server1.tcp_port = 111;
  server2.next = nullptr;
  server2.family = AF_INET;
  server2.addr.addr4.s_addr = htonl(0x02030405);
  server2.udp_port = 0;
  server2.tcp_port = 0;
  EXPECT_EQ(ARES_ENODATA, ares_set_servers_ports(nullptr, &server1));

  // Change while pending will requeue any requests to new servers
  HostResult result;
  ares_gethostbyname(channel_, "www.google.com.", AF_INET, HostCallback, &result);
  EXPECT_EQ(ARES_SUCCESS, ares_set_servers_ports(channel_, &server1));
  ares_cancel(channel_);
}

TEST_F(DefaultChannelTest, LiveSetServersCSV) {
  HostResult result;
  ares_gethostbyname(channel_, "www.google.com.", AF_INET, HostCallback, &result);
  // Change while pending will requeue any requests to new servers
  EXPECT_EQ(ARES_SUCCESS, ares_set_servers_csv(channel_, "1.2.3.4,2.3.4.5"));
  EXPECT_EQ(ARES_SUCCESS, ares_set_servers_ports_csv(channel_, "1.2.3.4:56,2.3.4.5:67"));
  ares_cancel(channel_);
}


}  // namespace test
}  // namespace ares
