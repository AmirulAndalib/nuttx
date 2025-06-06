/****************************************************************************
 * arch/arm/src/sama5/sam_systemreset.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>
#include <assert.h>

#include <nuttx/arch.h>
#include <nuttx/board.h>
#include <arch/sama5/chip.h>

#include "arm_internal.h"
#include "hardware/sam_rstc.h"
#include "sam_systemreset.h"

#ifdef CONFIG_SAMA5_SYSTEMRESET

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: sam_get_reset_cause
 *
 * Description:
 *   Get cause of the last CPU reset. This is done by reading reset status
 *   registger.
 *
 * Returned Value:
 *   CPU reset cause in form of macros defined in sam_systemreset.h. This is
 *   to avoid passing boardctl dependent structure to architecture layer.
 *   Board level specific code should include sam_systemreset.h and set
 *   boardctl result according to that. -1 is returned in case of invalid
 *   value in status register.
 *
 ****************************************************************************/

#ifdef CONFIG_BOARDCTL_RESET_CAUSE
int sam_get_reset_cause(void)
{
  int ret;
  uint32_t rstsr;

  rstsr = getreg32(SAM_RSTC_SR);
  switch (rstsr & RSTC_SR_RSTTYP_MASK)
    {
      case RSTC_SR_RSTTYP_PWRUP:
        ret = SAMA5_RESET_PWRUP;
        break;
      case RSTC_SR_RSTTYP_BACKUP:
        ret =  SAMA5_RESET_BACKUP;
        break;
      case RSTC_SR_RSTTYP_WDOG:
        ret =  SAMA5_RESET_WDOG;
        break;
      case RSTC_SR_RSTTYP_SWRST:
        ret =  SAMA5_RESET_SWRST;
        break;
      case RSTC_SR_RSTTYP_NRST:
        ret = SAMA5_RESET_NRST;
        break;
      default:
        ret = -1;
        break;
    }

  return ret;
}
#endif

/****************************************************************************
 * Name: up_systemreset
 *
 * Description:
 *   Internal reset logic.
 *
 ****************************************************************************/

void up_systemreset(void)
{
  uint32_t rstcr;
#if defined(CONFIG_SAMA5_EXTRESET_ERST) && CONFIG_SAMA5_EXTRESET_ERST != 0
  uint32_t rstmr;
#endif

  rstcr  = (RSTC_CR_PROCRST | RSTC_CR_KEY);

#if defined(CONFIG_ARCH_CHIP_SAMA5D3)
  rstcr  |= RSTC_CR_PERRST;
#endif

#if defined(CONFIG_SAMA5_EXTRESET_ERST) && CONFIG_SAMA5_EXTRESET_ERST != 0
  rstcr |= RSTC_CR_EXTRST;

  rstmr  = getreg32(SAM_RSTC_MR);
  rstmr &= ~RSTC_MR_ERSTL_MASK;
  rstmr &= RSTC_MR_ERSTL(CONFIG_SAMA5_EXTRESET_ERST - 1) | RSTC_MR_KEY;
  putreg32(rstmr, SAM_RSTC_MR);
#endif

  putreg32(rstcr, SAM_RSTC_CR);

  /* Wait for the reset */

  for (; ; );
}
#endif /* CONFIG_SAMA5_SYSTEMRESET */
