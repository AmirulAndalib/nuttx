/****************************************************************************
 * arch/risc-v/src/common/espressif/esp_spiflash_mtd.c
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
#include <debug.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <inttypes.h>
#include <errno.h>

#include <nuttx/arch.h>
#include <nuttx/init.h>
#include <nuttx/mutex.h>
#include <nuttx/mtd/mtd.h>

#include "esp_attr.h"
#include "esp_spiflash.h"
#include "esp_rom_spiflash.h"
#include "esp_rom_spiflash_defs.h"
#include "esp_spiflash_mtd.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MTD_BLK_SIZE                CONFIG_ESPRESSIF_SPIFLASH_MTD_BLKSIZE
#define MTD_ERASE_SIZE              4096
#define MTD_ERASED_STATE            (0xff)

#define MTD2PRIV(_dev)              ((struct esp_mtd_dev_s *)_dev)
#define MTD_SIZE(_priv)             ((*(_priv)->data)->chip.chip_size)
#define MTD_BLK2SIZE(_priv, _b)     (MTD_BLK_SIZE * (_b))
#define MTD_SIZE2BLK(_priv, _s)     ((_s) / MTD_BLK_SIZE)

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* ESP SPI Flash device private data  */

struct esp_mtd_dev_s
{
  struct mtd_dev_s mtd;

  /* SPI Flash data */

  const esp_rom_spiflash_legacy_data_t **data;
};

/****************************************************************************
 * Private Functions Prototypes
 ****************************************************************************/

/* MTD driver methods */

static int esp_erase(struct mtd_dev_s *dev, off_t startblock,
                     size_t nblocks);
static ssize_t esp_read(struct mtd_dev_s *dev, off_t offset,
                        size_t nbytes, uint8_t *buffer);
static ssize_t esp_bread(struct mtd_dev_s *dev, off_t startblock,
                         size_t nblocks, uint8_t *buffer);
static ssize_t esp_write(struct mtd_dev_s *dev, off_t offset,
                         size_t nbytes, const uint8_t *buffer);
static ssize_t esp_bwrite(struct mtd_dev_s *dev, off_t startblock,
                          size_t nblocks, const uint8_t *buffer);
