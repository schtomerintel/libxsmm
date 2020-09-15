/******************************************************************************
* Copyright (c) Intel Corporation - All rights reserved.                      *
* This file is part of the LIBXSMM library.                                   *
*                                                                             *
* For information on the license, see the LICENSE file.                       *
* Further information: https://github.com/hfp/libxsmm/                        *
* SPDX-License-Identifier: BSD-3-Clause                                       *
******************************************************************************/
/* Evangelos Georganas, Alexander Heinecke (Intel Corp.)
***************************************************`***************************/
#include "generator_gemm_common.h"
#include "generator_x86_instructions.h"
#include "generator_common.h"
#include "libxsmm_main.h"
#include "generator_gemm_amx.h"
#include "generator_gemm_amx_microkernel.h"

#if defined(LIBXSMM_OFFLOAD_TARGET)
# pragma offload_attribute(push,target(LIBXSMM_OFFLOAD_TARGET))
#endif
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#if defined(LIBXSMM_OFFLOAD_TARGET)
# pragma offload_attribute(pop)
#endif

LIBXSMM_API_INTERN
void libxsmm_generator_gemm_header_reduceloop_amx( libxsmm_generated_code*             io_generated_code,
    libxsmm_loop_label_tracker*        io_loop_label_tracker,
    const libxsmm_gp_reg_mapping*      i_gp_reg_mapping,
    const libxsmm_micro_kernel_config* i_micro_kernel_config ) {
  libxsmm_x86_instruction_alu_imm( io_generated_code, i_micro_kernel_config->alu_mov_instruction, i_gp_reg_mapping->gp_reg_reduce_loop, 0);
  libxsmm_x86_instruction_register_jump_back_label( io_generated_code, io_loop_label_tracker );
}


LIBXSMM_API_INTERN
void libxsmm_generator_gemm_header_nloop_amx( libxsmm_generated_code*             io_generated_code,
    libxsmm_loop_label_tracker*        io_loop_label_tracker,
    const libxsmm_gp_reg_mapping*      i_gp_reg_mapping,
    const libxsmm_micro_kernel_config* i_micro_kernel_config,
    const unsigned int                 i_n_blocking) {
  libxsmm_x86_instruction_alu_imm( io_generated_code, i_micro_kernel_config->alu_mov_instruction, i_gp_reg_mapping->gp_reg_nloop, 0);
  libxsmm_x86_instruction_register_jump_back_label( io_generated_code, io_loop_label_tracker );
  libxsmm_x86_instruction_alu_imm( io_generated_code, i_micro_kernel_config->alu_add_instruction, i_gp_reg_mapping->gp_reg_nloop, i_n_blocking );
  libxsmm_x86_instruction_alu_imm( io_generated_code, i_micro_kernel_config->alu_mov_instruction, i_gp_reg_mapping->gp_reg_mloop, 0 );
}


LIBXSMM_API_INTERN
void libxsmm_generator_gemm_header_mloop_amx( libxsmm_generated_code*             io_generated_code,
    libxsmm_loop_label_tracker*        io_loop_label_tracker,
    const libxsmm_gp_reg_mapping*      i_gp_reg_mapping,
    const libxsmm_micro_kernel_config* i_micro_kernel_config,
    const unsigned int                 i_m_blocking ) {
  libxsmm_x86_instruction_alu_imm( io_generated_code, i_micro_kernel_config->alu_mov_instruction, i_gp_reg_mapping->gp_reg_mloop, 0);
  libxsmm_x86_instruction_register_jump_back_label( io_generated_code, io_loop_label_tracker );
  libxsmm_x86_instruction_alu_imm( io_generated_code, i_micro_kernel_config->alu_add_instruction, i_gp_reg_mapping->gp_reg_mloop, i_m_blocking );
}


LIBXSMM_API_INTERN
void libxsmm_generator_gemm_footer_reduceloop_amx( libxsmm_generated_code*             io_generated_code,
    libxsmm_loop_label_tracker*        io_loop_label_tracker,
    const libxsmm_gp_reg_mapping*      i_gp_reg_mapping,
    const libxsmm_micro_kernel_config* i_micro_kernel_config,
    const libxsmm_gemm_descriptor*     i_xgemm_desc) {
  LIBXSMM_UNUSED(i_xgemm_desc);
  libxsmm_x86_instruction_alu_imm( io_generated_code, i_micro_kernel_config->alu_add_instruction, i_gp_reg_mapping->gp_reg_reduce_loop, 1);
  libxsmm_x86_instruction_alu_reg( io_generated_code, i_micro_kernel_config->alu_cmp_instruction, i_gp_reg_mapping->gp_reg_reduce_count, i_gp_reg_mapping->gp_reg_reduce_loop);
  libxsmm_x86_instruction_jump_back_to_label( io_generated_code, i_micro_kernel_config->alu_jmp_instruction, io_loop_label_tracker );
}


LIBXSMM_API_INTERN
void libxsmm_generator_gemm_footer_nloop_amx( libxsmm_generated_code*             io_generated_code,
    libxsmm_loop_label_tracker*        io_loop_label_tracker,
    const libxsmm_gp_reg_mapping*      i_gp_reg_mapping,
    const libxsmm_micro_kernel_config* i_micro_kernel_config,
    const libxsmm_gemm_descriptor*     i_xgemm_desc,
    const unsigned int                 i_n_blocking,
    const unsigned int                 i_n_done,
    const unsigned int                 i_m_loop_exists) {

  if ( LIBXSMM_GEMM_PRECISION_BF16 == LIBXSMM_GETENUM_OUT( i_xgemm_desc->datatype ) ) {
    if (i_micro_kernel_config->vnni_format_C == 0) {
      libxsmm_x86_instruction_alu_imm( io_generated_code, i_micro_kernel_config->alu_add_instruction, i_gp_reg_mapping->gp_reg_c,
        (i_n_blocking*(i_xgemm_desc->ldc)*(i_micro_kernel_config->datatype_size/2)) - ((i_xgemm_desc->m) * i_m_loop_exists * (i_micro_kernel_config->datatype_size/2)) );
    } else {
      libxsmm_x86_instruction_alu_imm( io_generated_code, i_micro_kernel_config->alu_add_instruction, i_gp_reg_mapping->gp_reg_c,
        (i_n_blocking*(i_xgemm_desc->ldc)*(i_micro_kernel_config->datatype_size/2)) - ((i_xgemm_desc->m) * i_m_loop_exists * 2 * (i_micro_kernel_config->datatype_size/2)) );
    }
  } else if ( LIBXSMM_GEMM_PRECISION_I8 == LIBXSMM_GETENUM_OUT( i_xgemm_desc->datatype ) ) {
    libxsmm_x86_instruction_alu_imm( io_generated_code, i_micro_kernel_config->alu_add_instruction, i_gp_reg_mapping->gp_reg_c,
        (i_n_blocking*(i_xgemm_desc->ldc)*(i_micro_kernel_config->datatype_size/4)) - ((i_xgemm_desc->m) * i_m_loop_exists * (i_micro_kernel_config->datatype_size/4)) );
  } else {
    libxsmm_x86_instruction_alu_imm( io_generated_code, i_micro_kernel_config->alu_add_instruction, i_gp_reg_mapping->gp_reg_c,
        (i_n_blocking*(i_xgemm_desc->ldc)*(i_micro_kernel_config->datatype_size)) - ((i_xgemm_desc->m) * i_m_loop_exists * (i_micro_kernel_config->datatype_size)) );
  }

  /* Also adjust eltwise pointers  */
  if ((i_micro_kernel_config->fused_relu == 1) || (i_micro_kernel_config->fused_bcolbias == 1) || (i_micro_kernel_config->fused_scolbias == 1) || (i_micro_kernel_config->overwrite_C == 0)) {
    libxsmm_x86_instruction_push_reg( io_generated_code, i_gp_reg_mapping->gp_reg_help_0 );
  }

  if ((i_micro_kernel_config->fused_relu == 1) && (i_micro_kernel_config->overwrite_C == 1) ) {
    libxsmm_generator_gemm_getval_stack_var( io_generated_code, i_micro_kernel_config, LIBXSMM_GEMM_STACK_VAR_ELT_OUTPUT_PTR, i_gp_reg_mapping->gp_reg_help_0 );
    libxsmm_x86_instruction_alu_imm( io_generated_code, i_micro_kernel_config->alu_add_instruction, i_gp_reg_mapping->gp_reg_help_0, (i_n_blocking*i_xgemm_desc->ldc)/8 - ((i_xgemm_desc->m/8) * i_m_loop_exists) );
    libxsmm_generator_gemm_setval_stack_var( io_generated_code, i_micro_kernel_config, LIBXSMM_GEMM_STACK_VAR_ELT_OUTPUT_PTR, i_gp_reg_mapping->gp_reg_help_0 );
  }

  /* In this case also advance the output ptr */
  if (i_micro_kernel_config->overwrite_C == 0) {
    libxsmm_generator_gemm_getval_stack_var( io_generated_code, i_micro_kernel_config, LIBXSMM_GEMM_STACK_VAR_ELT_OUTPUT_PTR, i_gp_reg_mapping->gp_reg_help_0 );
    libxsmm_x86_instruction_alu_imm( io_generated_code, i_micro_kernel_config->alu_add_instruction, i_gp_reg_mapping->gp_reg_help_0, (i_n_blocking*(i_xgemm_desc->ldc)*(i_micro_kernel_config->datatype_size/2)) - ((i_xgemm_desc->m) * i_m_loop_exists * (i_micro_kernel_config->datatype_size/2)) );
    libxsmm_generator_gemm_setval_stack_var( io_generated_code, i_micro_kernel_config, LIBXSMM_GEMM_STACK_VAR_ELT_OUTPUT_PTR, i_gp_reg_mapping->gp_reg_help_0 );
  }

  if (i_micro_kernel_config->fused_bcolbias == 1) {
    libxsmm_generator_gemm_getval_stack_var( io_generated_code, i_micro_kernel_config, LIBXSMM_GEMM_STACK_VAR_ELT_BIAS_PTR, i_gp_reg_mapping->gp_reg_help_0 );
    libxsmm_x86_instruction_alu_imm( io_generated_code, i_micro_kernel_config->alu_add_instruction, i_gp_reg_mapping->gp_reg_help_0, -( i_xgemm_desc->m  * i_m_loop_exists * (i_micro_kernel_config->datatype_size/2)) );
    libxsmm_generator_gemm_setval_stack_var( io_generated_code, i_micro_kernel_config, LIBXSMM_GEMM_STACK_VAR_ELT_BIAS_PTR, i_gp_reg_mapping->gp_reg_help_0 );
  }

  if (i_micro_kernel_config->fused_scolbias == 1) {
    libxsmm_generator_gemm_getval_stack_var( io_generated_code, i_micro_kernel_config, LIBXSMM_GEMM_STACK_VAR_ELT_BIAS_PTR, i_gp_reg_mapping->gp_reg_help_0 );
    libxsmm_x86_instruction_alu_imm( io_generated_code, i_micro_kernel_config->alu_add_instruction, i_gp_reg_mapping->gp_reg_help_0, -( i_xgemm_desc->m  * i_m_loop_exists * i_micro_kernel_config->datatype_size) );
    libxsmm_generator_gemm_setval_stack_var( io_generated_code, i_micro_kernel_config, LIBXSMM_GEMM_STACK_VAR_ELT_BIAS_PTR, i_gp_reg_mapping->gp_reg_help_0 );
  }

  if ((i_micro_kernel_config->fused_relu == 1) || (i_micro_kernel_config->fused_bcolbias == 1) || (i_micro_kernel_config->fused_scolbias == 1) || (i_micro_kernel_config->overwrite_C == 0)) {
    libxsmm_x86_instruction_pop_reg( io_generated_code, i_gp_reg_mapping->gp_reg_help_0 );
  }

#if 0
  if ( i_xgemm_desc->prefetch == LIBXSMM_GEMM_PREFETCH_CL2 ||
      i_xgemm_desc->prefetch == LIBXSMM_GEMM_PREFETCH_AL2CL2BL2_VIA_C ) {
    libxsmm_x86_instruction_alu_imm( io_generated_code, i_micro_kernel_config->alu_add_instruction, i_gp_reg_mapping->gp_reg_c_prefetch,
        (i_n_blocking*(i_xgemm_desc->ldc)*(i_micro_kernel_config->datatype_size)) - ((i_xgemm_desc->m)*(i_micro_kernel_config->datatype_size)) );
  }
#endif
  if (i_xgemm_desc->flags & LIBXSMM_GEMM_FLAG_BATCH_REDUCE_ADDRESS) {
    /* handle trans B */
    int l_b_offset = 0;
    if ( (i_xgemm_desc->flags & LIBXSMM_GEMM_FLAG_TRANS_B) > 0 ) {
      l_b_offset = i_n_blocking * i_micro_kernel_config->datatype_size;
    } else {
      l_b_offset = i_n_blocking * i_xgemm_desc->ldb * i_micro_kernel_config->datatype_size;
    }
    libxsmm_generator_gemm_header_reduceloop_amx( io_generated_code, io_loop_label_tracker, i_gp_reg_mapping, i_micro_kernel_config );
    if (i_m_loop_exists) {
      libxsmm_x86_instruction_alu_mem( io_generated_code,
          i_micro_kernel_config->alu_mov_instruction,
          i_gp_reg_mapping->gp_reg_a_ptrs,
          i_gp_reg_mapping->gp_reg_reduce_loop, 8,
          0,
          i_gp_reg_mapping->gp_reg_a,
          0 );
      libxsmm_x86_instruction_alu_imm( io_generated_code, i_micro_kernel_config->alu_sub_instruction,
          i_gp_reg_mapping->gp_reg_a, ((i_xgemm_desc->m)*(i_micro_kernel_config->datatype_size)) );
      libxsmm_x86_instruction_alu_mem( io_generated_code,
          i_micro_kernel_config->alu_mov_instruction,
          i_gp_reg_mapping->gp_reg_a_ptrs,
          i_gp_reg_mapping->gp_reg_reduce_loop, 8,
          0,
          i_gp_reg_mapping->gp_reg_a,
          1 );
    }

    libxsmm_x86_instruction_alu_mem( io_generated_code,
        i_micro_kernel_config->alu_mov_instruction,
        i_gp_reg_mapping->gp_reg_b_ptrs,
        i_gp_reg_mapping->gp_reg_reduce_loop, 8,
        0,
        i_gp_reg_mapping->gp_reg_b,
        0 );
    libxsmm_x86_instruction_alu_imm( io_generated_code, i_micro_kernel_config->alu_add_instruction,
        i_gp_reg_mapping->gp_reg_b, l_b_offset );
    libxsmm_x86_instruction_alu_mem( io_generated_code,
        i_micro_kernel_config->alu_mov_instruction,
        i_gp_reg_mapping->gp_reg_b_ptrs,
        i_gp_reg_mapping->gp_reg_reduce_loop, 8,
        0,
        i_gp_reg_mapping->gp_reg_b,
        1 );
    libxsmm_generator_gemm_footer_reduceloop_amx( io_generated_code, io_loop_label_tracker, i_gp_reg_mapping, i_micro_kernel_config, i_xgemm_desc);
  } else if ((i_xgemm_desc->flags & LIBXSMM_GEMM_FLAG_BATCH_REDUCE_OFFSET) || (i_xgemm_desc->flags & LIBXSMM_GEMM_FLAG_BATCH_REDUCE_STRIDE)) {
    /* handle trans B */
    int l_b_offset = 0;
    if ( (i_xgemm_desc->flags & LIBXSMM_GEMM_FLAG_TRANS_B) > 0 ) {
      l_b_offset = i_n_blocking * i_micro_kernel_config->datatype_size;
    } else {
      l_b_offset = i_n_blocking * i_xgemm_desc->ldb * i_micro_kernel_config->datatype_size;
    }

    libxsmm_x86_instruction_alu_imm( io_generated_code, i_micro_kernel_config->alu_add_instruction,
        i_gp_reg_mapping->gp_reg_b_base, l_b_offset );

    if (i_m_loop_exists) {
      libxsmm_x86_instruction_alu_imm( io_generated_code, i_micro_kernel_config->alu_sub_instruction,
          i_gp_reg_mapping->gp_reg_a_base, ((i_xgemm_desc->m)*(i_micro_kernel_config->datatype_size)) );
    }
  } else {
    /* handle trans B */
    int l_b_offset = 0;
    if ( (i_xgemm_desc->flags & LIBXSMM_GEMM_FLAG_TRANS_B) > 0 ) {
      l_b_offset = i_n_blocking * i_micro_kernel_config->datatype_size;
    } else {
      l_b_offset = i_n_blocking * i_xgemm_desc->ldb * i_micro_kernel_config->datatype_size;
    }

    libxsmm_x86_instruction_alu_imm( io_generated_code, i_micro_kernel_config->alu_add_instruction,
        i_gp_reg_mapping->gp_reg_b, l_b_offset );

    if (i_m_loop_exists) {
      libxsmm_x86_instruction_alu_imm( io_generated_code, i_micro_kernel_config->alu_sub_instruction,
          i_gp_reg_mapping->gp_reg_a, ((i_xgemm_desc->m)*(i_micro_kernel_config->datatype_size)) );
       if ( i_xgemm_desc->prefetch == LIBXSMM_GEMM_PREFETCH_AL2          ||
          i_xgemm_desc->prefetch == LIBXSMM_GEMM_PREFETCH_AL2BL2_VIA_C    ) {
            libxsmm_x86_instruction_alu_imm( io_generated_code, i_micro_kernel_config->alu_sub_instruction,
            i_gp_reg_mapping->gp_reg_a_prefetch, ((i_xgemm_desc->m)*(i_micro_kernel_config->datatype_size)) );
      }
    }
  }
  libxsmm_x86_instruction_alu_imm( io_generated_code, i_micro_kernel_config->alu_cmp_instruction, i_gp_reg_mapping->gp_reg_nloop, i_n_done );
  libxsmm_x86_instruction_jump_back_to_label( io_generated_code, i_micro_kernel_config->alu_jmp_instruction, io_loop_label_tracker );
}


