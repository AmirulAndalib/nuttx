/****************************************************************************
 * arch/xtensa/src/common/xtensa_asm_utils.h
 *
 * SPDX-License-Identifier: Apache-2.0
 * SPDX-FileCopyrightText: 2017, Intel Corporation
 *
 ****************************************************************************/

#ifndef __ARCH_XTENSA_SRC_COMMON_XTENSA_ASM_UTILS_H
#define __ARCH_XTENSA_SRC_COMMON_XTENSA_ASM_UTILS_H

/****************************************************************************
 * Assembly Language Macros
 ****************************************************************************/

/****************************************************************************
 *
 * Name: SPILL_ALL_WINDOWS
 *
 * Spills all windowed registers (i.e. registers not visible as
 * A0-A15) to their ABI-defined spill regions on the stack.
 *
 * Unlike the Xtensa HAL implementation, this code requires that the
 * EXCM and WOE bit be enabled in PS, and relies on repeated hardware
 * exception handling to do the register spills.  The trick is to do a
 * noop write to the high registers, which the hardware will trap
 * (into an overflow exception) in the case where those registers are
 * already used by an existing call frame.  Then it rotates the window
 * and repeats until all but the A0-A3 registers of the original frame
 * are guaranteed to be spilled, eventually rotating back around into
 * the original frame.  Advantages:
 *
 * - Vastly smaller code size
 *
 * - More easily maintained if changes are needed to window over/underflow
 *   exception handling.
 *
 * - Requires no scratch registers to do its work, so can be used safely in
 *   any context.
 *
 * - If the WOE bit is not enabled (for example, in code written for
 *   the CALL0 ABI), this becomes a silent noop and operates compatbily.
 *
 * - Hilariously it's ACTUALLY FASTER than the HAL routine.  And not
 *   just a little bit, it's MUCH faster.  With a mostly full register
 *   file on an LX6 core (ESP-32) I'm measuring 145 cycles to spill
 *   registers with this vs. 279 (!) to do it with
 *   xthal_spill_windows().
 ****************************************************************************/

.macro SPILL_ALL_WINDOWS
#if XCHAL_NUM_AREGS == 64
  and a12, a12, a12
  rotw 3
  and a12, a12, a12
  rotw 3
  and a12, a12, a12
  rotw 3
  and a12, a12, a12
  rotw 3
  and a12, a12, a12
  rotw 4
#elif XCHAL_NUM_AREGS == 32
  and a12, a12, a12
  rotw 3
  and a12, a12, a12
  rotw 3
  and a4, a4, a4
  rotw 2
#else
#error Unrecognized XCHAL_NUM_AREGS
#endif
.endm

#endif /* __ARCH_XTENSA_SRC_COMMON_XTENSA_ASM_UTILS_H */
