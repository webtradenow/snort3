//--------------------------------------------------------------------------
// Copyright (C) 2014-2016 Cisco and/or its affiliates. All rights reserved.
// Copyright (C) 2005-2013 Sourcefire, Inc.
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License Version 2 as published
// by the Free Software Foundation.  You may not use, modify or distribute
// this program under any other version of the GNU General Public License.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//--------------------------------------------------------------------------
// flow_key.cc author Steven Sturges <ssturges@sourcefire.com>

#include "flow/flow_key.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "hash/sfhashfcn.h"
#include "main/snort_config.h"
#include "protocols/icmp4.h"
#include "protocols/icmp6.h"
#include "utils/util.h"

//-------------------------------------------------------------------------
// init foo
//-------------------------------------------------------------------------

inline bool FlowKey::init4(
    IpProtocol ip_proto,
    const SfIp *srcIP, uint16_t srcPort,
    const SfIp *dstIP, uint16_t dstPort,
    uint32_t mplsId, bool order)
{
    uint32_t src;
    uint32_t dst;
    bool reversed = false;

    if ( ip_proto ==  IpProtocol::ICMPV4 )
    {
        if (srcPort == ICMP_ECHOREPLY)
        {
            dstPort = ICMP_ECHO; /* Treat ICMP echo reply the same as request */
            srcPort = 0;
        }
        else /* otherwise, every ICMP type gets different key */
        {
            dstPort = 0;
        }
    }

    src = srcIP->get_ip4_value();
    dst = dstIP->get_ip4_value();

    /* These comparisons are done in this fashion for performance reasons */
    if ( !order || src < dst)
    {
        COPY4(ip_l, srcIP->get_ip6_ptr());
        COPY4(ip_h, dstIP->get_ip6_ptr());
        port_l = srcPort;
        port_h = dstPort;
    }
    else if (src == dst)
    {
        COPY4(ip_l, srcIP->get_ip6_ptr());
        COPY4(ip_h, dstIP->get_ip6_ptr());
        if (srcPort < dstPort)
        {
            port_l = srcPort;
            port_h = dstPort;
        }
        else
        {
            port_l = dstPort;
            port_h = srcPort;
            reversed = true;
        }
    }
    else
    {
        COPY4(ip_l, dstIP->get_ip6_ptr());
        port_l = dstPort;
        COPY4(ip_h, srcIP->get_ip6_ptr());
        port_h = srcPort;
        reversed = true;
    }
    if (SnortConfig::mpls_overlapping_ip() &&
        ip::isPrivateIP(src) && ip::isPrivateIP(dst))
        mplsLabel = mplsId;
    else
        mplsLabel = 0;

    return reversed;
}

inline bool FlowKey::init6(
    IpProtocol ip_proto,
    const SfIp *srcIP, uint16_t srcPort,
    const SfIp *dstIP, uint16_t dstPort,
    uint32_t mplsId, bool order)
{
    bool reversed = false;

    if ( ip_proto == IpProtocol::ICMPV4 )
    {
        if (srcPort == ICMP_ECHOREPLY)
        {
            /* Treat ICMP echo reply the same as request */
            dstPort = ICMP_ECHO;
            srcPort = 0;
        }
        else
        {
            /* otherwise, every ICMP type gets different key */
            dstPort = 0;
        }
    }
    else if ( ip_proto == IpProtocol::ICMPV6 )
    {
        if (srcPort == icmp::Icmp6Types::ECHO_REPLY)
        {
            /* Treat ICMPv6 echo reply the same as request */
            dstPort = icmp::Icmp6Types::ECHO_REQUEST;
            srcPort = 0;
        }
        else
        {
            /* otherwise, every ICMP type gets different key */
            dstPort = 0;
        }
    }

    if ( !order || srcIP->fast_lt6(*dstIP))
    {
        COPY4(ip_l, srcIP->get_ip6_ptr());
        port_l = srcPort;
        COPY4(ip_h, dstIP->get_ip6_ptr());
        port_h = dstPort;
    }
    else if (srcIP->fast_eq6(*dstIP))
    {
        COPY4(ip_l, srcIP->get_ip6_ptr());
        COPY4(ip_h, dstIP->get_ip6_ptr());
        if (srcPort < dstPort)
        {
            port_l = srcPort;
            port_h = dstPort;
        }
        else
        {
            port_l = dstPort;
            port_h = srcPort;
            reversed = true;
        }
    }
    else
    {
        COPY4(ip_l, dstIP->get_ip6_ptr());
        port_l = dstPort;
        COPY4(ip_h, srcIP->get_ip6_ptr());
        port_h = srcPort;
        reversed = true;
    }

    if (SnortConfig::mpls_overlapping_ip())
        mplsLabel = mplsId;
    else
        mplsLabel = 0;

    return reversed;
}

