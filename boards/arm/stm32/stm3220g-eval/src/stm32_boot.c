/****************************************************************************
 * boards/arm/stm32/stm3220g-eval/src/stm32_boot.c
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

#include <debug.h>

#include <nuttx/board.h>
#include <arch/board/board.h>

#include "arm_internal.h"
#include "stm3220g-eval.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: stm32_boardinitialize
 *
 * Description:
 *   All STM32 architectures must provide the following entry point.
 *   This entry point is called early in the initialization -- after all
 *   memory has been configured and mapped but before any devices have been
 *   initialized.
 *
 ****************************************************************************/

void stm32_boardinitialize(void)
{
  /* Configure SPI chip selects if 1) SPI is not disabled, and 2) the weak
   * function stm32_spidev_initialize() has been brought into the link.
   */

#if defined(CONFIG_STM32_SPI1) || defined(CONFIG_STM32_SPI2) || defined(CONFIG_STM32_SPI3)
  if (stm32_spidev_initialize)
    {
      stm32_spidev_initialize();
    }
#endif

  /* If the FSMC is enabled, then enable SRAM access */

#ifdef CONFIG_STM32_FSMC
  stm32_selectsram();
#endif

  /* Initialize USB if the 1) OTG FS controller is in the configuration and
   * 2) the weak function stm32_usbinitialize() has been brought into the
   * build.
   * Presumably either CONFIG_USBDEV or CONFIG_USBHOST is also selected.
   */

#ifdef CONFIG_STM32_OTGFS
  if (stm32_usbinitialize)
    {
      stm32_usbinitialize();
    }
#endif

  /* Configure on-board LEDs if LED support has been selected. */

#ifdef CONFIG_ARCH_LEDS
  board_autoled_initialize();
#endif
}
