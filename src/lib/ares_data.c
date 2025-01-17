/* MIT License
 *
 * Copyright (c) 2009 Daniel Stenberg
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


#include "ares_setup.h"

#include <stddef.h>

#include "ares.h"
#include "ares_data.h"
#include "ares_private.h"

/*
** ares_free_data() - c-ares external API function.
**
** This function must be used by the application to free data memory that
** has been internally allocated by some c-ares function and for which a
** pointer has already been returned to the calling application. The list
** of c-ares functions returning pointers that must be free'ed using this
** function is:
**
**   ares_get_servers()
**   ares_parse_srv_reply()
**   ares_parse_txt_reply()
*/

void ares_free_data(void *dataptr)
{
  while (dataptr != NULL) {
    struct ares_data *ptr;
    void             *next_data = NULL;

#ifdef __INTEL_COMPILER
#  pragma warning(push)
#  pragma warning(disable : 1684)
    /* 1684: conversion from pointer to same-sized integral type */
#endif

    ptr = (void *)((char *)dataptr - offsetof(struct ares_data, data));

#ifdef __INTEL_COMPILER
#  pragma warning(pop)
#endif

    if (ptr->mark != ARES_DATATYPE_MARK) {
      return;
    }

    switch (ptr->type) {
      case ARES_DATATYPE_MX_REPLY:

        if (ptr->data.mx_reply.next) {
          next_data = ptr->data.mx_reply.next;
        }
        if (ptr->data.mx_reply.host) {
          ares_free(ptr->data.mx_reply.host);
        }
        break;

      case ARES_DATATYPE_SRV_REPLY:

        if (ptr->data.srv_reply.next) {
          next_data = ptr->data.srv_reply.next;
        }
        if (ptr->data.srv_reply.host) {
          ares_free(ptr->data.srv_reply.host);
        }
        break;

      case ARES_DATATYPE_URI_REPLY:

        if (ptr->data.uri_reply.next) {
          next_data = ptr->data.uri_reply.next;
        }
        if (ptr->data.uri_reply.uri) {
          ares_free(ptr->data.uri_reply.uri);
        }
        break;

      case ARES_DATATYPE_TXT_REPLY:
      case ARES_DATATYPE_TXT_EXT:

        if (ptr->data.txt_reply.next) {
          next_data = ptr->data.txt_reply.next;
        }
        if (ptr->data.txt_reply.txt) {
          ares_free(ptr->data.txt_reply.txt);
        }
        break;

      case ARES_DATATYPE_ADDR_NODE:

        if (ptr->data.addr_node.next) {
          next_data = ptr->data.addr_node.next;
        }
        break;

      case ARES_DATATYPE_ADDR_PORT_NODE:

        if (ptr->data.addr_port_node.next) {
          next_data = ptr->data.addr_port_node.next;
        }
        break;

      case ARES_DATATYPE_NAPTR_REPLY:

        if (ptr->data.naptr_reply.next) {
          next_data = ptr->data.naptr_reply.next;
        }
        if (ptr->data.naptr_reply.flags) {
          ares_free(ptr->data.naptr_reply.flags);
        }
        if (ptr->data.naptr_reply.service) {
          ares_free(ptr->data.naptr_reply.service);
        }
        if (ptr->data.naptr_reply.regexp) {
          ares_free(ptr->data.naptr_reply.regexp);
        }
        if (ptr->data.naptr_reply.replacement) {
          ares_free(ptr->data.naptr_reply.replacement);
        }
        break;

      case ARES_DATATYPE_SOA_REPLY:
        if (ptr->data.soa_reply.nsname) {
          ares_free(ptr->data.soa_reply.nsname);
        }
        if (ptr->data.soa_reply.hostmaster) {
          ares_free(ptr->data.soa_reply.hostmaster);
        }
        break;

      case ARES_DATATYPE_CAA_REPLY:

        if (ptr->data.caa_reply.next) {
          next_data = ptr->data.caa_reply.next;
        }
        if (ptr->data.caa_reply.property) {
          ares_free(ptr->data.caa_reply.property);
        }
        if (ptr->data.caa_reply.value) {
          ares_free(ptr->data.caa_reply.value);
        }
        break;

      default:
        return;
    }

    ares_free(ptr);
    dataptr = next_data;
  }
}

