/****************************************************************************
 * boards/arm/samd5e5/metro-m4/src/sam_composite.c
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
#include <sys/types.h>
#include <assert.h>
#include <debug.h>

#include <nuttx/usb/usbdev.h>
#include <nuttx/usb/usbdev_trace.h>
#include <nuttx/usb/cdcacm.h>
#include <nuttx/usb/usbmsc.h>
#include <nuttx/usb/composite.h>

#include "metro-m4.h"

#if defined(CONFIG_BOARDCTL_USBDEVCTRL) && defined(CONFIG_USBDEV_COMPOSITE)

/****************************************************************************
 * Private Data
 ****************************************************************************/

#ifdef CONFIG_USBMSC_COMPOSITE
static void *g_mschandle;
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: board_mscclassobject
 *
 * Description:
 *   If the mass storage class driver is part of composite device, then
 *   its instantiation and configuration is a multi-step, board-specific,
 *   process (See comments for usbmsc_configure below).  In this case,
 *   board-specific logic must provide board_mscclassobject().
 *
 *   board_mscclassobject() is called from the composite driver.  It must
 *   encapsulate the instantiation and configuration of the mass storage
 *   class and the return the mass storage device's class driver instance
 *   to the composite driver.
 *
 * Input Parameters:
 *   classdev - The location to return the mass storage class' device
 *     instance.
 *
 * Returned Value:
 *   0 on success; a negated errno on failure
 *
 ****************************************************************************/

#ifdef CONFIG_USBMSC_COMPOSITE
static int board_mscclassobject(int minor,
                                struct usbdev_devinfo_s *devinfo,
                                struct usbdevclass_driver_s **classdev)
{
  int ret;

  DEBUGASSERT(g_mschandle == NULL);

  /* Configure the mass storage device */

  uinfo("Configuring with NLUNS=1\n");
  ret = usbmsc_configure(1, &g_mschandle);
  if (ret < 0)
    {
      uerr("ERROR: usbmsc_configure failed: %d\n", -ret);
      return ret;
    }

  uinfo("MSC handle=%p\n", g_mschandle);

  /* Bind the LUN(s) */

  uinfo("Bind LUN=0 to /dev/mmcsd0\n");
  ret = usbmsc_bindlun(g_mschandle, "/dev/mmcsd0", 0, 0, 0, false);
  if (ret < 0)
    {
      uerr("ERROR: usbmsc_bindlun failed for LUN 1 at /dev/mmcsd0: %d\n",
           ret);
      usbmsc_uninitialize(g_mschandle);
      g_mschandle = NULL;
      return ret;
    }

  /* Get the mass storage device's class object */

  ret = usbmsc_classobject(g_mschandle, devinfo, classdev);
  if (ret < 0)
    {
      uerr("ERROR: usbmsc_classobject failed: %d\n", -ret);
      usbmsc_uninitialize(g_mschandle);
      g_mschandle = NULL;
    }

  return ret;
}
#endif

/****************************************************************************
 * Name: board_mscuninitialize
 *
 * Description:
 *   Un-initialize the USB storage class driver. This is just an application
 *   specific wrapper aboutn usbmsc_unitialize() that is called form the
 *   composite device logic.
 *
 * Input Parameters:
 *   classdev - The class driver instance previously given to the composite
 *     driver by board_mscclassobject().
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

#ifdef CONFIG_USBMSC_COMPOSITE
static void board_mscuninitialize(struct usbdevclass_driver_s *classdev)
{
  DEBUGASSERT(g_mschandle != NULL);
  usbmsc_uninitialize(g_mschandle);
  g_mschandle = NULL;
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: board_composite_initialize
 *
 * Description:
 *   Perform architecture specific initialization of a composite USB device.
 *
 ****************************************************************************/

int board_composite_initialize(int port)
{
  return OK;
}

/****************************************************************************
 * Name:  board_composite_connect
 *
 * Description:
 *   Connect the USB composite device on the specified USB device port using
 *   the specified configuration.  The interpretation of the configid is
 *   board specific.
 *
 * Input Parameters:
 *   port     - The USB device port.
 *   configid - The USB composite configuration
 *
 * Returned Value:
 *   A non-NULL handle value is returned on success.  NULL is returned on
 *   any failure.
 *
 ****************************************************************************/