static int esp_ioctl(struct mtd_dev_s *dev, int cmd,
                     unsigned long arg);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct esp_mtd_dev_s g_esp_spiflash =
{
  .mtd =
          {
            .erase  = esp_erase,
            .bread  = esp_bread,
            .bwrite = esp_bwrite,
            .read   = esp_read,
            .ioctl  = esp_ioctl,
#ifdef CONFIG_MTD_BYTE_WRITE
            .write  = esp_write,
#endif
            .name   = "esp_spiflash"
          },
  .data = (const esp_rom_spiflash_legacy_data_t **)&rom_spiflash_legacy_data,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: esp_erase
 *
 * Description:
 *   Erase SPI Flash designated sectors.
 *
 * Input Parameters:
 *   dev        - MTD device data
 *   startblock - start block number, it is not equal to SPI Flash's block
 *   nblocks    - Number of blocks
 *
 * Returned Value:
 *   Erased blocks if success or a negative value if fail.
 *
 ****************************************************************************/

static int esp_erase(struct mtd_dev_s *dev, off_t startblock,
                     size_t nblocks)
{
  ssize_t ret;
  uint32_t offset = startblock * MTD_ERASE_SIZE;
  uint32_t nbytes = nblocks * MTD_ERASE_SIZE;
  struct esp_mtd_dev_s *priv = (struct esp_mtd_dev_s *)dev;
  irqstate_t flags;

  if ((offset > MTD_SIZE(priv)) || ((offset + nbytes) > MTD_SIZE(priv)))
    {
      return -EINVAL;
    }

#ifdef CONFIG_ESPRESSIF_STORAGE_MTD_DEBUG
  finfo("%s(%p, 0x%x, %d)\n", __func__, dev, startblock, nblocks);

  finfo("spi_flash_erase_range(0x%x, %d)\n", offset, nbytes);
#endif

  flags = enter_critical_section();
  ret = spi_flash_erase_range(offset, nbytes);
  leave_critical_section(flags);

  if (ret == OK)
    {
      ret = nblocks;
    }
  else
    {
#ifdef CONFIG_ESPRESSIF_STORAGE_MTD_DEBUG
      finfo("Failed to erase the flash range!\n");
#endif
      ret = -1;
    }

#ifdef CONFIG_ESPRESSIF_STORAGE_MTD_DEBUG
  finfo("%s()=%d\n", __func__, ret);
#endif

  return ret;
}

/****************************************************************************
 * Name: esp_read
 *
 * Description:
 *   Read data from SPI Flash at designated address.
 *
 * Input Parameters:
 *   dev    - MTD device data
 *   offset - target address offset
 *   nbytes - data number
 *   buffer - data buffer pointer
 *
 * Returned Value:
 *   Read data bytes if success or a negative value if fail.
 *
 ****************************************************************************/

static ssize_t esp_read(struct mtd_dev_s *dev, off_t offset,
                        size_t nbytes, uint8_t *buffer)
{
  ssize_t ret;
  irqstate_t flags;

#ifdef CONFIG_ESPRESSIF_STORAGE_MTD_DEBUG
  finfo("%s(%p, 0x%x, %d, %p)\n", __func__, dev, offset, nbytes, buffer);

  finfo("spi_flash_read(0x%x, %p, %d)\n", offset, buffer, nbytes);
#endif

  flags = enter_critical_section();
  ret = spi_flash_read(offset, (uint32_t *)buffer, nbytes);
  leave_critical_section(flags);

  if (ret == OK)
    {
      ret = nbytes;
    }

#ifdef CONFIG_ESPRESSIF_STORAGE_MTD_DEBUG
  finfo("%s()=%d\n", __func__, ret);
#endif

  return ret;
}

/****************************************************************************
 * Name: esp_bread
 *
 * Description:
 *   Read data from designated blocks.
 *
 * Input Parameters:
 *   dev        - MTD device data
 *   startblock - start block number, it is not equal to SPI Flash's block
 *   nblocks    - blocks number
 *   buffer     - data buffer pointer
 *
 * Returned Value:
 *   Read block number if success or a negative value if fail.
 *
 ****************************************************************************/

static ssize_t esp_bread(struct mtd_dev_s *dev, off_t startblock,
                         size_t nblocks, uint8_t *buffer)
{
  ssize_t ret;
  uint32_t addr = startblock * MTD_BLK_SIZE;
  uint32_t size = nblocks * MTD_BLK_SIZE;
  irqstate_t flags;

#ifdef CONFIG_ESPRESSIF_STORAGE_MTD_DEBUG
  finfo("%s(%p, 0x%x, %d, %p)\n", __func__, dev, startblock, nblocks,
        buffer);

  finfo("spi_flash_read(0x%x, %p, %d)\n", addr, buffer, size);
#endif

  flags = enter_critical_section();
  ret = spi_flash_read(addr, (uint32_t *)buffer, size);
  leave_critical_section(flags);

  if (ret == OK)
    {
      ret = nblocks;
    }

#ifdef CONFIG_ESPRESSIF_STORAGE_MTD_DEBUG
  finfo("%s()=%d\n", __func__, ret);
#endif

  return ret;
}

/****************************************************************************
 * Name: esp_write
 *
 * Description:
 *   write data to SPI Flash at designated address.
 *
 * Input Parameters:
 *   dev    - MTD device data
 *   offset - target address offset
 *   nbytes - data number
 *   buffer - data buffer pointer
 *
 * Returned Value:
 *   Written bytes if success or a negative value if fail.
 *
 ****************************************************************************/

static ssize_t esp_write(struct mtd_dev_s *dev, off_t offset,
                         size_t nbytes, const uint8_t *buffer)
{
  ssize_t ret;
  struct esp_mtd_dev_s *priv = (struct esp_mtd_dev_s *)dev;
  irqstate_t flags;

  ASSERT(buffer);

  if ((offset > MTD_SIZE(priv)) || ((offset + nbytes) > MTD_SIZE(priv)))
    {
      return -EINVAL;
    }

#ifdef CONFIG_ESPRESSIF_STORAGE_MTD_DEBUG
  finfo("%s(%p, 0x%x, %d, %p)\n", __func__, dev, offset, nbytes, buffer);

  finfo("spi_flash_write(0x%x, %p, %d)\n", offset, buffer, nbytes);
#endif

  flags = enter_critical_section();
  ret = spi_flash_write(offset, (uint32_t *)buffer, nbytes);
  leave_critical_section(flags);

  if (ret == OK)
    {
      ret = nbytes;
    }

#ifdef CONFIG_ESPRESSIF_STORAGE_MTD_DEBUG
  finfo("%s()=%d\n", __func__, ret);
#endif

  return ret;
}

/****************************************************************************
 * Name: esp_bwrite
 *
 * Description:
 *   Write data to designated blocks.
 *
 * Input Parameters:
 *   dev        - MTD device data
 *   startblock - start MTD block number,
 *                it is not equal to SPI Flash's block
 *   nblocks    - blocks number
 *   buffer     - data buffer pointer
 *
 * Returned Value:
 *   Written block number if success or a negative value if fail.
 *
 ****************************************************************************/

static ssize_t esp_bwrite(struct mtd_dev_s *dev, off_t startblock,
                          size_t nblocks, const uint8_t *buffer)
{
  ssize_t ret;
  uint32_t addr = startblock * MTD_BLK_SIZE;
  uint32_t size = nblocks * MTD_BLK_SIZE;
  irqstate_t flags;

#ifdef CONFIG_ESPRESSIF_STORAGE_MTD_DEBUG
  finfo("%s(%p, 0x%x, %d, %p)\n", __func__, dev, startblock,
        nblocks, buffer);

  finfo("spi_flash_write(0x%x, %p, %d)\n", addr, buffer, size);
#endif

  flags = enter_critical_section();
  ret = spi_flash_write(addr, (uint32_t *)buffer, size);
  leave_critical_section(flags);

  if (ret == OK)
    {
      ret = nblocks;
    }

#ifdef CONFIG_ESPRESSIF_STORAGE_MTD_DEBUG
  finfo("%s()=%d\n", __func__, ret);
#endif

  return ret;
}

/****************************************************************************
 * Name: esp_ioctl
 *
 * Description:
 *   Set/Get option to/from ESP SPI Flash MTD device data.
 *
 * Input Parameters:
 *   dev - ESP MTD device data
 *   cmd - operation command
 *   arg - operation argument
 *
 * Returned Value:
 *   0 if success or a negative value if fail.
 *
 ****************************************************************************/

static int esp_ioctl(struct mtd_dev_s *dev, int cmd,
                     unsigned long arg)
{
  int ret = OK;
  finfo("cmd: %d\n", cmd);

  switch (cmd)
    {
      case MTDIOC_GEOMETRY:
        {
          struct esp_mtd_dev_s *priv = (struct esp_mtd_dev_s *)dev;
          struct mtd_geometry_s *geo = (struct mtd_geometry_s *)arg;
          if (geo)
            {
              memset(geo, 0, sizeof(*geo));

              geo->blocksize    = MTD_BLK_SIZE;
              geo->erasesize    = MTD_ERASE_SIZE;
              geo->neraseblocks = MTD_SIZE(priv) / MTD_ERASE_SIZE;
              ret               = OK;

              finfo("blocksize: %" PRId32 " erasesize: %" PRId32 \
                    " neraseblocks: %" PRId32 "\n",
                    geo->blocksize, geo->erasesize, geo->neraseblocks);
            }
        }
        break;

      case BIOC_PARTINFO:
        {
          struct esp_mtd_dev_s *priv = (struct esp_mtd_dev_s *)dev;
          struct partition_info_s *info = (struct partition_info_s *)arg;
          if (info != NULL)
            {
              info->numsectors  = MTD_SIZE(priv) / MTD_BLK_SIZE;
              info->sectorsize  = MTD_BLK_SIZE;
              info->startsector = 0;
              info->parent[0]   = '\0';
            }
        }
        break;

      case MTDIOC_ERASESTATE:
        {
          uint8_t *result = (uint8_t *)arg;
          *result = MTD_ERASED_STATE;

          ret = OK;
        }
        break;

      default:
        ret = -ENOTTY;
        break;
    }

  finfo("return %d\n", ret);
  return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: esp_spiflash_alloc_mtdpart
 *
 * Description:
 *   Allocate an MTD partition from the ESP SPI Flash.
 *
 * Input Parameters:
 *   mtd_offset - MTD Partition offset from the base address in SPI Flash.
 *   mtd_size   - Size for the MTD partition.
 *
 * Returned Value:
 *   ESP SPI Flash MTD data pointer if success or NULL if fail.
 *
 ****************************************************************************/

struct mtd_dev_s *esp_spiflash_alloc_mtdpart(uint32_t mtd_offset,
                                             uint32_t mtd_size)
{
  const struct esp_mtd_dev_s *priv;
  const esp_rom_spiflash_chip_t *chip;
  struct mtd_dev_s *mtd_part;
  uint32_t blocks;
  uint32_t startblock;
  uint32_t size;

  priv = &g_esp_spiflash;

  chip = &(*priv->data)->chip;

  finfo("ESP SPI Flash information:\n");
  finfo("\tID = 0x%" PRIx32 "\n", chip->device_id);
  finfo("\tStatus mask = 0x%" PRIx32 "\n", chip->status_mask);
  finfo("\tChip size = %" PRId32 " KB\n", chip->chip_size / 1024);
  finfo("\tPage size = %" PRId32 " B\n", chip->page_size);
  finfo("\tSector size = %" PRId32 " KB\n", chip->sector_size / 1024);
  finfo("\tBlock size = %" PRId32 " KB\n", chip->block_size / 1024);

  ASSERT((mtd_offset + mtd_size) <= chip->chip_size);
  ASSERT((mtd_offset % chip->sector_size) == 0);
  ASSERT((mtd_size % chip->sector_size) == 0);

  if (mtd_size == 0)
    {
      size = chip->chip_size - mtd_offset;
    }
  else
    {
      size = mtd_size;
    }

  finfo("\tMTD offset = 0x%" PRIx32 "\n", mtd_offset);
  finfo("\tMTD size = 0x%" PRIx32 "\n", size);

  startblock = MTD_SIZE2BLK(priv, mtd_offset);
  blocks = MTD_SIZE2BLK(priv, size);

  mtd_part = mtd_partition((struct mtd_dev_s *)&priv->mtd, startblock,
                           blocks);
  if (!mtd_part)
    {
      ferr("ERROR: Failed to create MTD partition\n");
      return NULL;
    }

  return mtd_part;
}

/****************************************************************************
 * Name: esp_spiflash_mtd
 *
 * Description:
 *   Get SPI Flash MTD.
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   ESP SPI Flash MTD pointer.
 *
 ****************************************************************************/

struct mtd_dev_s *esp_spiflash_mtd(void)
{
  struct esp_mtd_dev_s *priv =
      (struct esp_mtd_dev_s *)&g_esp_spiflash;

  return &priv->mtd;
}