LIBXSMM_API_INTERN
void libxsmm_generator_gemm_footer_mloop_amx( libxsmm_generated_code*             io_generated_code,
    libxsmm_loop_label_tracker*        io_loop_label_tracker,
    const libxsmm_gp_reg_mapping*      i_gp_reg_mapping,
    const libxsmm_micro_kernel_config* i_micro_kernel_config,
    const libxsmm_gemm_descriptor*     i_xgemm_desc,
    const unsigned int                 i_m_blocking,
    const unsigned int                 i_m_done,
    const unsigned int                 i_k_unrolled ) {
  /* advance C pointer */
  if ( LIBXSMM_GEMM_PRECISION_BF16 == LIBXSMM_GETENUM_OUT( i_xgemm_desc->datatype ) ) {
    if (i_micro_kernel_config->vnni_format_C == 0) {
      libxsmm_x86_instruction_alu_imm( io_generated_code, i_micro_kernel_config->alu_add_instruction,
        i_gp_reg_mapping->gp_reg_c, i_m_blocking*(i_micro_kernel_config->datatype_size/2) );
    } else {
      libxsmm_x86_instruction_alu_imm( io_generated_code, i_micro_kernel_config->alu_add_instruction,
        i_gp_reg_mapping->gp_reg_c, i_m_blocking*2*(i_micro_kernel_config->datatype_size/2) );
    }
  } else if ( LIBXSMM_GEMM_PRECISION_I8 == LIBXSMM_GETENUM_OUT( i_xgemm_desc->datatype ) ) {
    libxsmm_x86_instruction_alu_imm( io_generated_code, i_micro_kernel_config->alu_add_instruction,
        i_gp_reg_mapping->gp_reg_c, i_m_blocking*(i_micro_kernel_config->datatype_size/4) );
  } else {
    libxsmm_x86_instruction_alu_imm( io_generated_code, i_micro_kernel_config->alu_add_instruction,
        i_gp_reg_mapping->gp_reg_c, i_m_blocking*(i_micro_kernel_config->datatype_size) );
  }

  /* Also adjust eltwise pointers  */
  if ((i_micro_kernel_config->fused_relu == 1) || (i_micro_kernel_config->fused_bcolbias == 1) || (i_micro_kernel_config->fused_scolbias == 1) || (i_micro_kernel_config->overwrite_C == 0)) {
    libxsmm_x86_instruction_push_reg( io_generated_code, i_gp_reg_mapping->gp_reg_help_0 );
  }

  if ((i_micro_kernel_config->fused_relu == 1) && (i_micro_kernel_config->overwrite_C == 1) ) {
    libxsmm_generator_gemm_getval_stack_var( io_generated_code, i_micro_kernel_config, LIBXSMM_GEMM_STACK_VAR_ELT_OUTPUT_PTR, i_gp_reg_mapping->gp_reg_help_0 );
    libxsmm_x86_instruction_alu_imm( io_generated_code, i_micro_kernel_config->alu_add_instruction, i_gp_reg_mapping->gp_reg_help_0, i_m_blocking/8 );
    libxsmm_generator_gemm_setval_stack_var( io_generated_code, i_micro_kernel_config, LIBXSMM_GEMM_STACK_VAR_ELT_OUTPUT_PTR, i_gp_reg_mapping->gp_reg_help_0 );
  }

  if (i_micro_kernel_config->overwrite_C == 0) {
    libxsmm_generator_gemm_getval_stack_var( io_generated_code, i_micro_kernel_config, LIBXSMM_GEMM_STACK_VAR_ELT_OUTPUT_PTR, i_gp_reg_mapping->gp_reg_help_0 );
    libxsmm_x86_instruction_alu_imm( io_generated_code, i_micro_kernel_config->alu_add_instruction, i_gp_reg_mapping->gp_reg_help_0, i_m_blocking*(i_micro_kernel_config->datatype_size/2) );
    libxsmm_generator_gemm_setval_stack_var( io_generated_code, i_micro_kernel_config, LIBXSMM_GEMM_STACK_VAR_ELT_OUTPUT_PTR, i_gp_reg_mapping->gp_reg_help_0 );
  }

  if (i_micro_kernel_config->fused_bcolbias == 1) {
    libxsmm_generator_gemm_getval_stack_var( io_generated_code, i_micro_kernel_config, LIBXSMM_GEMM_STACK_VAR_ELT_BIAS_PTR, i_gp_reg_mapping->gp_reg_help_0 );
    libxsmm_x86_instruction_alu_imm( io_generated_code, i_micro_kernel_config->alu_add_instruction, i_gp_reg_mapping->gp_reg_help_0, i_m_blocking * (i_micro_kernel_config->datatype_size/2) );
    libxsmm_generator_gemm_setval_stack_var( io_generated_code, i_micro_kernel_config, LIBXSMM_GEMM_STACK_VAR_ELT_BIAS_PTR, i_gp_reg_mapping->gp_reg_help_0 );
  }

  if (i_micro_kernel_config->fused_scolbias == 1) {
    libxsmm_generator_gemm_getval_stack_var( io_generated_code, i_micro_kernel_config, LIBXSMM_GEMM_STACK_VAR_ELT_BIAS_PTR, i_gp_reg_mapping->gp_reg_help_0 );
    libxsmm_x86_instruction_alu_imm( io_generated_code, i_micro_kernel_config->alu_add_instruction, i_gp_reg_mapping->gp_reg_help_0, i_m_blocking * i_micro_kernel_config->datatype_size );
    libxsmm_generator_gemm_setval_stack_var( io_generated_code, i_micro_kernel_config, LIBXSMM_GEMM_STACK_VAR_ELT_BIAS_PTR, i_gp_reg_mapping->gp_reg_help_0 );
  }

  if ((i_micro_kernel_config->fused_relu == 1) || (i_micro_kernel_config->fused_bcolbias == 1) || (i_micro_kernel_config->fused_scolbias == 1) || (i_micro_kernel_config->overwrite_C == 0)) {
    libxsmm_x86_instruction_pop_reg( io_generated_code, i_gp_reg_mapping->gp_reg_help_0 );
  }

  /* C prefetch */
#if 0
  if ( i_xgemm_desc->prefetch == LIBXSMM_GEMM_PREFETCH_CL2 ||
      i_xgemm_desc->prefetch == LIBXSMM_GEMM_PREFETCH_AL2CL2BL2_VIA_C ) {
    libxsmm_x86_instruction_alu_imm( io_generated_code, i_micro_kernel_config->alu_add_instruction,
        i_gp_reg_mapping->gp_reg_c_prefetch, i_m_blocking*(i_micro_kernel_config->datatype_size) );

  }
#endif

  /* B prefetch */
  if ( i_xgemm_desc->prefetch == LIBXSMM_GEMM_PREFETCH_BL2_VIA_C ||
      i_xgemm_desc->prefetch == LIBXSMM_GEMM_PREFETCH_AL2BL2_VIA_C ||
      i_xgemm_desc->prefetch == LIBXSMM_GEMM_PREFETCH_AL2BL2_VIA_C_AHEAD) {
    if ( (i_xgemm_desc->flags & LIBXSMM_GEMM_FLAG_TRANS_B) == 0 ) {
      unsigned int l_type_scaling;
      if ( (LIBXSMM_GEMM_PRECISION_BF16 == LIBXSMM_GETENUM_INP( i_xgemm_desc->datatype )) ||
          (LIBXSMM_GEMM_PRECISION_I16  == LIBXSMM_GETENUM_INP( i_xgemm_desc->datatype ))    ) {
        l_type_scaling = 2;
      } else if ( LIBXSMM_GEMM_PRECISION_I8 == LIBXSMM_GETENUM_INP( i_xgemm_desc->datatype ) ) {
        l_type_scaling = 4;
      } else {
        l_type_scaling = 1;
      }
      libxsmm_x86_instruction_alu_imm( io_generated_code, i_micro_kernel_config->alu_add_instruction,
          i_gp_reg_mapping->gp_reg_b_prefetch, i_m_blocking*(i_micro_kernel_config->datatype_size/l_type_scaling) );
    }
  }

  if (i_k_unrolled == 0) {
    /* A prefetch */
    if ( i_xgemm_desc->prefetch == LIBXSMM_GEMM_PREFETCH_AL2 ||
        i_xgemm_desc->prefetch == LIBXSMM_GEMM_PREFETCH_AL2BL2_VIA_C) {
      if (i_xgemm_desc->flags & LIBXSMM_GEMM_FLAG_BATCH_REDUCE_ADDRESS) {
        if ( i_xgemm_desc->prefetch == LIBXSMM_GEMM_PREFETCH_AL2 ) {
          libxsmm_x86_instruction_push_reg( io_generated_code, i_gp_reg_mapping->gp_reg_help_0 );
          libxsmm_x86_instruction_push_reg( io_generated_code, i_gp_reg_mapping->gp_reg_reduce_loop );
          libxsmm_generator_gemm_header_reduceloop_amx( io_generated_code, io_loop_label_tracker, i_gp_reg_mapping, i_micro_kernel_config );
          libxsmm_x86_instruction_alu_mem( io_generated_code,
              i_micro_kernel_config->alu_mov_instruction,
              i_gp_reg_mapping->gp_reg_a_prefetch,
              i_gp_reg_mapping->gp_reg_reduce_loop, 8,
              0,
              i_gp_reg_mapping->gp_reg_help_0,
              0 );
          libxsmm_x86_instruction_alu_imm( io_generated_code, i_micro_kernel_config->alu_sub_instruction, i_gp_reg_mapping->gp_reg_help_0,
              ((i_xgemm_desc->k) * (i_micro_kernel_config->datatype_size) * (i_xgemm_desc->lda) ) -
              (i_m_blocking * (i_micro_kernel_config->datatype_size)) );
          libxsmm_x86_instruction_alu_mem( io_generated_code,
              i_micro_kernel_config->alu_mov_instruction,
              i_gp_reg_mapping->gp_reg_a_prefetch,
              i_gp_reg_mapping->gp_reg_reduce_loop, 8,
              0,
              i_gp_reg_mapping->gp_reg_help_0,
              1 );
          libxsmm_generator_gemm_footer_reduceloop_amx( io_generated_code, io_loop_label_tracker, i_gp_reg_mapping, i_micro_kernel_config, i_xgemm_desc);
          libxsmm_x86_instruction_pop_reg( io_generated_code, i_gp_reg_mapping->gp_reg_reduce_loop );
          libxsmm_x86_instruction_pop_reg( io_generated_code, i_gp_reg_mapping->gp_reg_help_0 );
        }
      } else if ((i_xgemm_desc->flags & LIBXSMM_GEMM_FLAG_BATCH_REDUCE_OFFSET) || (i_xgemm_desc->flags & LIBXSMM_GEMM_FLAG_BATCH_REDUCE_STRIDE)) {
        /* TODO: Add prefetching handling  */
      } else {
        libxsmm_x86_instruction_alu_imm( io_generated_code, i_micro_kernel_config->alu_sub_instruction, i_gp_reg_mapping->gp_reg_a_prefetch,
            ((i_xgemm_desc->k) * (i_micro_kernel_config->datatype_size) * (i_xgemm_desc->lda) ) -
            (i_m_blocking * (i_micro_kernel_config->datatype_size)) );
      }
    }
    /* advance A pointer */
    if (i_xgemm_desc->flags & LIBXSMM_GEMM_FLAG_BATCH_REDUCE_ADDRESS) {
      libxsmm_x86_instruction_push_reg( io_generated_code, i_gp_reg_mapping->gp_reg_help_0 );
      libxsmm_x86_instruction_push_reg( io_generated_code, i_gp_reg_mapping->gp_reg_reduce_loop );
      libxsmm_generator_gemm_header_reduceloop_amx( io_generated_code, io_loop_label_tracker, i_gp_reg_mapping, i_micro_kernel_config );
      libxsmm_x86_instruction_alu_mem( io_generated_code,
          i_micro_kernel_config->alu_mov_instruction,
          i_gp_reg_mapping->gp_reg_a,
          i_gp_reg_mapping->gp_reg_reduce_loop, 8,
          0,
          i_gp_reg_mapping->gp_reg_help_0,
          0 );
      libxsmm_x86_instruction_alu_imm( io_generated_code, i_micro_kernel_config->alu_sub_instruction, i_gp_reg_mapping->gp_reg_help_0,
          ((i_xgemm_desc->k) * (i_micro_kernel_config->datatype_size) * (i_xgemm_desc->lda) ) - (i_m_blocking * (i_micro_kernel_config->datatype_size)) );
      libxsmm_x86_instruction_alu_mem( io_generated_code,
          i_micro_kernel_config->alu_mov_instruction,
          i_gp_reg_mapping->gp_reg_a,
          i_gp_reg_mapping->gp_reg_reduce_loop, 8,
          0,
          i_gp_reg_mapping->gp_reg_help_0,
          1 );
      libxsmm_generator_gemm_footer_reduceloop_amx( io_generated_code, io_loop_label_tracker, i_gp_reg_mapping, i_micro_kernel_config, i_xgemm_desc);
      libxsmm_x86_instruction_pop_reg( io_generated_code, i_gp_reg_mapping->gp_reg_reduce_loop );
      libxsmm_x86_instruction_pop_reg( io_generated_code, i_gp_reg_mapping->gp_reg_help_0 );
    } else if ((i_xgemm_desc->flags & LIBXSMM_GEMM_FLAG_BATCH_REDUCE_OFFSET) || (i_xgemm_desc->flags & LIBXSMM_GEMM_FLAG_BATCH_REDUCE_STRIDE)) {
      libxsmm_x86_instruction_alu_imm( io_generated_code, i_micro_kernel_config->alu_sub_instruction, i_gp_reg_mapping->gp_reg_a_base,
          0 - (i_m_blocking * (i_micro_kernel_config->datatype_size)) );
    } else {
      libxsmm_x86_instruction_alu_imm( io_generated_code, i_micro_kernel_config->alu_sub_instruction, i_gp_reg_mapping->gp_reg_a,
          ((i_xgemm_desc->k) * (i_micro_kernel_config->datatype_size) * (i_xgemm_desc->lda) ) - (i_m_blocking * (i_micro_kernel_config->datatype_size)) );
    }
  } else {
    /* A prefetch */
    if ( i_xgemm_desc->prefetch == LIBXSMM_GEMM_PREFETCH_AL2 ||
        i_xgemm_desc->prefetch == LIBXSMM_GEMM_PREFETCH_AL2BL2_VIA_C) {
      if ( i_xgemm_desc->flags & LIBXSMM_GEMM_FLAG_BATCH_REDUCE_ADDRESS ) {
        libxsmm_x86_instruction_push_reg( io_generated_code, i_gp_reg_mapping->gp_reg_help_0 );
        libxsmm_x86_instruction_push_reg( io_generated_code, i_gp_reg_mapping->gp_reg_reduce_loop );
        libxsmm_generator_gemm_header_reduceloop_amx( io_generated_code, io_loop_label_tracker, i_gp_reg_mapping, i_micro_kernel_config );
        libxsmm_x86_instruction_alu_mem( io_generated_code,
            i_micro_kernel_config->alu_mov_instruction,
            i_gp_reg_mapping->gp_reg_a_prefetch,
            i_gp_reg_mapping->gp_reg_reduce_loop, 8,
            0,
            i_gp_reg_mapping->gp_reg_a,
            0 );
        libxsmm_x86_instruction_alu_imm( io_generated_code, i_micro_kernel_config->alu_add_instruction, i_gp_reg_mapping->gp_reg_a,
            (i_m_blocking * (i_micro_kernel_config->datatype_size)) );
        libxsmm_x86_instruction_alu_mem( io_generated_code,
            i_micro_kernel_config->alu_mov_instruction,
            i_gp_reg_mapping->gp_reg_a_prefetch,
            i_gp_reg_mapping->gp_reg_reduce_loop, 8,
            0,
            i_gp_reg_mapping->gp_reg_a,
            1 );
        libxsmm_generator_gemm_footer_reduceloop_amx( io_generated_code, io_loop_label_tracker, i_gp_reg_mapping, i_micro_kernel_config, i_xgemm_desc);
        libxsmm_x86_instruction_pop_reg( io_generated_code, i_gp_reg_mapping->gp_reg_reduce_loop );
        libxsmm_x86_instruction_pop_reg( io_generated_code, i_gp_reg_mapping->gp_reg_help_0 );
      } else if ((i_xgemm_desc->flags & LIBXSMM_GEMM_FLAG_BATCH_REDUCE_OFFSET) || (i_xgemm_desc->flags & LIBXSMM_GEMM_FLAG_BATCH_REDUCE_STRIDE)) {
        /* TODO: Add prefetching handling  */
      } else {
        libxsmm_x86_instruction_alu_imm( io_generated_code, i_micro_kernel_config->alu_add_instruction, i_gp_reg_mapping->gp_reg_a_prefetch,
            (i_m_blocking * (i_micro_kernel_config->datatype_size)) );
      }
    }

    if (i_xgemm_desc->flags & LIBXSMM_GEMM_FLAG_BATCH_REDUCE_ADDRESS) {
      libxsmm_x86_instruction_push_reg( io_generated_code, i_gp_reg_mapping->gp_reg_help_0 );
      libxsmm_x86_instruction_push_reg( io_generated_code, i_gp_reg_mapping->gp_reg_reduce_loop );
      libxsmm_generator_gemm_header_reduceloop_amx( io_generated_code, io_loop_label_tracker, i_gp_reg_mapping, i_micro_kernel_config );
      libxsmm_x86_instruction_alu_mem( io_generated_code,
          i_micro_kernel_config->alu_mov_instruction,
          i_gp_reg_mapping->gp_reg_a_ptrs,
          i_gp_reg_mapping->gp_reg_reduce_loop, 8,
          0,
          i_gp_reg_mapping->gp_reg_a,
          0 );
      libxsmm_x86_instruction_alu_imm( io_generated_code, i_micro_kernel_config->alu_add_instruction, i_gp_reg_mapping->gp_reg_a,
          (i_m_blocking * (i_micro_kernel_config->datatype_size)) );
      libxsmm_x86_instruction_alu_mem( io_generated_code,
          i_micro_kernel_config->alu_mov_instruction,
          i_gp_reg_mapping->gp_reg_a_ptrs,
          i_gp_reg_mapping->gp_reg_reduce_loop, 8,
          0,
          i_gp_reg_mapping->gp_reg_a,
          1 );
      libxsmm_generator_gemm_footer_reduceloop_amx( io_generated_code, io_loop_label_tracker, i_gp_reg_mapping, i_micro_kernel_config, i_xgemm_desc);
      libxsmm_x86_instruction_pop_reg( io_generated_code, i_gp_reg_mapping->gp_reg_reduce_loop );
      libxsmm_x86_instruction_pop_reg( io_generated_code, i_gp_reg_mapping->gp_reg_help_0 );
    } else if ((i_xgemm_desc->flags & LIBXSMM_GEMM_FLAG_BATCH_REDUCE_OFFSET) || (i_xgemm_desc->flags & LIBXSMM_GEMM_FLAG_BATCH_REDUCE_STRIDE)){
      /* advance A pointer */
      libxsmm_x86_instruction_alu_imm( io_generated_code, i_micro_kernel_config->alu_add_instruction, i_gp_reg_mapping->gp_reg_a_base,
          (i_m_blocking * (i_micro_kernel_config->datatype_size)) );
    } else {
      /* advance A pointer */
      libxsmm_x86_instruction_alu_imm( io_generated_code, i_micro_kernel_config->alu_add_instruction, i_gp_reg_mapping->gp_reg_a,
          (i_m_blocking * (i_micro_kernel_config->datatype_size)) );
    }
  }

  /* loop handling */
  libxsmm_x86_instruction_alu_imm( io_generated_code, i_micro_kernel_config->alu_cmp_instruction, i_gp_reg_mapping->gp_reg_mloop, i_m_done );
  libxsmm_x86_instruction_jump_back_to_label( io_generated_code, i_micro_kernel_config->alu_jmp_instruction, io_loop_label_tracker );
}