/*
** ares_malloc_data() - c-ares internal helper function.
**
** This function allocates memory for a c-ares private ares_data struct
** for the specified ares_datatype, initializes c-ares private fields
** and zero initializes those which later might be used from the public
** API. It returns an interior pointer which can be passed by c-ares
** functions to the calling application, and that must be free'ed using
** c-ares external API function ares_free_data().
*/

void *ares_malloc_data(ares_datatype type)
{
  struct ares_data *ptr;

  ptr = ares_malloc(sizeof(struct ares_data));
  if (!ptr) {
    return NULL;
  }

  switch (type) {
    case ARES_DATATYPE_MX_REPLY:
      ptr->data.mx_reply.next     = NULL;
      ptr->data.mx_reply.host     = NULL;
      ptr->data.mx_reply.priority = 0;
      break;

    case ARES_DATATYPE_SRV_REPLY:
      ptr->data.srv_reply.next     = NULL;
      ptr->data.srv_reply.host     = NULL;
      ptr->data.srv_reply.priority = 0;
      ptr->data.srv_reply.weight   = 0;
      ptr->data.srv_reply.port     = 0;
      break;

    case ARES_DATATYPE_URI_REPLY:
      ptr->data.uri_reply.next     = NULL;
      ptr->data.uri_reply.priority = 0;
      ptr->data.uri_reply.weight   = 0;
      ptr->data.uri_reply.uri      = NULL;
      ptr->data.uri_reply.ttl      = 0;
      break;

    case ARES_DATATYPE_TXT_EXT:
      ptr->data.txt_ext.record_start = 0;
      /* FALLTHROUGH */

    case ARES_DATATYPE_TXT_REPLY:
      ptr->data.txt_reply.next   = NULL;
      ptr->data.txt_reply.txt    = NULL;
      ptr->data.txt_reply.length = 0;
      break;

    case ARES_DATATYPE_CAA_REPLY:
      ptr->data.caa_reply.next     = NULL;
      ptr->data.caa_reply.plength  = 0;
      ptr->data.caa_reply.property = NULL;
      ptr->data.caa_reply.length   = 0;
      ptr->data.caa_reply.value    = NULL;
      break;

    case ARES_DATATYPE_ADDR_NODE:
      ptr->data.addr_node.next   = NULL;
      ptr->data.addr_node.family = 0;
      memset(&ptr->data.addr_node.addrV6, 0,
             sizeof(ptr->data.addr_node.addrV6));
      break;

    case ARES_DATATYPE_ADDR_PORT_NODE:
      ptr->data.addr_port_node.next     = NULL;
      ptr->data.addr_port_node.family   = 0;
      ptr->data.addr_port_node.udp_port = 0;
      ptr->data.addr_port_node.tcp_port = 0;
      memset(&ptr->data.addr_port_node.addrV6, 0,
             sizeof(ptr->data.addr_port_node.addrV6));
      break;

    case ARES_DATATYPE_NAPTR_REPLY:
      ptr->data.naptr_reply.next        = NULL;
      ptr->data.naptr_reply.flags       = NULL;
      ptr->data.naptr_reply.service     = NULL;
      ptr->data.naptr_reply.regexp      = NULL;
      ptr->data.naptr_reply.replacement = NULL;
      ptr->data.naptr_reply.order       = 0;
      ptr->data.naptr_reply.preference  = 0;
      break;

    case ARES_DATATYPE_SOA_REPLY:
      ptr->data.soa_reply.nsname     = NULL;
      ptr->data.soa_reply.hostmaster = NULL;
      ptr->data.soa_reply.serial     = 0;
      ptr->data.soa_reply.refresh    = 0;
      ptr->data.soa_reply.retry      = 0;
      ptr->data.soa_reply.expire     = 0;
      ptr->data.soa_reply.minttl     = 0;
      break;

    default:
      ares_free(ptr);
      return NULL;
  }

  ptr->mark = ARES_DATATYPE_MARK;
  ptr->type = type;

  return &ptr->data;
}
