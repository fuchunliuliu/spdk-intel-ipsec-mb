/*******************************************************************************
  Copyright (c) 2009-2019, Intel Corporation

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

      * Redistributions of source code must retain the above copyright notice,
        this list of conditions and the following disclaimer.
      * Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.
      * Neither the name of Intel Corporation nor the names of its contributors
        may be used to endorse or promote products derived from this software
        without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*******************************************************************************/

#ifndef _SNOW3G_TABLES_H_
#define _SNOW3G_TABLES_H_

#include <stdint.h>
#include "constant_lookup.h"

#if defined (AVX) || defined (AVX2)
#define SNOW3G_SAFE_LOOKUP_W0(table, idx, size) \
        ((uint32_t)(LOOKUP64_AVX(table, idx, size) >> 0))
#define SNOW3G_SAFE_LOOKUP_W1(table, idx, size) \
        ((uint32_t)(LOOKUP64_AVX(table, idx, size) >> 8))
#define SNOW3G_SAFE_LOOKUP_W2(table, idx, size) \
        ((uint32_t)(LOOKUP64_AVX(table, idx, size) >> 16))
#define SNOW3G_SAFE_LOOKUP_W3(table, idx, size) \
        ((uint32_t)(LOOKUP64_AVX(table, idx, size) >> 24))
#else /* SSE */
#define SNOW3G_SAFE_LOOKUP_W0(table, idx, size)                 \
        ((uint32_t)(LOOKUP64_SSE(table, idx, size) >> 0))
#define SNOW3G_SAFE_LOOKUP_W1(table, idx, size) \
        ((uint32_t)(LOOKUP64_SSE(table, idx, size) >> 8))
#define SNOW3G_SAFE_LOOKUP_W2(table, idx, size) \
        ((uint32_t)(LOOKUP64_SSE(table, idx, size) >> 16))
#define SNOW3G_SAFE_LOOKUP_W3(table, idx, size) \
        ((uint32_t)(LOOKUP64_SSE(table, idx, size) >> 24))
#endif /* AVX || AVX2 */

extern const int snow3g_table_A_mul[256];
extern const int snow3g_table_A_div[256];
extern const uint64_t snow3g_table_S2[256];
#ifdef AVX2
extern const int S2_T0[256];
extern const int S2_T1[256];
extern const int S2_T2[256];
extern const int S2_T3[256];
#endif /* AVX2 */


#endif /* _SNOW3G_TABLES_H_  */