LIBXSMM_API_INTERN
void libxsmm_generator_gemm_load_C_amx( libxsmm_generated_code*            io_generated_code,
    libxsmm_gp_reg_mapping*      i_gp_reg_mapping,
    const libxsmm_micro_kernel_config* i_micro_kernel_config,
    const libxsmm_gemm_descriptor*     i_xgemm_desc,
    libxsmm_blocking_info_t*           n_blocking_info,
    libxsmm_blocking_info_t*           m_blocking_info ) {

  int im, in, acc_id = 0, i_n_offset, i_m_offset, zmm_reg = 0;
  int m_tiles = m_blocking_info->tiles;
  int n_tiles = n_blocking_info->tiles;
  unsigned int col = 0;
  unsigned int gp_reg_bias = (i_micro_kernel_config->m_loop_exists == 0) ? i_gp_reg_mapping->gp_reg_help_0 : i_gp_reg_mapping->gp_reg_help_1;

  if (0 == (LIBXSMM_GEMM_FLAG_BETA_0 & i_xgemm_desc->flags)) { /* Beta=1 */
    if (LIBXSMM_GEMM_PRECISION_BF16 == LIBXSMM_GETENUM_OUT( i_xgemm_desc->datatype )) {
      unsigned int gp_reg_gemm_scratch = (i_micro_kernel_config->n_loop_exists == 0) ? i_gp_reg_mapping->gp_reg_help_1 : i_gp_reg_mapping->gp_reg_help_0;
      /* Check if we have to save the tmp registers  */
      if ( (gp_reg_gemm_scratch == i_gp_reg_mapping->gp_reg_help_0) && (i_micro_kernel_config->m_loop_exists == 1)  ) {
        libxsmm_x86_instruction_push_reg( io_generated_code, i_gp_reg_mapping->gp_reg_help_0 );
      }

      libxsmm_generator_gemm_getval_stack_var( io_generated_code, i_micro_kernel_config, LIBXSMM_GEMM_STACK_VAR_GEMM_SCRATCH_PTR, gp_reg_gemm_scratch );

      i_m_offset = 0;
      for (im = 0; im < m_tiles; im++) {
        i_n_offset = 0;
        for (in = 0; in < n_tiles; in++) {
          /* Now for all the columns in the tile, upconvert them to F32 from BF16  */
          for (col = 0; col < n_blocking_info->sizes[in]; col++) {
            zmm_reg = (col % 4) + i_micro_kernel_config->reserved_zmms;  /* we do mod 4 as are otherwise running out ymms */
            /* load 16 bit values into ymm portion of the register */
            libxsmm_x86_instruction_vec_move( io_generated_code,
                i_micro_kernel_config->instruction_set,
                i_micro_kernel_config->c_vmove_instruction,
                i_gp_reg_mapping->gp_reg_c,
                LIBXSMM_X86_GP_REG_UNDEF, 0,
                ( ((i_n_offset+col) * i_xgemm_desc->ldc) + i_m_offset) * (i_micro_kernel_config->datatype_size/2),
                'y',
                zmm_reg, 0, 1, 0 );
            /* convert 16 bit values into 32 bit (integer convert) */
            libxsmm_x86_instruction_vec_compute_convert( io_generated_code,
                i_micro_kernel_config->instruction_set,
                LIBXSMM_X86_INSTR_VPMOVSXWD,
                i_micro_kernel_config->vector_name,
                zmm_reg, LIBXSMM_X86_VEC_REG_UNDEF,
                zmm_reg,
                LIBXSMM_X86_VEC_REG_UNDEF);
            /* shift 16 bits to the left to generate valid FP32 numbers */
            libxsmm_x86_instruction_vec_shuffle_reg(io_generated_code,
                i_micro_kernel_config->instruction_set,
                LIBXSMM_X86_INSTR_VPSLLD,
                i_micro_kernel_config->vector_name,
                zmm_reg,
                zmm_reg,
                LIBXSMM_X86_VEC_REG_UNDEF,
                16);
            /* Store upconverted column to GEMM scratch */
            libxsmm_x86_instruction_vec_move( io_generated_code,
                i_micro_kernel_config->instruction_set,
                LIBXSMM_X86_INSTR_VMOVUPS,
                gp_reg_gemm_scratch,
                LIBXSMM_X86_GP_REG_UNDEF, 0,
                ((i_n_offset+col) * i_xgemm_desc->ldc + i_m_offset) * i_micro_kernel_config->datatype_size,
                i_micro_kernel_config->vector_name,
                zmm_reg, 0, 1, 1 );
          }
          /* Move zmm registers stored in GEMM scratch to the proper tile */
          libxsmm_x86_instruction_tile_move( io_generated_code,
              i_micro_kernel_config->instruction_set,
              LIBXSMM_X86_INSTR_TILELOADD,
              gp_reg_gemm_scratch,
              i_gp_reg_mapping->gp_reg_ldc,
              4,
              (i_n_offset * i_xgemm_desc->ldc + i_m_offset) * i_micro_kernel_config->datatype_size,
              im * n_tiles + in);

          i_n_offset += n_blocking_info->sizes[in];
        }
        i_m_offset += m_blocking_info->sizes[im];
      }
      /* Check if we have to restore the tmp registers  */
      if ( (gp_reg_gemm_scratch == i_gp_reg_mapping->gp_reg_help_0) && (i_micro_kernel_config->m_loop_exists == 1)  ) {
        libxsmm_x86_instruction_pop_reg( io_generated_code, i_gp_reg_mapping->gp_reg_help_0 );
      }
    } else {
      i_m_offset = 0;
      for (im = 0; im < m_tiles; im++) {
        i_n_offset = 0;
        for (in = 0; in < n_tiles; in++) {
          libxsmm_x86_instruction_tile_move( io_generated_code,
              i_micro_kernel_config->instruction_set,
              LIBXSMM_X86_INSTR_TILELOADD,
              i_gp_reg_mapping->gp_reg_c,
              i_gp_reg_mapping->gp_reg_ldc,
              4,
              (i_n_offset * i_xgemm_desc->ldc + i_m_offset) * i_micro_kernel_config->datatype_size,
              acc_id);

          acc_id++;
          if (n_tiles == 1) {
            acc_id++;
          }
          i_n_offset += n_blocking_info->sizes[in];
        }
        i_m_offset += m_blocking_info->sizes[im];
      }
    }
  } else { /* Beta=0 */
    /* Check if we have to fuse colbias bcast */
    if ((i_micro_kernel_config->fused_bcolbias == 1) || (i_micro_kernel_config->fused_scolbias == 1)) {

      if (i_micro_kernel_config->fused_scolbias == 1) {
        if ( (gp_reg_bias == i_gp_reg_mapping->gp_reg_help_1) && (i_micro_kernel_config->n_loop_exists == 1)  ) {
          libxsmm_x86_instruction_push_reg( io_generated_code, i_gp_reg_mapping->gp_reg_help_1 );
        }
        libxsmm_generator_gemm_getval_stack_var( io_generated_code, i_micro_kernel_config, LIBXSMM_GEMM_STACK_VAR_ELT_BIAS_PTR, gp_reg_bias );
        /* Set gp_reg_ldc to 0 in order to broadcast the bias */
        libxsmm_x86_instruction_alu_imm(io_generated_code, i_micro_kernel_config->alu_mov_instruction, i_gp_reg_mapping->gp_reg_ldc, 0);
        i_m_offset = 0;
        for (im = 0; im < m_tiles; im++) {
          i_n_offset = 0;
          for (in = 0; in < n_tiles; in++) {
            libxsmm_x86_instruction_tile_move( io_generated_code,
                i_micro_kernel_config->instruction_set,
                LIBXSMM_X86_INSTR_TILELOADD,
                gp_reg_bias,
                i_gp_reg_mapping->gp_reg_ldc,
                4,
                i_m_offset * i_micro_kernel_config->datatype_size,
                acc_id);

            acc_id++;
            i_n_offset += n_blocking_info->sizes[in];
          }
          i_m_offset += m_blocking_info->sizes[im];
        }

        if ( (gp_reg_bias == i_gp_reg_mapping->gp_reg_help_1) && (i_micro_kernel_config->n_loop_exists == 1)  ) {
          libxsmm_x86_instruction_pop_reg( io_generated_code, i_gp_reg_mapping->gp_reg_help_1 );
        }
        /* Restore gp_reg_ldc to proper value */
        libxsmm_x86_instruction_alu_imm(io_generated_code, i_micro_kernel_config->alu_mov_instruction, i_gp_reg_mapping->gp_reg_ldc, (i_xgemm_desc->ldc * i_micro_kernel_config->datatype_size)/4);

      } else if (i_micro_kernel_config->fused_bcolbias == 1) {

        unsigned int gp_reg_gemm_scratch = (i_micro_kernel_config->m_loop_exists == 0) ? i_gp_reg_mapping->gp_reg_help_1 : i_gp_reg_mapping->gp_reg_help_0;

        if ( (gp_reg_bias == i_gp_reg_mapping->gp_reg_help_1) && (i_micro_kernel_config->n_loop_exists == 1)  ) {
          libxsmm_x86_instruction_push_reg( io_generated_code, i_gp_reg_mapping->gp_reg_help_1 );
        }
        if ( (gp_reg_bias == i_gp_reg_mapping->gp_reg_help_0) && (i_micro_kernel_config->m_loop_exists == 1)  ) {
          libxsmm_x86_instruction_push_reg( io_generated_code, i_gp_reg_mapping->gp_reg_help_0 );
        }
        if ( (gp_reg_gemm_scratch == i_gp_reg_mapping->gp_reg_help_1) && (i_micro_kernel_config->n_loop_exists == 1)  ) {
          libxsmm_x86_instruction_push_reg( io_generated_code, i_gp_reg_mapping->gp_reg_help_1 );
        }
        if ( (gp_reg_gemm_scratch == i_gp_reg_mapping->gp_reg_help_0) && (i_micro_kernel_config->m_loop_exists == 1)  ) {
          libxsmm_x86_instruction_push_reg( io_generated_code, i_gp_reg_mapping->gp_reg_help_0 );
        }

        libxsmm_generator_gemm_getval_stack_var( io_generated_code, i_micro_kernel_config, LIBXSMM_GEMM_STACK_VAR_ELT_BIAS_PTR, gp_reg_bias );
        libxsmm_generator_gemm_getval_stack_var( io_generated_code, i_micro_kernel_config, LIBXSMM_GEMM_STACK_VAR_GEMM_SCRATCH_PTR, gp_reg_gemm_scratch );

        /* Upconvert bf16 bias to GEMM scratch */
        i_m_offset = 0;
        for (im = 0; im < m_tiles; im++) {
          zmm_reg = (im % (16-i_micro_kernel_config->reserved_zmms)) + i_micro_kernel_config->reserved_zmms;
          /* load 16 bit values into ymm portion of the register */
          libxsmm_x86_instruction_vec_move( io_generated_code,
              i_micro_kernel_config->instruction_set,
              i_micro_kernel_config->c_vmove_instruction,
              gp_reg_bias,
              LIBXSMM_X86_GP_REG_UNDEF, 0,
              i_m_offset * (i_micro_kernel_config->datatype_size/2),
              'y',
              zmm_reg, 0, 1, 0 );
          /* convert 16 bit values into 32 bit (integer convert) */
          libxsmm_x86_instruction_vec_compute_convert( io_generated_code,
              i_micro_kernel_config->instruction_set,
              LIBXSMM_X86_INSTR_VPMOVSXWD,
              i_micro_kernel_config->vector_name,
              zmm_reg, LIBXSMM_X86_VEC_REG_UNDEF,
              zmm_reg,
              LIBXSMM_X86_VEC_REG_UNDEF);
          /* shift 16 bits to the left to generate valid FP32 numbers */
          libxsmm_x86_instruction_vec_shuffle_reg(io_generated_code,
              i_micro_kernel_config->instruction_set,
              LIBXSMM_X86_INSTR_VPSLLD,
              i_micro_kernel_config->vector_name,
              zmm_reg,
              zmm_reg,
              LIBXSMM_X86_VEC_REG_UNDEF,
              16);
          /* Store upconverted column to GEMM scratch */
          libxsmm_x86_instruction_vec_move( io_generated_code,
              i_micro_kernel_config->instruction_set,
              LIBXSMM_X86_INSTR_VMOVUPS,
              gp_reg_gemm_scratch,
              LIBXSMM_X86_GP_REG_UNDEF, 0,
              i_m_offset * i_micro_kernel_config->datatype_size,
              i_micro_kernel_config->vector_name,
              zmm_reg, 0, 1, 1 );

          i_m_offset += m_blocking_info->sizes[im];
        }

        /* Set gp_reg_ldc to 0 in order to broadcast the bias */
        libxsmm_x86_instruction_alu_imm(io_generated_code, i_micro_kernel_config->alu_mov_instruction, i_gp_reg_mapping->gp_reg_ldc, 0);
        i_m_offset = 0;
        for (im = 0; im < m_tiles; im++) {
          i_n_offset = 0;
          for (in = 0; in < n_tiles; in++) {
            libxsmm_x86_instruction_tile_move( io_generated_code,
                i_micro_kernel_config->instruction_set,
                LIBXSMM_X86_INSTR_TILELOADD,
                gp_reg_gemm_scratch,
                i_gp_reg_mapping->gp_reg_ldc,
                4,
                i_m_offset * i_micro_kernel_config->datatype_size,
                acc_id);

            acc_id++;
            i_n_offset += n_blocking_info->sizes[in];
          }
          i_m_offset += m_blocking_info->sizes[im];
        }

        if ( (gp_reg_gemm_scratch == i_gp_reg_mapping->gp_reg_help_1) && (i_micro_kernel_config->n_loop_exists == 1)  ) {
          libxsmm_x86_instruction_pop_reg( io_generated_code, i_gp_reg_mapping->gp_reg_help_1 );
        }
        if ( (gp_reg_gemm_scratch == i_gp_reg_mapping->gp_reg_help_0) && (i_micro_kernel_config->m_loop_exists == 1)  ) {
          libxsmm_x86_instruction_pop_reg( io_generated_code, i_gp_reg_mapping->gp_reg_help_0 );
        }
        if ( (gp_reg_bias == i_gp_reg_mapping->gp_reg_help_1) && (i_micro_kernel_config->n_loop_exists == 1)  ) {
          libxsmm_x86_instruction_pop_reg( io_generated_code, i_gp_reg_mapping->gp_reg_help_1 );
        }
        if ( (gp_reg_bias == i_gp_reg_mapping->gp_reg_help_0) && (i_micro_kernel_config->m_loop_exists == 1)  ) {
          libxsmm_x86_instruction_pop_reg( io_generated_code, i_gp_reg_mapping->gp_reg_help_0 );
        }

        /* Restore gp_reg_ldc to proper value */
        libxsmm_x86_instruction_alu_imm(io_generated_code, i_micro_kernel_config->alu_mov_instruction, i_gp_reg_mapping->gp_reg_ldc, (i_xgemm_desc->ldc * i_micro_kernel_config->datatype_size)/4);
      }

    } else {
      for (im = 0; im < m_tiles; im++) {
        for (in = 0; in < n_tiles; in++) {
          libxsmm_x86_instruction_tile_move( io_generated_code,
              i_micro_kernel_config->instruction_set,
              LIBXSMM_X86_INSTR_TILEZERO,
              LIBXSMM_X86_GP_REG_UNDEF,
              LIBXSMM_X86_GP_REG_UNDEF,
              0,
              0,
              acc_id);
          acc_id++;
          if (n_tiles == 1) {
            acc_id++;
          }
        }
      }
    }
  }
}


