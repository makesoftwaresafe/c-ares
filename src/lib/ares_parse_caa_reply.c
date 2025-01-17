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

/* =============================================================================
 * NOTE:  The below copyright is preserved from the original author.  In
 *        October 2023, there were attempts made to contact the author in order
 *        gain approval for relicensing to the modern MIT license from the
 *        below 1989 variant, but all contact information for the author is
 *        no longer valid.
 *
 * Copyright (c) 2020 <danny.sonnenschein@platynum.ch>
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting
 * documentation, and that the name of M.I.T. not be used in
 * advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.
 * M.I.T. makes no representations about the suitability of
 * this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * =============================================================================
 */

#include "ares_setup.h"

#ifdef HAVE_NETINET_IN_H
#  include <netinet/in.h>
#endif
#ifdef HAVE_NETDB_H
#  include <netdb.h>
#endif
#ifdef HAVE_ARPA_INET_H
#  include <arpa/inet.h>
#endif

#include "ares_nameser.h"

#ifdef HAVE_STRINGS_H
#  include <strings.h>
#endif

#include "ares.h"
#include "ares_dns.h"
#include "ares_data.h"
#include "ares_private.h"

int ares_parse_caa_reply(const unsigned char *abuf, int alen_int,
                         struct ares_caa_reply **caa_out)
{
  size_t                 qdcount, ancount, i;
  const unsigned char   *aptr;
  const unsigned char   *strptr;
  ares_status_t          status;
  int                    rr_type, rr_class;
  size_t                 rr_len;
  size_t                 len;
  size_t                 alen;
  char                  *hostname = NULL, *rr_name = NULL;
  struct ares_caa_reply *caa_head = NULL;
  struct ares_caa_reply *caa_last = NULL;
  struct ares_caa_reply *caa_curr;

  /* Set *caa_out to NULL for all failure cases. */
  *caa_out = NULL;

  if (alen_int < 0) {
    return ARES_EBADRESP;
  }

  alen = (size_t)alen_int;

  /* Give up if abuf doesn't have room for a header. */
  if (alen < HFIXEDSZ) {
    return ARES_EBADRESP;
  }

  /* Fetch the question and answer count from the header. */
  qdcount = DNS_HEADER_QDCOUNT(abuf);
  ancount = DNS_HEADER_ANCOUNT(abuf);
  if (qdcount != 1) {
    return ARES_EBADRESP;
  }
  if (ancount == 0) {
    return ARES_ENODATA;
  }

  /* Expand the name from the question, and skip past the question. */
  aptr   = abuf + HFIXEDSZ;
  status = ares__expand_name_for_response(aptr, abuf, alen, &hostname, &len,
                                          ARES_TRUE);
  if (status != ARES_SUCCESS) {
    return (int)status;
  }

  if (aptr + len + QFIXEDSZ > abuf + alen) {
    ares_free(hostname);
    return ARES_EBADRESP;
  }
  aptr += len + QFIXEDSZ;

  /* Examine each answer resource record (RR) in turn. */
  for (i = 0; i < ancount; i++) {
    /* Decode the RR up to the data field. */
    status = ares__expand_name_for_response(aptr, abuf, alen, &rr_name, &len,
                                            ARES_FALSE);
    if (status != ARES_SUCCESS) {
      break;
    }
    aptr += len;
    if (aptr + RRFIXEDSZ > abuf + alen) {
      status = ARES_EBADRESP;
      break;
    }
    rr_type   = DNS_RR_TYPE(aptr);
    rr_class  = DNS_RR_CLASS(aptr);
    rr_len    = DNS_RR_LEN(aptr);
    aptr     += RRFIXEDSZ;
    if (aptr + rr_len > abuf + alen) {
      status = ARES_EBADRESP;
      break;
    }

    /* Check if we are really looking at a CAA record */
    if ((rr_class == C_IN || rr_class == C_CHAOS) && rr_type == T_CAA) {
      strptr = aptr;

      /* Allocate storage for this CAA answer appending it to the list */
      caa_curr = ares_malloc_data(ARES_DATATYPE_CAA_REPLY);
      if (!caa_curr) {
        status = ARES_ENOMEM;
        break;
      }
      if (caa_last) {
        caa_last->next = caa_curr;
      } else {
        caa_head = caa_curr;
      }
      caa_last = caa_curr;
      if (rr_len < 2) {
        status = ARES_EBADRESP;
        break;
      }
      caa_curr->critical = (int)*strptr++;
      caa_curr->plength  = *strptr++;
      if (caa_curr->plength <= 0 || caa_curr->plength >= rr_len - 2) {
        status = ARES_EBADRESP;
        break;
      }
      caa_curr->property =
        ares_malloc(caa_curr->plength + 1 /* Including null byte */);
      if (caa_curr->property == NULL) {
        status = ARES_ENOMEM;
        break;
      }
      memcpy((char *)caa_curr->property, strptr, caa_curr->plength);
      /* Make sure we NULL-terminate */
      caa_curr->property[caa_curr->plength]  = 0;
      strptr                                += caa_curr->plength;

      caa_curr->length = rr_len - caa_curr->plength - 2;
      if (caa_curr->length <= 0) {
        status = ARES_EBADRESP;
        break;
      }
      caa_curr->value =
        ares_malloc(caa_curr->length + 1 /* Including null byte */);
      if (caa_curr->value == NULL) {
        status = ARES_ENOMEM;
        break;
      }
      memcpy((char *)caa_curr->value, strptr, caa_curr->length);
      /* Make sure we NULL-terminate */
      caa_curr->value[caa_curr->length] = 0;
    }

    /* Propagate any failures */
    if (status != ARES_SUCCESS) {
      break;
    }

    /* Don't lose memory in the next iteration */
    ares_free(rr_name);
    rr_name = NULL;

    /* Move on to the next record */
    aptr += rr_len;
  }

  if (hostname) {
    ares_free(hostname);
  }
  if (rr_name) {
    ares_free(rr_name);
  }

  /* clean up on error */
  if (status != ARES_SUCCESS) {
    if (caa_head) {
      ares_free_data(caa_head);
    }
    return (int)status;
  }

  /* everything looks fine, return the data */
  *caa_out = caa_head;

  return ARES_SUCCESS;
}