void *board_composite_connect(int port, int configid)
{
  /* Here we are composing the configuration of the usb composite device.
   *
   * The standard is to use one CDC/ACM and one USB mass storage device.
   */

  if (configid == 0)
    {
#ifdef CONFIG_USBMSC_COMPOSITE
      struct composite_devdesc_s dev[2];
      int ifnobase = 0;
      int strbase  = COMPOSITE_NSTRIDS;

      /* Configure the CDC/ACM device */

      /* Ask the cdcacm driver to fill in the constants we didn't
       * know here.
       */

      cdcacm_get_composite_devdesc(&dev[0]);

      /* Overwrite and correct some values... */

      /* The callback functions for the CDC/ACM class */

      dev[0].classobject  = cdcacm_classobject;
      dev[0].uninitialize = cdcacm_uninitialize;

      /* Interfaces */

      dev[0].devinfo.ifnobase = ifnobase;             /* Offset to Interface-IDs */
      dev[0].minor = 0;                               /* The minor interface number */

      /* Strings */

      dev[0].devinfo.strbase = strbase;               /* Offset to String Numbers */

      /* Endpoints */

      dev[0].devinfo.epno[CDCACM_EP_INTIN_IDX]   = 3;
      dev[0].devinfo.epno[CDCACM_EP_BULKIN_IDX]  = 4;
      dev[0].devinfo.epno[CDCACM_EP_BULKOUT_IDX] = 5;

      /* Count up the base numbers */

      ifnobase += dev[0].devinfo.ninterfaces;
      strbase  += dev[0].devinfo.nstrings;

      /* Configure the mass storage device device */

      /* Ask the usbmsc driver to fill in the constants we didn't
       * know here.
       */

      usbmsc_get_composite_devdesc(&dev[1]);

      /* Overwrite and correct some values... */

      /* The callback functions for the USBMSC class */

      dev[1].classobject  = board_mscclassobject;
      dev[1].uninitialize = board_mscuninitialize;

      /* Interfaces */

      dev[1].devinfo.ifnobase = ifnobase;               /* Offset to Interface-IDs */
      dev[1].minor = 0;                                 /* The minor interface number */

      /* Strings */

      dev[1].devinfo.strbase = strbase;                 /* Offset to String Numbers */

      /* Endpoints */

      dev[1].devinfo.epno[USBMSC_EP_BULKIN_IDX]  = 1;
      dev[1].devinfo.epno[USBMSC_EP_BULKOUT_IDX] = 2;

      /* Count up the base numbers */

      ifnobase += dev[1].devinfo.ninterfaces;
      strbase  += dev[1].devinfo.nstrings;

      return composite_initialize(composite_getdevdescs(), dev, 2);
#else
      return NULL;
#endif
    }

  /* Configuration with three CDC/ACMs
   *
   * This configuration can be used e.g. on a samv71-xult. The samv71 has
   * 10 Endpoints (EPs). The EPs 0 up to 7 are DMA aware. The EPs 8 and 9
   * are not.
   *
   * In a composite device we need the EP0 as an control Endpoint.  Each
   * CDC/ACM needs one Interrupt driven and two bulk Endpoints.  This is
   * why we configure the EPs 7, 8 and 9 to be the IRQ-EPs and the
   * EP-Pairs 1/2, 3/4, 5/6 to be the bulk EPs for each device.
   *
   * This means, that
   *
   *   - the Composite device uses EP0 as the control-Endpoint,
   *   - the CDC/ACM 0 uses EP7, EP1 and EP2,
   *   - the CDC/ACM 1 uses EP8, EP3 and EP4,
   *   - the CDC/ACM 2 uses EP9, EP5 and EP6
   *
   * as its EP-Configuration.
   */

  else if (configid == 1)
    {
      struct composite_devdesc_s dev[3];
      int strbase = COMPOSITE_NSTRIDS;
      int ifnobase = 0;
      int ia;

      for (ia = 0; ia < 3; ia++)
        {
          /* Ask the cdcacm driver to fill in the
           * constants we didn't know here
           */

          cdcacm_get_composite_devdesc(&dev[ia]);

          /* Overwrite and correct some values... */

          /* The callback functions for the CDC/ACM class */

          dev[ia].classobject = cdcacm_classobject;
          dev[ia].uninitialize = cdcacm_uninitialize;

          dev[ia].minor = ia;                         /* The minor interface number */

          /* Interfaces */

          dev[ia].devinfo.ifnobase = ifnobase;        /* Offset to Interface-IDs */

          /* Strings */

          dev[ia].devinfo.strbase = strbase;          /* Offset to String Numbers */

          /* Endpoints */

          dev[ia].devinfo.epno[CDCACM_EP_INTIN_IDX]   = 7 + ia;
          dev[ia].devinfo.epno[CDCACM_EP_BULKIN_IDX]  = 1 + ia * 2;
          dev[ia].devinfo.epno[CDCACM_EP_BULKOUT_IDX] = 2 + ia * 2;

          ifnobase += dev[ia].devinfo.ninterfaces;
          strbase  += dev[ia].devinfo.nstrings;
        }

      return composite_initialize(composite_getdevdescs(), dev, 3);
    }
  else
    {
      return NULL;
    }
}

#endif /* CONFIG_BOARDCTL_USBDEVCTRL && CONFIG_USBDEV_COMPOSITE */