LIBXSMM_API_INTERN
void libxsmm_generator_gemm_store_C_amx( libxsmm_generated_code*            io_generated_code,
    libxsmm_gp_reg_mapping*            i_gp_reg_mapping,
    libxsmm_micro_kernel_config*       i_micro_kernel_config,
    const libxsmm_gemm_descriptor*     i_xgemm_desc,
    libxsmm_blocking_info_t*           n_blocking_info,
    libxsmm_blocking_info_t*           m_blocking_info ) {

  int m_tiles = m_blocking_info->tiles;
  int n_tiles = n_blocking_info->tiles;
  int _C_tile_done[4] = { 0 };
  int i, im, in;

  for (i = 0; i < m_tiles*n_tiles; i++) {
    im = i_micro_kernel_config->_im[i];
    in = i_micro_kernel_config->_in[i];
    _C_tile_done[i_micro_kernel_config->_C_tile_id[i]] = 1;
    if (i_micro_kernel_config->use_paired_tilestores == 1) {
      /* If mate C tile is also ready, then two paired tilestore  */
      if (_C_tile_done[i_micro_kernel_config->_C_tile_mate_id[i_micro_kernel_config->_C_tile_id[i]]] == 1) {
        int min_mate_C_id = (i_micro_kernel_config->_C_tile_id[i] < i_micro_kernel_config->_C_tile_mate_id[i_micro_kernel_config->_C_tile_id[i]]) ? i_micro_kernel_config->_C_tile_id[i] : i_micro_kernel_config->_C_tile_mate_id[i_micro_kernel_config->_C_tile_id[i]];
        int im_store = min_mate_C_id / n_tiles;
        int in_store = min_mate_C_id % n_tiles;
        paired_tilestore( io_generated_code,
            i_gp_reg_mapping,
            i_micro_kernel_config,
            i_xgemm_desc,
            min_mate_C_id,
            i_micro_kernel_config->_C_tile_mate_id[min_mate_C_id],
            i_micro_kernel_config->_im_offset_prefix_sums[im_store],
            i_micro_kernel_config->_in_offset_prefix_sums[in_store],
            n_blocking_info->sizes[in_store]);
      }
    } else {
      single_tilestore( io_generated_code,
          i_gp_reg_mapping,
          i_micro_kernel_config,
          i_xgemm_desc,
          i_micro_kernel_config->_C_tile_id[i],
          i_micro_kernel_config->_im_offset_prefix_sums[im],
          i_micro_kernel_config->_in_offset_prefix_sums[in],
          n_blocking_info->sizes[in]);
    }
  }
}

LIBXSMM_API_INTERN
void libxsmm_setup_tile( unsigned int tile_id, unsigned int n_rows, unsigned int n_cols, libxsmm_tile_config *tc) {
  switch (tile_id) {
    case 0:
      tc->tile0rowsb = (unsigned short)(n_rows * 4);
      tc->tile0cols  = (unsigned char)n_cols;
      break;
    case 1:
      tc->tile1rowsb = (unsigned short)(n_rows * 4);
      tc->tile1cols  = (unsigned char)n_cols;
      break;
    case 2:
      tc->tile2rowsb = (unsigned short)(n_rows * 4);
      tc->tile2cols  = (unsigned char)n_cols;
      break;
    case 3:
      tc->tile3rowsb = (unsigned short)(n_rows * 4);
      tc->tile3cols  = (unsigned char)n_cols;
      break;
    case 4:
      tc->tile4rowsb = (unsigned short)(n_rows * 4);
      tc->tile4cols  = (unsigned char)n_cols;
      break;
    case 5:
      tc->tile5rowsb = (unsigned short)(n_rows * 4);
      tc->tile5cols  = (unsigned char)n_cols;
      break;
    case 6:
      tc->tile6rowsb = (unsigned short)(n_rows * 4);
      tc->tile6cols  = (unsigned char)n_cols;
      break;
    case 7:
      tc->tile7rowsb = (unsigned short)(n_rows * 4);
      tc->tile7cols  = (unsigned char)n_cols;
      break;
    default:
      fprintf(stderr, "Invalid tile id in setp tile!!!\n");
      exit(-1);
  }
}

LIBXSMM_API_INTERN
void libxsmm_generator_gemm_amx_setup_fusion_infra( libxsmm_generated_code*            io_generated_code,
                                                    const libxsmm_gemm_descriptor*      i_xgemm_desc,
                                                    const libxsmm_gp_reg_mapping*       i_gp_reg_mapping,
                                                    libxsmm_micro_kernel_config*  i_micro_kernel_config ) {

  unsigned int temp_reg = LIBXSMM_X86_GP_REG_R10;
  unsigned int reserved_zmms      = 0;
  unsigned int reserved_mask_regs = 1;
  LIBXSMM_UNUSED(i_gp_reg_mapping);

  i_micro_kernel_config->fused_bcolbias = 0;
  i_micro_kernel_config->fused_scolbias = 0;
  i_micro_kernel_config->fused_relu     = 0;
  i_micro_kernel_config->fused_sigmoid  = 0;
  i_micro_kernel_config->overwrite_C    = 0;
  i_micro_kernel_config->vnni_format_C  = 0;

  /* TODO: Add support fror more fusions  */
  if (i_xgemm_desc->meltw_operation == LIBXSMM_MELTW_OPERATION_COLBIAS_ACT) {
    if ((libxsmm_get_meltw_cbiasact_flags((libxsmm_meltw_comp_cbiasact_flags)i_xgemm_desc->meltw_flags) & LIBXSMM_MELTW_FLAG_CBIASACT_OVERWRITE_C) > 0) {
      i_micro_kernel_config->overwrite_C = 1;
    }
    if ((libxsmm_get_meltw_cbiasact_flags((libxsmm_meltw_comp_cbiasact_flags)i_xgemm_desc->meltw_flags) & LIBXSMM_MELTW_FLAG_CBIASACT_ACT_RELU) > 0) {
      i_micro_kernel_config->fused_relu = 1;
    }
    if ((libxsmm_get_meltw_cbiasact_flags((libxsmm_meltw_comp_cbiasact_flags)i_xgemm_desc->meltw_flags) & LIBXSMM_MELTW_FLAG_CBIASACT_ACT_SIGM) > 0) {
      i_micro_kernel_config->fused_sigmoid = 1;
    }
    if ((libxsmm_get_meltw_cbiasact_flags((libxsmm_meltw_comp_cbiasact_flags)i_xgemm_desc->meltw_flags) & LIBXSMM_MELTW_FLAG_CBIASACT_COLBIAS) > 0) {
      if (i_xgemm_desc->meltw_datatype_aux == LIBXSMM_DATATYPE_BF16) {
        i_micro_kernel_config->fused_bcolbias = 1;
      }
      if (i_xgemm_desc->meltw_datatype_aux == LIBXSMM_DATATYPE_F32) {
        i_micro_kernel_config->fused_scolbias = 1;
      }
    }
  } else {
    i_micro_kernel_config->overwrite_C    = 1;
    i_micro_kernel_config->vnni_format_C  = ((i_xgemm_desc->flags & LIBXSMM_GEMM_FLAG_VNNI_C) > 0) ? 1 : 0;
  }

  /* Setup zmms to be reused throughout the kernel  */
  if (i_micro_kernel_config->fused_relu == 1) {
    i_micro_kernel_config->zero_reg = reserved_zmms;
    libxsmm_x86_instruction_vec_compute_reg( io_generated_code,
                                             i_micro_kernel_config->instruction_set,
                                             LIBXSMM_X86_INSTR_VPXORD,
                                             i_micro_kernel_config->vector_name,
                                             i_micro_kernel_config->zero_reg, i_micro_kernel_config->zero_reg, i_micro_kernel_config->zero_reg );
    reserved_zmms++;
  }

  if (i_micro_kernel_config->vnni_format_C == 1) {
    short vnni_perm_array[32] = {0, 16, 1, 17, 2, 18, 3, 19, 4, 20, 5, 21, 6, 22, 7, 23, 8, 24, 9, 25, 10, 26, 11, 27, 12, 28, 13, 29, 14, 30, 15, 31};
    i_micro_kernel_config->vnni_perm_reg = reserved_zmms;
    libxsmm_x86_instruction_full_vec_load_of_constants ( io_generated_code, (const unsigned char *) vnni_perm_array, "vnni_perm_array_", i_micro_kernel_config->vector_name, i_micro_kernel_config->vnni_perm_reg);
    reserved_zmms++;
  }

  if (i_micro_kernel_config->fused_sigmoid == 1) {
    float pade78_sigm_array[16] = { 2027025.0f, 270270.0f, 6930.0f, 36.0f, 945945.0f, 51975.0f,  630.0f, 4.97f, -4.97f,  1.0f, -1.0f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f };
    reserved_zmms       += 15;
    reserved_mask_regs  += 2;
    i_micro_kernel_config->vec_x2        = reserved_zmms - 1;
    i_micro_kernel_config->vec_nom       = reserved_zmms - 2;
    i_micro_kernel_config->vec_denom     = reserved_zmms - 3;
    i_micro_kernel_config->vec_c0        = reserved_zmms - 4;
    i_micro_kernel_config->vec_c1        = reserved_zmms - 5;
    i_micro_kernel_config->vec_c2        = reserved_zmms - 6;
    i_micro_kernel_config->vec_c3        = reserved_zmms - 7;
    i_micro_kernel_config->vec_c1_d      = reserved_zmms - 8;
    i_micro_kernel_config->vec_c2_d      = reserved_zmms - 9;
    i_micro_kernel_config->vec_c3_d      = reserved_zmms - 10;
    i_micro_kernel_config->vec_hi_bound  = reserved_zmms - 11;
    i_micro_kernel_config->vec_lo_bound  = reserved_zmms - 12;
    i_micro_kernel_config->vec_ones      = reserved_zmms - 13;
    i_micro_kernel_config->vec_neg_ones  = reserved_zmms - 14;
    i_micro_kernel_config->vec_halves    = reserved_zmms - 15;

    libxsmm_x86_instruction_full_vec_load_of_constants ( io_generated_code, (const unsigned char *) pade78_sigm_array, "pade78_sigm_array_", i_micro_kernel_config->vector_name, i_micro_kernel_config->vec_c0);
    libxsmm_generator_gemm_getval_stack_var( io_generated_code, i_micro_kernel_config, LIBXSMM_GEMM_STACK_VAR_GEMM_SCRATCH_PTR, temp_reg );
    libxsmm_x86_instruction_vec_move( io_generated_code,
        i_micro_kernel_config->instruction_set,
        LIBXSMM_X86_INSTR_VMOVUPS,
        temp_reg,
        LIBXSMM_X86_GP_REG_UNDEF, 0, 0,
        i_micro_kernel_config->vector_name,
        i_micro_kernel_config->vec_c0, 0, 1, 1 );

    libxsmm_x86_instruction_vec_move( io_generated_code, i_micro_kernel_config->instruction_set, LIBXSMM_X86_INSTR_VBROADCASTSS, temp_reg, LIBXSMM_X86_GP_REG_UNDEF, 0,
        0, i_micro_kernel_config->vector_name, i_micro_kernel_config->vec_c0, 0, 1, 0 );
    libxsmm_x86_instruction_vec_move( io_generated_code, i_micro_kernel_config->instruction_set, LIBXSMM_X86_INSTR_VBROADCASTSS, temp_reg, LIBXSMM_X86_GP_REG_UNDEF, 0,
        4, i_micro_kernel_config->vector_name, i_micro_kernel_config->vec_c1, 0, 1, 0 );
    libxsmm_x86_instruction_vec_move( io_generated_code, i_micro_kernel_config->instruction_set, LIBXSMM_X86_INSTR_VBROADCASTSS, temp_reg, LIBXSMM_X86_GP_REG_UNDEF, 0,
        8, i_micro_kernel_config->vector_name, i_micro_kernel_config->vec_c2, 0, 1, 0 );
    libxsmm_x86_instruction_vec_move( io_generated_code, i_micro_kernel_config->instruction_set, LIBXSMM_X86_INSTR_VBROADCASTSS, temp_reg, LIBXSMM_X86_GP_REG_UNDEF, 0,
        12, i_micro_kernel_config->vector_name, i_micro_kernel_config->vec_c3, 0, 1, 0 );
    libxsmm_x86_instruction_vec_move( io_generated_code, i_micro_kernel_config->instruction_set, LIBXSMM_X86_INSTR_VBROADCASTSS, temp_reg, LIBXSMM_X86_GP_REG_UNDEF, 0,
        16, i_micro_kernel_config->vector_name, i_micro_kernel_config->vec_c1_d, 0, 1, 0 );
    libxsmm_x86_instruction_vec_move( io_generated_code, i_micro_kernel_config->instruction_set, LIBXSMM_X86_INSTR_VBROADCASTSS, temp_reg, LIBXSMM_X86_GP_REG_UNDEF, 0,
        20, i_micro_kernel_config->vector_name, i_micro_kernel_config->vec_c2_d, 0, 1, 0 );
    libxsmm_x86_instruction_vec_move( io_generated_code, i_micro_kernel_config->instruction_set, LIBXSMM_X86_INSTR_VBROADCASTSS, temp_reg, LIBXSMM_X86_GP_REG_UNDEF, 0,
        24, i_micro_kernel_config->vector_name, i_micro_kernel_config->vec_c3_d, 0, 1, 0 );
    libxsmm_x86_instruction_vec_move( io_generated_code, i_micro_kernel_config->instruction_set, LIBXSMM_X86_INSTR_VBROADCASTSS, temp_reg, LIBXSMM_X86_GP_REG_UNDEF, 0,
        28, i_micro_kernel_config->vector_name, i_micro_kernel_config->vec_hi_bound, 0, 1, 0 );
    libxsmm_x86_instruction_vec_move( io_generated_code, i_micro_kernel_config->instruction_set, LIBXSMM_X86_INSTR_VBROADCASTSS, temp_reg, LIBXSMM_X86_GP_REG_UNDEF, 0,
        32, i_micro_kernel_config->vector_name, i_micro_kernel_config->vec_lo_bound, 0, 1, 0 );
    libxsmm_x86_instruction_vec_move( io_generated_code, i_micro_kernel_config->instruction_set, LIBXSMM_X86_INSTR_VBROADCASTSS, temp_reg, LIBXSMM_X86_GP_REG_UNDEF, 0,
        36, i_micro_kernel_config->vector_name, i_micro_kernel_config->vec_ones, 0, 1, 0 );
    libxsmm_x86_instruction_vec_move( io_generated_code, i_micro_kernel_config->instruction_set, LIBXSMM_X86_INSTR_VBROADCASTSS, temp_reg, LIBXSMM_X86_GP_REG_UNDEF, 0,
        40, i_micro_kernel_config->vector_name, i_micro_kernel_config->vec_neg_ones, 0, 1, 0 );
    libxsmm_x86_instruction_vec_move( io_generated_code, i_micro_kernel_config->instruction_set, LIBXSMM_X86_INSTR_VBROADCASTSS, temp_reg, LIBXSMM_X86_GP_REG_UNDEF, 0,
        44, i_micro_kernel_config->vector_name, i_micro_kernel_config->vec_halves, 0, 1, 0 );

    i_micro_kernel_config->mask_hi  = reserved_mask_regs - 1;
    i_micro_kernel_config->mask_lo  = reserved_mask_regs - 2;
  }

  i_micro_kernel_config->reserved_zmms      = reserved_zmms;
  i_micro_kernel_config->reserved_mask_regs = reserved_mask_regs;
}


