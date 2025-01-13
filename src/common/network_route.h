/*
 * @Author: DI JUNKUN
 * @Date: 2025-01-13
 * Copyright (c) 2025 by DI JUNKUN, All Rights Reserved.
 */

#ifndef _NETWORK_ROUTE_H_
#define _NETWORK_ROUTE_H_

#include <cstdint>
#include <string>

struct NetworkRoute;

class RouteEndpoint {
 public:
  enum AdapterType {
    // This enum resembles the one in Chromium net::ConnectionType.
    ADAPTER_TYPE_UNKNOWN = 0,
    ADAPTER_TYPE_ETHERNET = 1 << 0,
    ADAPTER_TYPE_WIFI = 1 << 1,
    ADAPTER_TYPE_CELLULAR = 1 << 2,  // This is CELLULAR of unknown type.
    ADAPTER_TYPE_VPN = 1 << 3,
    ADAPTER_TYPE_LOOPBACK = 1 << 4,
    // ADAPTER_TYPE_ANY is used for a network, which only contains a single "any
    // address" IP address (INADDR_ANY for IPv4 or in6addr_any for IPv6), and
    // can
    // use any/all network interfaces. Whereas ADAPTER_TYPE_UNKNOWN is used
    // when the network uses a specific interface/IP, but its interface type can
    // not be determined or not fit in this enum.
    ADAPTER_TYPE_ANY = 1 << 5,
    ADAPTER_TYPE_CELLULAR_2G = 1 << 6,
    ADAPTER_TYPE_CELLULAR_3G = 1 << 7,
    ADAPTER_TYPE_CELLULAR_4G = 1 << 8,
    ADAPTER_TYPE_CELLULAR_5G = 1 << 9
  };

 public:
  RouteEndpoint() {}  // Used by tests.
  RouteEndpoint(AdapterType adapter_type, uint16_t adapter_id,
                uint16_t network_id, bool uses_turn)
      : adapter_type_(adapter_type),
        adapter_id_(adapter_id),
        network_id_(network_id),
        uses_turn_(uses_turn) {}

  RouteEndpoint(const RouteEndpoint&) = default;
  RouteEndpoint& operator=(const RouteEndpoint&) = default;

  bool operator==(const RouteEndpoint& other) const {
    return adapter_type_ == other.adapter_type_ &&
           adapter_id_ == other.adapter_id_ &&
           network_id_ == other.network_id_ && uses_turn_ == other.uses_turn_;
  }

  // Used by tests.
  static RouteEndpoint CreateWithNetworkId(uint16_t network_id) {
    return RouteEndpoint(ADAPTER_TYPE_UNKNOWN,
                         /* adapter_id = */ 0, network_id,
                         /* uses_turn = */ false);
  }
  RouteEndpoint CreateWithTurn(bool uses_turn) const {
    return RouteEndpoint(adapter_type_, adapter_id_, network_id_, uses_turn);
  }

  AdapterType adapter_type() const { return adapter_type_; }
  uint16_t adapter_id() const { return adapter_id_; }
  uint16_t network_id() const { return network_id_; }
  bool uses_turn() const { return uses_turn_; }

 private:
  AdapterType adapter_type_ = ADAPTER_TYPE_UNKNOWN;
  uint16_t adapter_id_ = 0;
  uint16_t network_id_ = 0;
  bool uses_turn_ = false;
};

struct NetworkRoute {
  bool connected = false;
  RouteEndpoint local;
  RouteEndpoint remote;
  // Last packet id sent on the PREVIOUS route.
  int last_sent_packet_id = -1;
  // The overhead in bytes from IP layer and above.
  // This is the maximum of any part of the route.
  int packet_overhead = 0;

  bool operator==(const NetworkRoute& other) const {
    return connected == other.connected && local == other.local &&
           remote == other.remote && packet_overhead == other.packet_overhead &&
           last_sent_packet_id == other.last_sent_packet_id;
  }

  bool operator!=(const NetworkRoute& other) { return !operator==(other); }
};

#endif