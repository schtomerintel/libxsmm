/******************************************************************************
** Copyright (c) 2015-2019, Intel Corporation                                **
** All rights reserved.                                                      **
**                                                                           **
** Redistribution and use in source and binary forms, with or without        **
** modification, are permitted provided that the following conditions        **
** are met:                                                                  **
** 1. Redistributions of source code must retain the above copyright         **
**    notice, this list of conditions and the following disclaimer.          **
** 2. Redistributions in binary form must reproduce the above copyright      **
**    notice, this list of conditions and the following disclaimer in the    **
**    documentation and/or other materials provided with the distribution.   **
** 3. Neither the name of the copyright holder nor the names of its          **
**    contributors may be used to endorse or promote products derived        **
**    from this software without specific prior written permission.          **
**                                                                           **
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS       **
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT         **
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR     **
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT      **
** HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,    **
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED  **
** TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR    **
** PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF    **
** LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING      **
** NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS        **
** SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.              **
******************************************************************************/
/* Alexander Heinecke (Intel Corp.)
******************************************************************************/
#include "generator_gemm_noarch.h"
#include "libxsmm_main.h"

#if defined(LIBXSMM_OFFLOAD_TARGET)
# pragma offload_attribute(push,target(LIBXSMM_OFFLOAD_TARGET))
#endif
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#if defined(LIBXSMM_OFFLOAD_TARGET)
# pragma offload_attribute(pop)
#endif

LIBXSMM_API_INTERN
void libxsmm_generator_gemm_noarch_kernel( libxsmm_generated_code*        io_generated_code,
                                           const libxsmm_gemm_descriptor* i_xgemm_desc ) {
  char l_new_code[512];
  int l_max_code_length = 511;
  int l_code_length = 0;

  l_code_length = LIBXSMM_SNPRINTF(l_new_code, l_max_code_length, "  unsigned int l_m = 0;\n");
  libxsmm_append_code_as_string( io_generated_code, l_new_code, l_code_length );
  l_code_length = LIBXSMM_SNPRINTF(l_new_code, l_max_code_length, "  unsigned int l_n = 0;\n");
  libxsmm_append_code_as_string( io_generated_code, l_new_code, l_code_length );
  l_code_length = LIBXSMM_SNPRINTF(l_new_code, l_max_code_length, "  unsigned int l_k = 0;\n\n");
  libxsmm_append_code_as_string( io_generated_code, l_new_code, l_code_length );

  l_code_length = LIBXSMM_SNPRINTF(l_new_code, l_max_code_length, "  for ( l_n = 0; l_n < %u; l_n++ ) {\n", (unsigned int)i_xgemm_desc->n);
  libxsmm_append_code_as_string( io_generated_code, l_new_code, l_code_length );
  if (0 != (LIBXSMM_GEMM_FLAG_BETA_0 & i_xgemm_desc->flags)) { /* Beta=0 */
    if ( LIBXSMM_GEMM_PRECISION_F64 == LIBXSMM_GETENUM_INP( i_xgemm_desc->datatype )  ) {
      l_code_length = LIBXSMM_SNPRINTF(l_new_code, l_max_code_length, "    for ( l_m = 0; l_m < %u; l_m++ ) { C[(l_n*%u)+l_m] = 0.0; }\n\n", (unsigned int)i_xgemm_desc->m, (unsigned int)i_xgemm_desc->ldc);
    } else {
      l_code_length = LIBXSMM_SNPRINTF(l_new_code, l_max_code_length, "    for ( l_m = 0; l_m < %u; l_m++ ) { C[(l_n*%u)+l_m] = 0.0f; }\n\n", (unsigned int)i_xgemm_desc->m, (unsigned int)i_xgemm_desc->ldc);
    }
    libxsmm_append_code_as_string( io_generated_code, l_new_code, l_code_length );
  }
  l_code_length = LIBXSMM_SNPRINTF(l_new_code, l_max_code_length, "    for ( l_k = 0; l_k < %u; l_k++ ) {\n", (unsigned int)i_xgemm_desc->k);
  libxsmm_append_code_as_string( io_generated_code, l_new_code, l_code_length );
  l_code_length = LIBXSMM_SNPRINTF(l_new_code, l_max_code_length, "      #pragma simd\n");
  libxsmm_append_code_as_string( io_generated_code, l_new_code, l_code_length );
  l_code_length = LIBXSMM_SNPRINTF(l_new_code, l_max_code_length, "      for ( l_m = 0; l_m < %u; l_m++ ) {\n", (unsigned int)i_xgemm_desc->m);
  libxsmm_append_code_as_string( io_generated_code, l_new_code, l_code_length );
  l_code_length = LIBXSMM_SNPRINTF(l_new_code, l_max_code_length, "        C[(l_n*%u)+l_m] += A[(l_k*%u)+l_m] * B[(l_n*%u)+l_k];\n", (unsigned int)i_xgemm_desc->ldc, (unsigned int)i_xgemm_desc->lda, (unsigned int)i_xgemm_desc->ldb);
  libxsmm_append_code_as_string( io_generated_code, l_new_code, l_code_length );
  l_code_length = LIBXSMM_SNPRINTF(l_new_code, l_max_code_length, "      }\n");
  libxsmm_append_code_as_string( io_generated_code, l_new_code, l_code_length );
  l_code_length = LIBXSMM_SNPRINTF(l_new_code, l_max_code_length, "    }\n");
  libxsmm_append_code_as_string( io_generated_code, l_new_code, l_code_length );
  l_code_length = LIBXSMM_SNPRINTF(l_new_code, l_max_code_length, "  }\n");
  libxsmm_append_code_as_string( io_generated_code, l_new_code, l_code_length );
}