LIBXSMM_API_INTERN
void libxsmm_generator_gemm_amx_setup_stack_frame( libxsmm_generated_code*            io_generated_code,
    const libxsmm_gemm_descriptor*      i_xgemm_desc,
    const libxsmm_gp_reg_mapping*       i_gp_reg_mapping,
    libxsmm_micro_kernel_config*        i_micro_kernel_config,
    int                                 m_tiles,
    int                                 n_tiles ) {

  int is_stride_brgemm  = ((i_xgemm_desc->flags & LIBXSMM_GEMM_FLAG_BATCH_REDUCE_STRIDE) > 0) ? 1 : 0;
  int is_offset_brgemm  = ((i_xgemm_desc->flags & LIBXSMM_GEMM_FLAG_BATCH_REDUCE_OFFSET) > 0) ? 1 : 0;
  int is_address_brgemm = ((i_xgemm_desc->flags & LIBXSMM_GEMM_FLAG_BATCH_REDUCE_ADDRESS) > 0) ? 1 : 0;
  int is_brgemm         = ((is_stride_brgemm == 1) || (is_offset_brgemm == 1) || (is_address_brgemm == 1)) ? 1 : 0;
  int has_scf           = ((LIBXSMM_GEMM_PRECISION_I8 == LIBXSMM_GETENUM_INP( i_xgemm_desc->datatype )) && (LIBXSMM_GEMM_PRECISION_I8 == LIBXSMM_GETENUM_OUT( i_xgemm_desc->datatype ))) ? 1 : 0;
  int has_A_pf_ptr      = (i_xgemm_desc->prefetch == LIBXSMM_GEMM_PREFETCH_AL2_AHEAD || i_xgemm_desc->prefetch == LIBXSMM_GEMM_PREFETCH_AL2BL2_VIA_C_AHEAD) ? 1 : 0;
  int has_B_pf_ptr      = (i_xgemm_desc->prefetch == LIBXSMM_GEMM_PREFETCH_BL2_VIA_C || i_xgemm_desc->prefetch == LIBXSMM_GEMM_PREFETCH_AL2BL2_VIA_C_AHEAD ||
      i_xgemm_desc->prefetch == LIBXSMM_GEMM_PREFETCH_AL2BL2_VIA_C || i_xgemm_desc->prefetch == LIBXSMM_PREFETCH_AL2CL2BL2_VIA_C ) ? 1 : 0;
  int has_colbias_act_fused = 0;
  int has_eltwise_fused     = 0;
  unsigned int eltwise_struct_ptr_reg = LIBXSMM_X86_GP_REG_R11;
  unsigned int temp_reg               = LIBXSMM_X86_GP_REG_R10;
  unsigned int gemm_scratch_size      = 0;
  unsigned int scratch_pad_size       = 0;
  LIBXSMM_UNUSED(i_gp_reg_mapping);
  LIBXSMM_UNUSED(m_tiles);

  if (i_xgemm_desc->meltw_operation == LIBXSMM_MELTW_OPERATION_COLBIAS_ACT) {
    if (libxsmm_get_meltw_cbiasact_flags((libxsmm_meltw_comp_cbiasact_flags)i_xgemm_desc->meltw_flags) != (unsigned int)LIBXSMM_MELTW_COMP_FLAG_CBIASACT_NONE) {
      has_colbias_act_fused = 1;
    }
  }

  has_eltwise_fused = ((has_colbias_act_fused == 1)) ? 1: 0;
  i_micro_kernel_config->fused_eltwise = has_eltwise_fused;

  libxsmm_x86_instruction_push_reg( io_generated_code, LIBXSMM_X86_GP_REG_RBP );
  libxsmm_x86_instruction_alu_reg( io_generated_code, i_micro_kernel_config->alu_mov_instruction, LIBXSMM_X86_GP_REG_RSP, LIBXSMM_X86_GP_REG_RBP);
  libxsmm_x86_instruction_alu_imm( io_generated_code, i_micro_kernel_config->alu_sub_instruction, LIBXSMM_X86_GP_REG_RSP, 64 );

  if (is_brgemm == 0) {
    /* GEMM (A, B, C, [scf, eltwise_struct, Apf, Bpf] */
    if (has_scf == 1) {
      libxsmm_generator_gemm_setval_stack_var( io_generated_code, i_micro_kernel_config, LIBXSMM_GEMM_STACK_VAR_INT8_SCF, LIBXSMM_X86_GP_REG_RCX );
      if (has_eltwise_fused == 1) {
        eltwise_struct_ptr_reg = LIBXSMM_X86_GP_REG_R8;
        if (has_A_pf_ptr == 1) {
          libxsmm_generator_gemm_setval_stack_var( io_generated_code, i_micro_kernel_config, LIBXSMM_GEMM_STACK_VAR_PFA_PTR, LIBXSMM_X86_GP_REG_R9 );
          if (has_B_pf_ptr == 1) {
            libxsmm_generator_gemm_getval_stack_var( io_generated_code, i_micro_kernel_config, LIBXSMM_GEMM_STACK_VAR_ARG_7, temp_reg );
            libxsmm_generator_gemm_setval_stack_var( io_generated_code, i_micro_kernel_config, LIBXSMM_GEMM_STACK_VAR_PFB_PTR, temp_reg );
          }
        } else {
          if (has_B_pf_ptr == 1) {
            libxsmm_generator_gemm_setval_stack_var( io_generated_code, i_micro_kernel_config, LIBXSMM_GEMM_STACK_VAR_PFB_PTR, LIBXSMM_X86_GP_REG_R9 );
          }
        }
      } else {
        if (has_A_pf_ptr == 1) {
          libxsmm_generator_gemm_setval_stack_var( io_generated_code, i_micro_kernel_config, LIBXSMM_GEMM_STACK_VAR_PFA_PTR, LIBXSMM_X86_GP_REG_R8 );
          if (has_B_pf_ptr == 1) {
            libxsmm_generator_gemm_setval_stack_var( io_generated_code, i_micro_kernel_config, LIBXSMM_GEMM_STACK_VAR_PFB_PTR, LIBXSMM_X86_GP_REG_R9 );
          }
        } else {
          if (has_B_pf_ptr == 1) {
            libxsmm_generator_gemm_setval_stack_var( io_generated_code, i_micro_kernel_config, LIBXSMM_GEMM_STACK_VAR_PFB_PTR, LIBXSMM_X86_GP_REG_R8 );
          }
        }
      }
    } else {
      if (has_eltwise_fused == 1) {
        eltwise_struct_ptr_reg = LIBXSMM_X86_GP_REG_RCX;
        if (has_A_pf_ptr == 1) {
          libxsmm_generator_gemm_setval_stack_var( io_generated_code, i_micro_kernel_config, LIBXSMM_GEMM_STACK_VAR_PFA_PTR, LIBXSMM_X86_GP_REG_R8 );
          if (has_B_pf_ptr == 1) {
            libxsmm_generator_gemm_setval_stack_var( io_generated_code, i_micro_kernel_config, LIBXSMM_GEMM_STACK_VAR_PFB_PTR, LIBXSMM_X86_GP_REG_R9 );
          }
        } else {
          if (has_B_pf_ptr == 1) {
            libxsmm_generator_gemm_setval_stack_var( io_generated_code, i_micro_kernel_config, LIBXSMM_GEMM_STACK_VAR_PFB_PTR, LIBXSMM_X86_GP_REG_R8 );
          }
        }
      } else {
        if (has_A_pf_ptr == 1) {
          libxsmm_generator_gemm_setval_stack_var( io_generated_code, i_micro_kernel_config, LIBXSMM_GEMM_STACK_VAR_PFA_PTR, LIBXSMM_X86_GP_REG_RCX );
          if (has_B_pf_ptr == 1) {
            libxsmm_generator_gemm_setval_stack_var( io_generated_code, i_micro_kernel_config, LIBXSMM_GEMM_STACK_VAR_PFB_PTR, LIBXSMM_X86_GP_REG_R8 );
          }
        } else {
          if (has_B_pf_ptr == 1) {
            libxsmm_generator_gemm_setval_stack_var( io_generated_code, i_micro_kernel_config, LIBXSMM_GEMM_STACK_VAR_PFB_PTR, LIBXSMM_X86_GP_REG_RCX );
          }
        }
      }
    }
  } else {
    if ((is_stride_brgemm == 1) || (is_address_brgemm == 1)) {
      /* BRGEMM_ADDR/STRIDE (A, B, C, cnt, [scf, eltwise_struct, Apf, Bpf] */
      if (has_scf == 1) {
        libxsmm_generator_gemm_setval_stack_var( io_generated_code, i_micro_kernel_config, LIBXSMM_GEMM_STACK_VAR_INT8_SCF, LIBXSMM_X86_GP_REG_R8 );
        if (has_eltwise_fused == 1) {
          eltwise_struct_ptr_reg = LIBXSMM_X86_GP_REG_R9;
          if (has_A_pf_ptr == 1) {
            libxsmm_generator_gemm_getval_stack_var( io_generated_code, i_micro_kernel_config, LIBXSMM_GEMM_STACK_VAR_ARG_7, temp_reg );
            libxsmm_generator_gemm_setval_stack_var( io_generated_code, i_micro_kernel_config, LIBXSMM_GEMM_STACK_VAR_PFA_PTR, temp_reg );
            if (has_B_pf_ptr == 1) {
              libxsmm_generator_gemm_getval_stack_var( io_generated_code, i_micro_kernel_config, LIBXSMM_GEMM_STACK_VAR_ARG_8, temp_reg );
              libxsmm_generator_gemm_setval_stack_var( io_generated_code, i_micro_kernel_config, LIBXSMM_GEMM_STACK_VAR_PFB_PTR, temp_reg );
            }
          } else {
            if (has_B_pf_ptr == 1) {
              libxsmm_generator_gemm_getval_stack_var( io_generated_code, i_micro_kernel_config, LIBXSMM_GEMM_STACK_VAR_ARG_7, temp_reg );
              libxsmm_generator_gemm_setval_stack_var( io_generated_code, i_micro_kernel_config, LIBXSMM_GEMM_STACK_VAR_PFB_PTR, temp_reg );
            }
          }
        } else {
          if (has_A_pf_ptr == 1) {
            libxsmm_generator_gemm_setval_stack_var( io_generated_code, i_micro_kernel_config, LIBXSMM_GEMM_STACK_VAR_PFA_PTR, LIBXSMM_X86_GP_REG_R9 );
            if (has_B_pf_ptr == 1) {
              libxsmm_generator_gemm_getval_stack_var( io_generated_code, i_micro_kernel_config, LIBXSMM_GEMM_STACK_VAR_ARG_7, temp_reg );
              libxsmm_generator_gemm_setval_stack_var( io_generated_code, i_micro_kernel_config, LIBXSMM_GEMM_STACK_VAR_PFB_PTR, temp_reg );
            }
          } else {
            if (has_B_pf_ptr == 1) {
              libxsmm_generator_gemm_setval_stack_var( io_generated_code, i_micro_kernel_config, LIBXSMM_GEMM_STACK_VAR_PFB_PTR, LIBXSMM_X86_GP_REG_R9 );
            }
          }
        }
      } else {
        if (has_eltwise_fused == 1) {
          eltwise_struct_ptr_reg = LIBXSMM_X86_GP_REG_R8;
          if (has_A_pf_ptr == 1) {
            libxsmm_generator_gemm_setval_stack_var( io_generated_code, i_micro_kernel_config, LIBXSMM_GEMM_STACK_VAR_PFA_PTR, LIBXSMM_X86_GP_REG_R9 );
            if (has_B_pf_ptr == 1) {
              libxsmm_generator_gemm_getval_stack_var( io_generated_code, i_micro_kernel_config, LIBXSMM_GEMM_STACK_VAR_ARG_7, temp_reg );
              libxsmm_generator_gemm_setval_stack_var( io_generated_code, i_micro_kernel_config, LIBXSMM_GEMM_STACK_VAR_PFB_PTR, temp_reg );
            }
          } else {
            if (has_B_pf_ptr == 1) {
              libxsmm_generator_gemm_setval_stack_var( io_generated_code, i_micro_kernel_config, LIBXSMM_GEMM_STACK_VAR_PFB_PTR, LIBXSMM_X86_GP_REG_R9 );
            }
          }
        } else {
          if (has_A_pf_ptr == 1) {
            libxsmm_generator_gemm_setval_stack_var( io_generated_code, i_micro_kernel_config, LIBXSMM_GEMM_STACK_VAR_PFA_PTR, LIBXSMM_X86_GP_REG_R8 );
            if (has_B_pf_ptr == 1) {
              libxsmm_generator_gemm_setval_stack_var( io_generated_code, i_micro_kernel_config, LIBXSMM_GEMM_STACK_VAR_PFB_PTR, LIBXSMM_X86_GP_REG_R9 );
            }
          } else {
            if (has_B_pf_ptr == 1) {
              libxsmm_generator_gemm_setval_stack_var( io_generated_code, i_micro_kernel_config, LIBXSMM_GEMM_STACK_VAR_PFB_PTR, LIBXSMM_X86_GP_REG_R8 );
            }
          }
        }
      }
    } else {
      /* BRGEMM_OFFS (A, B, C, cnt, A_off, B_off, [scf, eltwise struct, Apf, Bpf] */
      libxsmm_generator_gemm_setval_stack_var( io_generated_code, i_micro_kernel_config, LIBXSMM_GEMM_STACK_VAR_A_OFFS_BRGEMM_PTR, LIBXSMM_X86_GP_REG_R8 );
      libxsmm_generator_gemm_setval_stack_var( io_generated_code, i_micro_kernel_config, LIBXSMM_GEMM_STACK_VAR_B_OFFS_BRGEMM_PTR, LIBXSMM_X86_GP_REG_R9 );
      if (has_scf == 1) {
        libxsmm_generator_gemm_getval_stack_var( io_generated_code, i_micro_kernel_config, LIBXSMM_GEMM_STACK_VAR_ARG_7, temp_reg );
        libxsmm_generator_gemm_setval_stack_var( io_generated_code, i_micro_kernel_config, LIBXSMM_GEMM_STACK_VAR_INT8_SCF, temp_reg );
        if (has_eltwise_fused == 1) {
          libxsmm_generator_gemm_getval_stack_var( io_generated_code, i_micro_kernel_config, LIBXSMM_GEMM_STACK_VAR_ARG_8, eltwise_struct_ptr_reg );
          if (has_A_pf_ptr == 1) {
            libxsmm_generator_gemm_getval_stack_var( io_generated_code, i_micro_kernel_config, LIBXSMM_GEMM_STACK_VAR_ARG_9, temp_reg );
            libxsmm_generator_gemm_setval_stack_var( io_generated_code, i_micro_kernel_config, LIBXSMM_GEMM_STACK_VAR_PFA_PTR, temp_reg );
            if (has_B_pf_ptr == 1) {
              libxsmm_generator_gemm_getval_stack_var( io_generated_code, i_micro_kernel_config, LIBXSMM_GEMM_STACK_VAR_ARG_10, temp_reg );
              libxsmm_generator_gemm_setval_stack_var( io_generated_code, i_micro_kernel_config, LIBXSMM_GEMM_STACK_VAR_PFB_PTR, temp_reg );
            }
          } else {
            if (has_B_pf_ptr == 1) {
              libxsmm_generator_gemm_getval_stack_var( io_generated_code, i_micro_kernel_config, LIBXSMM_GEMM_STACK_VAR_ARG_9, temp_reg );
              libxsmm_generator_gemm_setval_stack_var( io_generated_code, i_micro_kernel_config, LIBXSMM_GEMM_STACK_VAR_PFB_PTR, temp_reg );
            }
          }
        } else {
          if (has_A_pf_ptr == 1) {
            libxsmm_generator_gemm_getval_stack_var( io_generated_code, i_micro_kernel_config, LIBXSMM_GEMM_STACK_VAR_ARG_8, temp_reg );
            libxsmm_generator_gemm_setval_stack_var( io_generated_code, i_micro_kernel_config, LIBXSMM_GEMM_STACK_VAR_PFA_PTR, temp_reg );
            if (has_B_pf_ptr == 1) {
              libxsmm_generator_gemm_getval_stack_var( io_generated_code, i_micro_kernel_config, LIBXSMM_GEMM_STACK_VAR_ARG_9, temp_reg );
              libxsmm_generator_gemm_setval_stack_var( io_generated_code, i_micro_kernel_config, LIBXSMM_GEMM_STACK_VAR_PFB_PTR, temp_reg );
            }
          } else {
            if (has_B_pf_ptr == 1) {
              libxsmm_generator_gemm_getval_stack_var( io_generated_code, i_micro_kernel_config, LIBXSMM_GEMM_STACK_VAR_ARG_8, temp_reg );
              libxsmm_generator_gemm_setval_stack_var( io_generated_code, i_micro_kernel_config, LIBXSMM_GEMM_STACK_VAR_PFB_PTR, temp_reg );
            }
          }
        }
      } else {
        if (has_eltwise_fused == 1) {
          libxsmm_generator_gemm_getval_stack_var( io_generated_code, i_micro_kernel_config, LIBXSMM_GEMM_STACK_VAR_ARG_7, eltwise_struct_ptr_reg );
          if (has_A_pf_ptr == 1) {
            libxsmm_generator_gemm_getval_stack_var( io_generated_code, i_micro_kernel_config, LIBXSMM_GEMM_STACK_VAR_ARG_8, temp_reg );
            libxsmm_generator_gemm_setval_stack_var( io_generated_code, i_micro_kernel_config, LIBXSMM_GEMM_STACK_VAR_PFA_PTR, temp_reg );
            if (has_B_pf_ptr == 1) {
              libxsmm_generator_gemm_getval_stack_var( io_generated_code, i_micro_kernel_config, LIBXSMM_GEMM_STACK_VAR_ARG_9, temp_reg );
              libxsmm_generator_gemm_setval_stack_var( io_generated_code, i_micro_kernel_config, LIBXSMM_GEMM_STACK_VAR_PFB_PTR, temp_reg );
            }
          } else {
            if (has_B_pf_ptr == 1) {
              libxsmm_generator_gemm_getval_stack_var( io_generated_code, i_micro_kernel_config, LIBXSMM_GEMM_STACK_VAR_ARG_8, temp_reg );
              libxsmm_generator_gemm_setval_stack_var( io_generated_code, i_micro_kernel_config, LIBXSMM_GEMM_STACK_VAR_PFB_PTR, temp_reg );
            }
          }
        } else {
          if (has_A_pf_ptr == 1) {
            libxsmm_generator_gemm_getval_stack_var( io_generated_code, i_micro_kernel_config, LIBXSMM_GEMM_STACK_VAR_ARG_7, temp_reg );
            libxsmm_generator_gemm_setval_stack_var( io_generated_code, i_micro_kernel_config, LIBXSMM_GEMM_STACK_VAR_PFA_PTR, temp_reg );
            if (has_B_pf_ptr == 1) {
              libxsmm_generator_gemm_getval_stack_var( io_generated_code, i_micro_kernel_config, LIBXSMM_GEMM_STACK_VAR_ARG_8, temp_reg );
              libxsmm_generator_gemm_setval_stack_var( io_generated_code, i_micro_kernel_config, LIBXSMM_GEMM_STACK_VAR_PFB_PTR, temp_reg );
            }
          } else {
            if (has_B_pf_ptr == 1) {
              libxsmm_generator_gemm_getval_stack_var( io_generated_code, i_micro_kernel_config, LIBXSMM_GEMM_STACK_VAR_ARG_7, temp_reg );
              libxsmm_generator_gemm_setval_stack_var( io_generated_code, i_micro_kernel_config, LIBXSMM_GEMM_STACK_VAR_PFB_PTR, temp_reg );
            }
          }
        }
      }
    }
  }

  if (has_eltwise_fused == 1) {
    if (has_colbias_act_fused == 1) {
      /* TODO: Optimize this copy to operate only in used fileds form struct... */
      libxsmm_x86_instruction_alu_mem( io_generated_code, i_micro_kernel_config->alu_mov_instruction, eltwise_struct_ptr_reg, LIBXSMM_X86_GP_REG_UNDEF, 0,   0, temp_reg, 0 );
      libxsmm_generator_gemm_setval_stack_var( io_generated_code, i_micro_kernel_config, LIBXSMM_GEMM_STACK_VAR_ELT_BIAS_PTR, temp_reg );
      libxsmm_x86_instruction_alu_mem( io_generated_code, i_micro_kernel_config->alu_mov_instruction, eltwise_struct_ptr_reg, LIBXSMM_X86_GP_REG_UNDEF, 0,   8, temp_reg, 0 );
      libxsmm_generator_gemm_setval_stack_var( io_generated_code, i_micro_kernel_config, LIBXSMM_GEMM_STACK_VAR_ELT_OUTPUT_PTR, temp_reg );
    }
  }

  /* The stack now looks like this:
   *      10th param (if applicable)                <-- RBP+40
   *      9th param (if applicable)                 <-- RBP+32
   *      8th param (if applicable)                 <-- RBP+24
   *      7th param (if applicable)                 <-- RBP+16
   *      Return address                            <-- RBP+8
   *      Entry/saved RBP                           <-- RBP
   *      prefetch A ptr                            <-- RBP-8
   *      prefetch B ptr                            <-- RBP-16
   *      Offset A array ptr                        <-- RBP-24
   *      Offset B array ptr                        <-- RBP-32
   *      Int8 scaling factor                       <-- RBP-40
   *      GEMM_scratch ptr in stack (to be filled)  <-- RBP-48
   *      Eltwise bias ptr                          <-- RBP-56
   *      Eltwise output_ptr                        <-- RBP-64, RSP
   *
   */

  /* Now align RSP to 64 byte boundary  */
  libxsmm_x86_instruction_alu_imm_i64( io_generated_code, i_micro_kernel_config->alu_mov_instruction, temp_reg, 0xFFFFFFFFFFFFFFC0 );
  libxsmm_x86_instruction_alu_reg( io_generated_code, LIBXSMM_X86_INSTR_ANDQ, temp_reg, LIBXSMM_X86_GP_REG_RSP);

  /* Now alllocate in stack required GEMM scratch if necessary*/
  if (LIBXSMM_GEMM_PRECISION_F32 != LIBXSMM_GETENUM_OUT( i_xgemm_desc->datatype )) {
    int expand_scratch_factor = (n_tiles == 1) ? 2 : 1;
    gemm_scratch_size = expand_scratch_factor * i_xgemm_desc->n * i_xgemm_desc->ldc * i_micro_kernel_config->datatype_size;
    scratch_pad_size  = (gemm_scratch_size % 64 == 0) ? 0 : ((gemm_scratch_size + 63)/64) * 64 - gemm_scratch_size;
    gemm_scratch_size += scratch_pad_size;
  }

  if (gemm_scratch_size > 0) {
    libxsmm_x86_instruction_alu_imm( io_generated_code, i_micro_kernel_config->alu_sub_instruction, LIBXSMM_X86_GP_REG_RSP, gemm_scratch_size );
    libxsmm_generator_gemm_setval_stack_var( io_generated_code, i_micro_kernel_config, LIBXSMM_GEMM_STACK_VAR_GEMM_SCRATCH_PTR, LIBXSMM_X86_GP_REG_RSP );
  }

  /* Now push to RSP the callee-save registers  */
  libxsmm_x86_instruction_push_reg( io_generated_code, LIBXSMM_X86_GP_REG_RBX );
  libxsmm_x86_instruction_push_reg( io_generated_code, LIBXSMM_X86_GP_REG_R12 );
  libxsmm_x86_instruction_push_reg( io_generated_code, LIBXSMM_X86_GP_REG_R13 );
  libxsmm_x86_instruction_push_reg( io_generated_code, LIBXSMM_X86_GP_REG_R14 );
  libxsmm_x86_instruction_push_reg( io_generated_code, LIBXSMM_X86_GP_REG_R15 );

  /* The stack at exit of setup looks like this:
   *
   *      10th param (if applicable)            <-- RBP+40
   *      9th param (if applicable)             <-- RBP+32
   *      8th param (if applicable)             <-- RBP+24
   *      7th param (if applicable)             <-- RBP+16
   *      Return address                        <-- RBP+8
   *      Entry/saved RBP                       <-- RBP
   *      prefetch A ptr                        <-- RBP-8
   *      prefetch B ptr                        <-- RBP-16
   *      Offset A array ptr                    <-- RBP-24
   *      Offset B array ptr                    <-- RBP-32
   *      Int8 scaling factor                   <-- RBP-40
   *      GEMM_scratch ptr in stack             <-- RBP-48
   *      Eltwise bias ptr                      <-- RBP-56
   *      Eltwise output_ptr                    <-- RBP-64
   *      [ Potentianl  pad for 64b align ]
   *      GEMM scratch, 64b aligned             <-- (RBP-48) contains this address
   *      Callee-saved registers                <-- RSP
   *
   */

}