void FlowKey::init_vlan(uint16_t vlanId)
{
    if (!SnortConfig::get_vlan_agnostic())
        vlan_tag = vlanId;
    else
        vlan_tag = 0;
}

void FlowKey::init_address_space(uint16_t addrSpaceId)
{
    if (!SnortConfig::address_space_agnostic())
        addressSpaceId = addrSpaceId;
    else
        addressSpaceId = 0;
    addressSpaceIdPad1 = 0;
}

void FlowKey::init_mpls(uint32_t mplsId)
{
    if (SnortConfig::mpls_overlapping_ip())
        mplsLabel = mplsId;
    else
        mplsLabel = 0;
}

bool FlowKey::init(
    PktType type, IpProtocol ip_proto,
    const SfIp *srcIP, uint16_t srcPort,
    const SfIp *dstIP, uint16_t dstPort,
    uint16_t vlanId, uint32_t mplsId, uint16_t addrSpaceId)
{
    bool reversed;

    /* Because the key is going to be used for hash lookups,
     * the key fields will be normalized such that the lower
     * of the IP addresses is stored in ip_l and the port for
     * that IP is stored in port_l.
     */
    if (srcIP->is_ip4())
    {
        version = 4;
        reversed = init4(ip_proto, srcIP, srcPort, dstIP, dstPort, mplsId);
    }
    else
    {
        version = 6;
        reversed = init6(ip_proto, srcIP, srcPort, dstIP, dstPort, mplsId);
    }

    pkt_type = type;

    init_vlan(vlanId);
    init_address_space(addrSpaceId);

    return reversed;
}

bool FlowKey::init(
    PktType type, IpProtocol ip_proto,
    const SfIp *srcIP, const SfIp *dstIP,
    uint32_t id, uint16_t vlanId,
    uint32_t mplsId, uint16_t addrSpaceId)
{
    // to avoid confusing 2 different datagrams or confusing a datagram
    // with a session, we don't order the addresses and we set version
    uint16_t srcPort = id & 0xFFFF;
    uint16_t dstPort = id >> 16;

    if (srcIP->is_ip4())
    {
        version = 4;
        init4(ip_proto, srcIP, srcPort, dstIP, dstPort, mplsId, false);
    }
    else
    {
        version = 6;
        init6(ip_proto, srcIP, srcPort, dstIP, dstPort, mplsId, false);
    }
    pkt_type = type;

    init_vlan(vlanId);
    init_address_space(addrSpaceId);

    return false;
}

//-------------------------------------------------------------------------
// hash foo
//-------------------------------------------------------------------------

uint32_t FlowKey::hash(SFHASHFCN*, unsigned char* d, int)
{
    uint32_t a,b,c;

    a = *(uint32_t*)d;         /* IPv6 lo[0] */
    b = *(uint32_t*)(d+4);     /* IPv6 lo[1] */
    c = *(uint32_t*)(d+8);     /* IPv6 lo[2] */

    mix(a,b,c);

    a += *(uint32_t*)(d+12);   /* IPv6 lo[3] */
    b += *(uint32_t*)(d+16);   /* IPv6 hi[0] */
    c += *(uint32_t*)(d+20);   /* IPv6 hi[1] */

    mix(a,b,c);

    a += *(uint32_t*)(d+24);   /* IPv6 hi[2] */
    b += *(uint32_t*)(d+28);   /* IPv6 hi[3] */
    c += *(uint32_t*)(d+32);   /* port lo & port hi */

    mix(a,b,c);

    a += *(uint32_t*)(d+36);    /* vlan tag, packet type, & version */
    b += *(uint32_t*)(d+40);    /* mpls label */
    c += *(uint32_t*)(d+44);    /* address space id and 16bits of zero'd pad */

    finalize(a,b,c);

    return c;
}

int FlowKey::compare(const void* s1, const void* s2, size_t)
{
    uint64_t* a,* b;

    a = (uint64_t*)s1;
    b = (uint64_t*)s2;
    if (*a - *b)
        return 1;               /* Compares IPv4 lo/hi
                                   Compares IPv6 low[0,1] */

    a++;
    b++;
    if (*a - *b)
        return 1;               /* Compares port lo/hi, vlan, protocol, version
                                   Compares IPv6 low[2,3] */

    a++;
    b++;
    if (*a - *b)
        return 1;               /* Compares IPv6 hi[0,1] */

    a++;
    b++;
    if (*a - *b)
        return 1;               /* Compares IPv6 hi[2,3] */

    a++;
    b++;
    if (*a - *b)
        return 1;               /* Compares port lo/hi, vlan, protocol, version */

    a++;
    b++;
    if (*a - *b)
        return 1;               /* Compares MPLS label, AddressSpace ID and 16bit pad */

    return 0;
}