LIBXSMM_API_INTERN
void libxsmm_generator_gemm_amx_destroy_stack_frame( libxsmm_generated_code*            io_generated_code,
    const libxsmm_gemm_descriptor*      i_xgemm_desc,
    const libxsmm_gp_reg_mapping*       i_gp_reg_mapping,
    const libxsmm_micro_kernel_config*  i_micro_kernel_config ) {
  LIBXSMM_UNUSED(i_xgemm_desc);
  LIBXSMM_UNUSED(i_gp_reg_mapping);

  libxsmm_x86_instruction_pop_reg( io_generated_code, LIBXSMM_X86_GP_REG_R15 );
  libxsmm_x86_instruction_pop_reg( io_generated_code, LIBXSMM_X86_GP_REG_R14 );
  libxsmm_x86_instruction_pop_reg( io_generated_code, LIBXSMM_X86_GP_REG_R13 );
  libxsmm_x86_instruction_pop_reg( io_generated_code, LIBXSMM_X86_GP_REG_R12 );
  libxsmm_x86_instruction_pop_reg( io_generated_code, LIBXSMM_X86_GP_REG_RBX );
  libxsmm_x86_instruction_alu_reg( io_generated_code, i_micro_kernel_config->alu_mov_instruction, LIBXSMM_X86_GP_REG_RBP, LIBXSMM_X86_GP_REG_RSP);
  libxsmm_x86_instruction_pop_reg( io_generated_code, LIBXSMM_X86_GP_REG_RBP );
}

LIBXSMM_API_INTERN
void libxsmm_generator_gemm_amx_kernel( libxsmm_generated_code* io_generated_code, const libxsmm_gemm_descriptor* i_xgemm_desc ) {
  libxsmm_micro_kernel_config l_micro_kernel_config;
  libxsmm_loop_label_tracker l_loop_label_tracker;
  libxsmm_gp_reg_mapping l_gp_reg_mapping;

  /* AMX specific blocking info */
  libxsmm_blocking_info_t m_blocking_info[2], n_blocking_info[2];
  unsigned int m_blocking, n_blocking, k_blocking, ii = 0, m_tiles, n_tiles, im, in;
  libxsmm_tile_config tile_config;

  /* define gp register mapping */
  libxsmm_reset_x86_gp_reg_mapping( &l_gp_reg_mapping );
#if defined(_WIN32) || defined(__CYGWIN__)
#else
  l_gp_reg_mapping.gp_reg_a = LIBXSMM_X86_GP_REG_RDI;
  l_gp_reg_mapping.gp_reg_b = LIBXSMM_X86_GP_REG_RSI;
  l_gp_reg_mapping.gp_reg_c = LIBXSMM_X86_GP_REG_RDX;
  l_gp_reg_mapping.gp_reg_a_prefetch = LIBXSMM_X86_GP_REG_RCX;
  l_gp_reg_mapping.gp_reg_b_prefetch = LIBXSMM_X86_GP_REG_R8;
  /* If we are generating the batchreduce kernel, then we rename the registers  */
  if (i_xgemm_desc->flags & LIBXSMM_GEMM_FLAG_BATCH_REDUCE_ADDRESS) {
    l_gp_reg_mapping.gp_reg_a = LIBXSMM_X86_GP_REG_RAX;
    l_gp_reg_mapping.gp_reg_a_ptrs = LIBXSMM_X86_GP_REG_RDI;
    l_gp_reg_mapping.gp_reg_b = LIBXSMM_X86_GP_REG_RBX;
    l_gp_reg_mapping.gp_reg_b_ptrs = LIBXSMM_X86_GP_REG_RSI;
    l_gp_reg_mapping.gp_reg_c = LIBXSMM_X86_GP_REG_RDX;
    l_gp_reg_mapping.gp_reg_reduce_count = LIBXSMM_X86_GP_REG_RCX;
    l_gp_reg_mapping.gp_reg_a_prefetch = LIBXSMM_X86_GP_REG_R8;
    l_gp_reg_mapping.gp_reg_b_prefetch = LIBXSMM_X86_GP_REG_R9;
    l_gp_reg_mapping.gp_reg_reduce_loop = LIBXSMM_X86_GP_REG_R10;
  } else if (i_xgemm_desc->flags & LIBXSMM_GEMM_FLAG_BATCH_REDUCE_OFFSET) {
    l_gp_reg_mapping.gp_reg_a_base = LIBXSMM_X86_GP_REG_RDI;
    l_gp_reg_mapping.gp_reg_a_offset = LIBXSMM_X86_GP_REG_R8;
    l_gp_reg_mapping.gp_reg_a = LIBXSMM_X86_GP_REG_RAX;
    l_gp_reg_mapping.gp_reg_b_base = LIBXSMM_X86_GP_REG_RSI;
    l_gp_reg_mapping.gp_reg_b_offset = LIBXSMM_X86_GP_REG_R9;
    l_gp_reg_mapping.gp_reg_b = LIBXSMM_X86_GP_REG_RBX;
    l_gp_reg_mapping.gp_reg_c = LIBXSMM_X86_GP_REG_RDX;
    l_gp_reg_mapping.gp_reg_reduce_count = LIBXSMM_X86_GP_REG_RCX;
    l_gp_reg_mapping.gp_reg_reduce_loop = LIBXSMM_X86_GP_REG_R10;
  } else if (i_xgemm_desc->flags & LIBXSMM_GEMM_FLAG_BATCH_REDUCE_STRIDE) {
    l_gp_reg_mapping.gp_reg_a_base = LIBXSMM_X86_GP_REG_RDI;
    l_gp_reg_mapping.gp_reg_a = LIBXSMM_X86_GP_REG_R8;
    l_gp_reg_mapping.gp_reg_b_base = LIBXSMM_X86_GP_REG_RSI;
    l_gp_reg_mapping.gp_reg_b = LIBXSMM_X86_GP_REG_RBX;
    l_gp_reg_mapping.gp_reg_c = LIBXSMM_X86_GP_REG_RDX;
    l_gp_reg_mapping.gp_reg_reduce_count = LIBXSMM_X86_GP_REG_RCX;
    l_gp_reg_mapping.gp_reg_reduce_loop = LIBXSMM_X86_GP_REG_R10;
  }
#endif
  l_gp_reg_mapping.gp_reg_mloop = LIBXSMM_X86_GP_REG_R12;
  l_gp_reg_mapping.gp_reg_nloop = LIBXSMM_X86_GP_REG_R13;
  l_gp_reg_mapping.gp_reg_help_0 = LIBXSMM_X86_GP_REG_R12;
  l_gp_reg_mapping.gp_reg_help_1 = LIBXSMM_X86_GP_REG_R13;
  l_gp_reg_mapping.gp_reg_kloop = LIBXSMM_X86_GP_REG_R14;
  l_gp_reg_mapping.gp_reg_lda = LIBXSMM_X86_GP_REG_R11;
  l_gp_reg_mapping.gp_reg_ldb = LIBXSMM_X86_GP_REG_R15;
  l_gp_reg_mapping.gp_reg_ldc = LIBXSMM_X86_GP_REG_R14;

  /* define loop_label_tracker */
  libxsmm_reset_loop_label_tracker( &l_loop_label_tracker );

  /* define the micro kernel code gen properties */
  libxsmm_generator_gemm_init_micro_kernel_config_fullvector( &l_micro_kernel_config, io_generated_code->arch, i_xgemm_desc, 0 );

  /* Here compute the 2D blocking info based on the M and N values  */
  /* For now super simple, rudimentary logic, to be generalized later on  */
  m_blocking = 32;
  while (i_xgemm_desc->m % m_blocking != 0) {
    m_blocking--;
  }
  if (m_blocking <= 16) {
    m_blocking_info[0].blocking = m_blocking;
    m_blocking_info[0].block_size = i_xgemm_desc->m;
    m_blocking_info[0].tiles = 1;
    m_blocking_info[0].sizes[0] = m_blocking;
  } else {
    m_blocking_info[0].blocking = m_blocking;
    m_blocking_info[0].block_size = i_xgemm_desc->m;
    m_blocking_info[0].tiles = 2;
    m_blocking_info[0].sizes[0] = (m_blocking+1)/2;
    m_blocking_info[0].sizes[1] = m_blocking - m_blocking_info[0].sizes[0];
  }

  n_blocking = 32;
  while (i_xgemm_desc->n % n_blocking != 0) {
    n_blocking--;
  }
  if (n_blocking <= 16) {
    n_blocking_info[0].blocking = n_blocking;
    n_blocking_info[0].block_size = i_xgemm_desc->n;
    n_blocking_info[0].tiles = 1;
    n_blocking_info[0].sizes[0] = n_blocking;
  } else {
    n_blocking_info[0].blocking = n_blocking;
    n_blocking_info[0].block_size = i_xgemm_desc->n;
    n_blocking_info[0].tiles = 2;
    n_blocking_info[0].sizes[0] = (n_blocking+1)/2;
    n_blocking_info[0].sizes[1] = n_blocking - n_blocking_info[0].sizes[0];
  }

  /* Special case when N = 49 or N = 61 -- we do 1x4 blocking */
  if (i_xgemm_desc->n == 49 || i_xgemm_desc->n == 61) {
    m_blocking = 16;
    while (i_xgemm_desc->m % m_blocking != 0) {
      m_blocking--;
    }
    m_blocking_info[0].blocking = m_blocking;
    m_blocking_info[0].block_size = i_xgemm_desc->m;
    m_blocking_info[0].tiles = 1;
    m_blocking_info[0].sizes[0] = m_blocking;
    if (i_xgemm_desc->n == 49) {
      n_blocking_info[0].blocking = 49;
      n_blocking_info[0].block_size = 49;
      n_blocking_info[0].tiles = 4;
      /* I.e. N = 49 = 3 * 13 + 10 */
      n_blocking_info[0].sizes[0] = 13;
      n_blocking_info[0].sizes[1] = 13;
      n_blocking_info[0].sizes[2] = 13;
      n_blocking_info[0].sizes[3] = 10;
    }
    if (i_xgemm_desc->n == 61) {
      n_blocking_info[0].blocking = 61;
      n_blocking_info[0].block_size = 61;
      n_blocking_info[0].tiles = 4;
      /* I.e. N = 61 = 3 * 16 + 13 */
      n_blocking_info[0].sizes[0] = 16;
      n_blocking_info[0].sizes[1] = 16;
      n_blocking_info[0].sizes[2] = 16;
      n_blocking_info[0].sizes[3] = 13;
    }
  }


#if 0
  if (i_xgemm_desc->n == 49) {
    n_blocking_info[0].blocking = 19;
    n_blocking_info[0].block_size = 38;
    n_blocking_info[0].tiles = 2;
    n_blocking_info[0].sizes[0] = 11;
    n_blocking_info[0].sizes[1] = 8;

    n_blocking_info[1].blocking = 11;
    n_blocking_info[1].block_size = 11;
    n_blocking_info[1].tiles = 1;
    n_blocking_info[1].sizes[0] = 11;
    n_blocking_info[1].sizes[1] = 11;
  }

  if (i_xgemm_desc->n == 61) {
    n_blocking_info[0].blocking = 25;
    n_blocking_info[0].block_size = 50;
    n_blocking_info[0].tiles = 2;
    n_blocking_info[0].sizes[0] = 11;
    n_blocking_info[0].sizes[1] = 14;

    n_blocking_info[1].blocking = 11;
    n_blocking_info[1].block_size = 11;
    n_blocking_info[1].tiles = 1;
    n_blocking_info[1].sizes[0] = 11;
    n_blocking_info[1].sizes[1] = 11;
  }
#endif

  /* Find K blocking  */
  k_blocking = 16;
  while (i_xgemm_desc->k % k_blocking != 0) {
    k_blocking--;
  }

  /* First init all tiles with default value 16 */
  for (im = 0; im < 8; im++) {
    libxsmm_setup_tile(im, 16, 16, &tile_config);
  }

  /* For now introduce here tileconfig redundantly -- want to move it externally somewhere... */
  /* Create array with tileconfig */
  /* TODO: revisit */
  tile_config.palette_id = 1;
  /* First configure the accumulator tiles 0-4 */
  m_tiles = m_blocking_info[0].tiles;
  n_tiles = n_blocking_info[0].tiles;
  for (im = 0; im < m_tiles; im++) {
    for (in = 0; in < n_tiles; in++) {
      libxsmm_setup_tile(ii, m_blocking_info[0].sizes[im], n_blocking_info[0].sizes[in], &tile_config);
      ii++;
      if (n_tiles == 1) {
        ii++;
      }
    }
  }
  /* Configure tiles for A */
  libxsmm_setup_tile(4, m_blocking_info[0].sizes[0], k_blocking, &tile_config);
  if (m_tiles == 2) {
    libxsmm_setup_tile(5, m_blocking_info[0].sizes[1], k_blocking, &tile_config);
  }
  /* Configure tiles for B */
  libxsmm_setup_tile(6, k_blocking, n_blocking_info[0].sizes[0], &tile_config);
  if (n_tiles == 2) {
    libxsmm_setup_tile(7, k_blocking, n_blocking_info[0].sizes[1], &tile_config);
  }
  if (n_tiles == 4) {
    libxsmm_setup_tile(7, k_blocking, n_blocking_info[0].sizes[3], &tile_config);
  }

  /* open asm */
  libxsmm_x86_instruction_open_stream_amx( io_generated_code, &l_gp_reg_mapping, i_xgemm_desc->prefetch );
  /* Setup stack frame...  */
  libxsmm_generator_gemm_amx_setup_stack_frame( io_generated_code, i_xgemm_desc, &l_gp_reg_mapping, &l_micro_kernel_config, m_tiles, n_tiles );
  libxsmm_generator_gemm_amx_setup_fusion_infra( io_generated_code, i_xgemm_desc, &l_gp_reg_mapping, &l_micro_kernel_config );

  if ((((LIBXSMM_GEMM_FLAG_NO_RESET_TILECONFIG & i_xgemm_desc->flags) != 0) && ((LIBXSMM_GEMM_FLAG_NO_SETUP_TILECONFIG & i_xgemm_desc->flags) == 0)) ||
      (((LIBXSMM_GEMM_FLAG_NO_RESET_TILECONFIG & i_xgemm_desc->flags) == 0) && ((LIBXSMM_GEMM_FLAG_NO_SETUP_TILECONFIG & i_xgemm_desc->flags) == 0))    ) {
    libxsmm_x86_instruction_tile_control( io_generated_code,
        0,
        l_micro_kernel_config.instruction_set,
        LIBXSMM_X86_INSTR_LDTILECFG,
        LIBXSMM_X86_GP_REG_UNDEF,
        0,
        &tile_config );
  }

  if ( (((LIBXSMM_GEMM_FLAG_NO_RESET_TILECONFIG & i_xgemm_desc->flags) == 0) && ((LIBXSMM_GEMM_FLAG_NO_SETUP_TILECONFIG & i_xgemm_desc->flags) == 0)) ||
      (((LIBXSMM_GEMM_FLAG_NO_RESET_TILECONFIG & i_xgemm_desc->flags) != 0) && ((LIBXSMM_GEMM_FLAG_NO_SETUP_TILECONFIG & i_xgemm_desc->flags) != 0))     ) {
    /* Set the LD registers for A and B matrices */
    libxsmm_x86_instruction_alu_imm(io_generated_code, l_micro_kernel_config.alu_mov_instruction, l_gp_reg_mapping.gp_reg_lda, (i_xgemm_desc->lda * l_micro_kernel_config.datatype_size)/4);
    libxsmm_x86_instruction_alu_imm(io_generated_code, l_micro_kernel_config.alu_mov_instruction, l_gp_reg_mapping.gp_reg_ldb, (i_xgemm_desc->ldb * l_micro_kernel_config.datatype_size)/4);
    libxsmm_x86_instruction_alu_imm(io_generated_code, l_micro_kernel_config.alu_mov_instruction, l_gp_reg_mapping.gp_reg_ldc, (i_xgemm_desc->ldc * l_micro_kernel_config.datatype_size)/4);

    libxsmm_generator_gemm_amx_kernel_nloop(io_generated_code, &l_loop_label_tracker, &l_gp_reg_mapping, &l_micro_kernel_config, i_xgemm_desc, n_blocking_info, m_blocking_info);
  }

  /* Conditionally perform a tilerelease */
  if ( (((LIBXSMM_GEMM_FLAG_NO_RESET_TILECONFIG & i_xgemm_desc->flags) == 0) && ((LIBXSMM_GEMM_FLAG_NO_SETUP_TILECONFIG & i_xgemm_desc->flags) != 0)) ||
      (((LIBXSMM_GEMM_FLAG_NO_RESET_TILECONFIG & i_xgemm_desc->flags) == 0) && ((LIBXSMM_GEMM_FLAG_NO_SETUP_TILECONFIG & i_xgemm_desc->flags) == 0))    ) {
    libxsmm_x86_instruction_tile_control( io_generated_code,
        0,
        l_micro_kernel_config.instruction_set,
        LIBXSMM_X86_INSTR_TILERELEASE,
        LIBXSMM_X86_GP_REG_UNDEF,
        0,
        &tile_config );
  }

  /* Properly destroy stack frame...  */
  libxsmm_generator_gemm_amx_destroy_stack_frame( io_generated_code, i_xgemm_desc, &l_gp_reg_mapping, &l_micro_kernel_config );
  /* close asm */
  libxsmm_x86_instruction_close_stream_amx( io_generated_code, &l_gp_reg_mapping, i_xgemm_desc->prefetch );
}


LIBXSMM_API_INTERN
void libxsmm_generator_gemm_amx_kernel_mloop( libxsmm_generated_code*            io_generated_code,
    libxsmm_loop_label_tracker*        io_loop_label_tracker,
    libxsmm_gp_reg_mapping*            i_gp_reg_mapping,
    libxsmm_micro_kernel_config*       i_micro_kernel_config,
    const libxsmm_gemm_descriptor*     i_xgemm_desc,
    libxsmm_blocking_info_t*           n_blocking_info,
    libxsmm_blocking_info_t*           m_blocking_info ) {

  void (*l_generator_kloop)(libxsmm_generated_code*, libxsmm_loop_label_tracker*, const libxsmm_gp_reg_mapping*, libxsmm_micro_kernel_config*, const libxsmm_gemm_descriptor*,  libxsmm_blocking_info_t*,  libxsmm_blocking_info_t*, unsigned int, unsigned int);
  unsigned int l_m_done = 0;
  unsigned int l_m_count = 0;
  unsigned int l_m_blocking = m_blocking_info[0].blocking;
  unsigned int l_m_block_size = 0;
  unsigned int m_assembly_loop_exists = (l_m_blocking == i_xgemm_desc->m) ? 0 : 1;
  unsigned int fully_unroll_k = 1;
  unsigned int NON_UNROLLED_BR_LOOP_LABEL_START = 0;
  unsigned int NON_UNROLLED_BR_LOOP_LABEL_END = 1;
  unsigned int i;
  unsigned int A_offs = 0, B_offs = 0;

  libxsmm_jump_label_tracker l_jump_label_tracker;
  libxsmm_reset_jump_label_tracker(&l_jump_label_tracker);
  l_generator_kloop = libxsmm_generator_gemm_amx_kernel_kloop;

  /* apply m_blocking */
  while (l_m_done != (unsigned int)i_xgemm_desc->m) {
    l_m_blocking = m_blocking_info[l_m_count].blocking;
    l_m_block_size = m_blocking_info[l_m_count].block_size;
    l_m_done += l_m_block_size;

    if (m_assembly_loop_exists) {
      libxsmm_generator_gemm_header_mloop_amx( io_generated_code, io_loop_label_tracker, i_gp_reg_mapping, i_micro_kernel_config, l_m_blocking );
    }

    libxsmm_generator_gemm_load_C_amx( io_generated_code, i_gp_reg_mapping, i_micro_kernel_config, i_xgemm_desc, n_blocking_info, &m_blocking_info[l_m_count] );

    if ((i_xgemm_desc->flags & LIBXSMM_GEMM_FLAG_BATCH_REDUCE_ADDRESS) || (i_xgemm_desc->flags & LIBXSMM_GEMM_FLAG_BATCH_REDUCE_OFFSET) || (i_xgemm_desc->flags & LIBXSMM_GEMM_FLAG_BATCH_REDUCE_STRIDE)) {
      /* Compare actual trip count to the hint value. If equal jump to UNROLL START LABEL*/
      if (i_xgemm_desc->c3 > 0) {
        libxsmm_x86_instruction_alu_imm(io_generated_code, i_micro_kernel_config->alu_cmp_instruction, i_gp_reg_mapping->gp_reg_reduce_count, i_xgemm_desc->c3);
        libxsmm_x86_instruction_jump_to_label(io_generated_code, LIBXSMM_X86_INSTR_JNE, NON_UNROLLED_BR_LOOP_LABEL_START, &l_jump_label_tracker);
      }

      /* UNROLLED version code is here */
      if ((i_xgemm_desc->flags & LIBXSMM_GEMM_FLAG_BATCH_REDUCE_STRIDE) && (i_xgemm_desc->c3 > 0) ) {
        libxsmm_x86_instruction_alu_reg( io_generated_code, i_micro_kernel_config->alu_mov_instruction, i_gp_reg_mapping->gp_reg_a_base, i_gp_reg_mapping->gp_reg_a);
        libxsmm_x86_instruction_alu_reg( io_generated_code, i_micro_kernel_config->alu_mov_instruction, i_gp_reg_mapping->gp_reg_b_base, i_gp_reg_mapping->gp_reg_b);
      }
      /* This is the reduce loop  */
      for (i = 0; i < i_xgemm_desc->c3; i++) {
        if (i_xgemm_desc->flags & LIBXSMM_GEMM_FLAG_BATCH_REDUCE_ADDRESS) {
          /* load to reg_a the proper array based on the reduce loop index  */
          libxsmm_x86_instruction_alu_mem( io_generated_code,
              i_micro_kernel_config->alu_mov_instruction,
              i_gp_reg_mapping->gp_reg_a_ptrs,
              LIBXSMM_X86_GP_REG_UNDEF, 0,
              i*8,
              i_gp_reg_mapping->gp_reg_a,
              0 );
          /* load to reg_b the proper array based on the reduce loop index  */
          libxsmm_x86_instruction_alu_mem( io_generated_code,
              i_micro_kernel_config->alu_mov_instruction,
              i_gp_reg_mapping->gp_reg_b_ptrs,
              LIBXSMM_X86_GP_REG_UNDEF, 0,
              i*8,
              i_gp_reg_mapping->gp_reg_b,
              0 );
        } else if (i_xgemm_desc->flags & LIBXSMM_GEMM_FLAG_BATCH_REDUCE_OFFSET) {
          /* Calculate to reg_a the proper address based on the reduce loop index  */
          libxsmm_x86_instruction_alu_mem( io_generated_code,
              i_micro_kernel_config->alu_mov_instruction,
              i_gp_reg_mapping->gp_reg_a_offset,
              LIBXSMM_X86_GP_REG_UNDEF, 0,
              i*8,
              i_gp_reg_mapping->gp_reg_a,
              0 );
          libxsmm_x86_instruction_alu_reg( io_generated_code, i_micro_kernel_config->alu_add_instruction, i_gp_reg_mapping->gp_reg_a_base, i_gp_reg_mapping->gp_reg_a);
          /* Calculate to reg_b the proper address based on the reduce loop index  */
          libxsmm_x86_instruction_alu_mem( io_generated_code,
              i_micro_kernel_config->alu_mov_instruction,
              i_gp_reg_mapping->gp_reg_b_offset,
              LIBXSMM_X86_GP_REG_UNDEF, 0,
              i*8,
              i_gp_reg_mapping->gp_reg_b,
              0 );
          libxsmm_x86_instruction_alu_reg( io_generated_code, i_micro_kernel_config->alu_add_instruction, i_gp_reg_mapping->gp_reg_b_base, i_gp_reg_mapping->gp_reg_b);
        } else if (i_xgemm_desc->flags & LIBXSMM_GEMM_FLAG_BATCH_REDUCE_STRIDE) {
          A_offs = i * i_xgemm_desc->c1;
          B_offs = i * i_xgemm_desc->c2;
        }

        /* Here is the K loop along with the microkernel */
        l_generator_kloop(io_generated_code, io_loop_label_tracker, i_gp_reg_mapping, i_micro_kernel_config, i_xgemm_desc, n_blocking_info, &m_blocking_info[l_m_count], A_offs, B_offs);

        /* In case of address based batch redcue push the proper A/B address updates if the k loop is not fully unrolled */
        if ((i_xgemm_desc->flags & LIBXSMM_GEMM_FLAG_BATCH_REDUCE_ADDRESS) && (fully_unroll_k == 0)) {
          libxsmm_x86_instruction_alu_mem( io_generated_code,
              i_micro_kernel_config->alu_mov_instruction,
              i_gp_reg_mapping->gp_reg_b_ptrs,
              LIBXSMM_X86_GP_REG_UNDEF, 0,
              i*8,
              i_gp_reg_mapping->gp_reg_b,
              1 );
          libxsmm_x86_instruction_alu_mem( io_generated_code,
              i_micro_kernel_config->alu_mov_instruction,
              i_gp_reg_mapping->gp_reg_a_ptrs,
              LIBXSMM_X86_GP_REG_UNDEF, 0,
              i*8,
              i_gp_reg_mapping->gp_reg_a,
              1 );
        }
      }

      if (i_xgemm_desc->c3 > 0) {
        if (!(i_xgemm_desc->flags & LIBXSMM_GEMM_FLAG_BATCH_REDUCE_STRIDE)) {
          libxsmm_generator_gemm_store_C_amx( io_generated_code, i_gp_reg_mapping, i_micro_kernel_config, i_xgemm_desc, n_blocking_info, &m_blocking_info[l_m_count] );
        }
      }

      /* End of UNROLLED code is here*/
      /* Jump after non-unrolled code variant */
      if (i_xgemm_desc->c3 > 0) {
        libxsmm_x86_instruction_jump_to_label(io_generated_code, LIBXSMM_X86_INSTR_JMP, NON_UNROLLED_BR_LOOP_LABEL_END, &l_jump_label_tracker);
      }

      /* NON_UNROLLED_BR_LOOP_LABEL_START */
      libxsmm_x86_instruction_register_jump_label(io_generated_code, NON_UNROLLED_BR_LOOP_LABEL_START, &l_jump_label_tracker);
      /* This is the reduce loop  */
      libxsmm_generator_gemm_header_reduceloop_amx( io_generated_code, io_loop_label_tracker, i_gp_reg_mapping, i_micro_kernel_config );
      if (i_xgemm_desc->flags & LIBXSMM_GEMM_FLAG_BATCH_REDUCE_ADDRESS) {
        /* load to reg_a the proper array based on the reduce loop index  */
        libxsmm_x86_instruction_alu_mem( io_generated_code,
            i_micro_kernel_config->alu_mov_instruction,
            i_gp_reg_mapping->gp_reg_a_ptrs,
            i_gp_reg_mapping->gp_reg_reduce_loop, 8,
            0,
            i_gp_reg_mapping->gp_reg_a,
            0 );
        /* load to reg_b the proper array based on the reduce loop index  */
        libxsmm_x86_instruction_alu_mem( io_generated_code,
            i_micro_kernel_config->alu_mov_instruction,
            i_gp_reg_mapping->gp_reg_b_ptrs,
            i_gp_reg_mapping->gp_reg_reduce_loop, 8,
            0,
            i_gp_reg_mapping->gp_reg_b,
            0 );
      } else if (i_xgemm_desc->flags & LIBXSMM_GEMM_FLAG_BATCH_REDUCE_OFFSET) {
        /* Calculate to reg_a the proper address based on the reduce loop index  */
        libxsmm_x86_instruction_alu_mem( io_generated_code,
            i_micro_kernel_config->alu_mov_instruction,
            i_gp_reg_mapping->gp_reg_a_offset,
            i_gp_reg_mapping->gp_reg_reduce_loop, 8,
            0,
            i_gp_reg_mapping->gp_reg_a,
            0 );
        libxsmm_x86_instruction_alu_reg( io_generated_code, i_micro_kernel_config->alu_add_instruction, i_gp_reg_mapping->gp_reg_a_base, i_gp_reg_mapping->gp_reg_a);
        /* Calculate to reg_b the proper address based on the reduce loop index  */
        libxsmm_x86_instruction_alu_mem( io_generated_code,
            i_micro_kernel_config->alu_mov_instruction,
            i_gp_reg_mapping->gp_reg_b_offset,
            i_gp_reg_mapping->gp_reg_reduce_loop, 8,
            0,
            i_gp_reg_mapping->gp_reg_b,
            0 );
        libxsmm_x86_instruction_alu_reg( io_generated_code, i_micro_kernel_config->alu_add_instruction, i_gp_reg_mapping->gp_reg_b_base, i_gp_reg_mapping->gp_reg_b);
      } else if (i_xgemm_desc->flags & LIBXSMM_GEMM_FLAG_BATCH_REDUCE_STRIDE) {
        libxsmm_x86_instruction_alu_reg( io_generated_code, i_micro_kernel_config->alu_mov_instruction, i_gp_reg_mapping->gp_reg_reduce_loop, i_gp_reg_mapping->gp_reg_a);
        libxsmm_x86_instruction_alu_imm( io_generated_code, LIBXSMM_X86_INSTR_IMUL, i_gp_reg_mapping->gp_reg_a, i_xgemm_desc->c1);
        libxsmm_x86_instruction_alu_reg( io_generated_code, i_micro_kernel_config->alu_add_instruction, i_gp_reg_mapping->gp_reg_a_base, i_gp_reg_mapping->gp_reg_a);
        libxsmm_x86_instruction_alu_reg( io_generated_code, i_micro_kernel_config->alu_mov_instruction, i_gp_reg_mapping->gp_reg_reduce_loop, i_gp_reg_mapping->gp_reg_b);
        libxsmm_x86_instruction_alu_imm( io_generated_code, LIBXSMM_X86_INSTR_IMUL, i_gp_reg_mapping->gp_reg_b, i_xgemm_desc->c2);
        libxsmm_x86_instruction_alu_reg( io_generated_code, i_micro_kernel_config->alu_add_instruction, i_gp_reg_mapping->gp_reg_b_base, i_gp_reg_mapping->gp_reg_b);
      }

      /* Here is the K loop along with the microkernel */
      l_generator_kloop(io_generated_code, io_loop_label_tracker, i_gp_reg_mapping, i_micro_kernel_config, i_xgemm_desc, n_blocking_info, &m_blocking_info[l_m_count], 0, 0);

      /* In case of address based batch redcue push the proper A/B address updates if the k loop is not fully unrolled */
      if ((i_xgemm_desc->flags & LIBXSMM_GEMM_FLAG_BATCH_REDUCE_ADDRESS) && (fully_unroll_k == 0)) {
        libxsmm_x86_instruction_alu_mem( io_generated_code,
            i_micro_kernel_config->alu_mov_instruction,
            i_gp_reg_mapping->gp_reg_b_ptrs,
            i_gp_reg_mapping->gp_reg_reduce_loop, 8,
            0,
            i_gp_reg_mapping->gp_reg_b,
            1 );
        libxsmm_x86_instruction_alu_mem( io_generated_code,
            i_micro_kernel_config->alu_mov_instruction,
            i_gp_reg_mapping->gp_reg_a_ptrs,
            i_gp_reg_mapping->gp_reg_reduce_loop, 8,
            0,
            i_gp_reg_mapping->gp_reg_a,
            1 );
      }

      libxsmm_generator_gemm_footer_reduceloop_amx( io_generated_code, io_loop_label_tracker, i_gp_reg_mapping, i_micro_kernel_config, i_xgemm_desc);

      libxsmm_generator_gemm_store_C_amx( io_generated_code, i_gp_reg_mapping, i_micro_kernel_config, i_xgemm_desc, n_blocking_info, &m_blocking_info[l_m_count] );

      /* NON_UNROLLED_BR_LOOP_LABEL_END */
      libxsmm_x86_instruction_register_jump_label(io_generated_code, NON_UNROLLED_BR_LOOP_LABEL_END, &l_jump_label_tracker);
    } else {
      /* Here is the K loop along with the microkernel */
      l_generator_kloop(io_generated_code, io_loop_label_tracker, i_gp_reg_mapping, i_micro_kernel_config, i_xgemm_desc, n_blocking_info, &m_blocking_info[l_m_count], 0, 0);
      libxsmm_generator_gemm_store_C_amx( io_generated_code, i_gp_reg_mapping, i_micro_kernel_config, i_xgemm_desc, n_blocking_info, &m_blocking_info[l_m_count] );
    }

    if (m_assembly_loop_exists) {
      libxsmm_generator_gemm_footer_mloop_amx( io_generated_code, io_loop_label_tracker, i_gp_reg_mapping, i_micro_kernel_config, i_xgemm_desc, l_m_blocking, l_m_done, fully_unroll_k );
    }
    l_m_count++;
  }
}


LIBXSMM_API_INTERN
void libxsmm_generator_gemm_amx_kernel_nloop( libxsmm_generated_code*            io_generated_code,
    libxsmm_loop_label_tracker*        io_loop_label_tracker,
    libxsmm_gp_reg_mapping*            i_gp_reg_mapping,
    libxsmm_micro_kernel_config*       i_micro_kernel_config,
    const libxsmm_gemm_descriptor*     i_xgemm_desc,
    libxsmm_blocking_info_t*           n_blocking_info,
    libxsmm_blocking_info_t*           m_blocking_info ) {

  /* initialize n-blocking */
  unsigned int l_n_count = 0;          /* array counter for blocking info  arrays */
  unsigned int l_n_done = 0;           /* progress tracker */
  unsigned int l_n_blocking = 0;
  unsigned int l_n_block_size = 0;
  unsigned int m_assembly_loop_exists = (m_blocking_info[0].blocking < (unsigned int)i_xgemm_desc->m) ? 1 : 0;
  unsigned int n_assembly_loop_exists = (n_blocking_info[0].blocking < (unsigned int)i_xgemm_desc->n) ? 1 : 0;

  i_micro_kernel_config->m_loop_exists = m_assembly_loop_exists;
  i_micro_kernel_config->n_loop_exists = n_assembly_loop_exists;

  /* Load the actual batch-reduce trip count */
  if ((i_xgemm_desc->flags & LIBXSMM_GEMM_FLAG_BATCH_REDUCE_ADDRESS) || (i_xgemm_desc->flags & LIBXSMM_GEMM_FLAG_BATCH_REDUCE_OFFSET) || (i_xgemm_desc->flags & LIBXSMM_GEMM_FLAG_BATCH_REDUCE_STRIDE)) {
    libxsmm_x86_instruction_alu_mem( io_generated_code,
        i_micro_kernel_config->alu_mov_instruction,
        i_gp_reg_mapping->gp_reg_reduce_count,
        LIBXSMM_X86_GP_REG_UNDEF, 0,
        0,
        i_gp_reg_mapping->gp_reg_reduce_count,
        0 );
  }

  /* apply n_blocking */
  while (l_n_done != (unsigned int)i_xgemm_desc->n) {
    l_n_blocking = n_blocking_info[l_n_count].blocking;
    l_n_block_size = n_blocking_info[l_n_count].block_size;
    /* advance N */
    l_n_done += l_n_block_size;

    if (l_n_blocking < i_xgemm_desc->n) {
      /* Open N loop */
      libxsmm_generator_gemm_header_nloop_amx( io_generated_code, io_loop_label_tracker, i_gp_reg_mapping, i_micro_kernel_config, l_n_blocking );
    }

    /* Generate M loop  */
    libxsmm_generator_gemm_amx_kernel_mloop(io_generated_code, io_loop_label_tracker, i_gp_reg_mapping, i_micro_kernel_config, i_xgemm_desc, &n_blocking_info[l_n_count], m_blocking_info);

    if (l_n_blocking < i_xgemm_desc->n) {
      /* Close N loop */
      libxsmm_generator_gemm_footer_nloop_amx( io_generated_code, io_loop_label_tracker, i_gp_reg_mapping, i_micro_kernel_config, i_xgemm_desc, l_n_blocking, l_n_done, m_assembly_loop_exists );
    }
    l_n_count++;
  }
}

