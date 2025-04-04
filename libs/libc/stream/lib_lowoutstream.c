/****************************************************************************
 * libs/libc/stream/lib_lowoutstream.c
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

#ifdef CONFIG_ARCH_LOWPUTC

#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <nuttx/arch.h>

#include "libc.h"

/****************************************************************************
 * Private Functions Prototypes
 ****************************************************************************/

static void lowoutstream_putc(FAR struct lib_outstream_s *self, int ch);
static ssize_t lowoutstream_puts(FAR struct lib_outstream_s *self,
                                 FAR const void *buf, size_t len);

/****************************************************************************
 * Public Data
 ****************************************************************************/

struct lib_outstream_s g_lowoutstream =
{
  0,
  lowoutstream_putc,
  lowoutstream_puts,
  lib_noflush
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: lowoutstream_putc
 ****************************************************************************/

static void lowoutstream_putc(FAR struct lib_outstream_s *self, int ch)
{
  DEBUGASSERT(self);

  up_lowputc(ch);

  if (ch != EOF)
    {
      self->nput++;
    }
}

/****************************************************************************
 * Name: lowoutstream_puts
 ****************************************************************************/

static ssize_t lowoutstream_puts(FAR struct lib_outstream_s *self,
                                 FAR const void *buf, size_t len)
{
  FAR const char *str = (FAR const char *)buf;
  size_t idx          = 0;
  DEBUGASSERT(self);

  while (str[idx] != 0 && idx < len)
    {
      lowoutstream_putc(self, str[idx]);
      idx++;
    }

  return len;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: lib_lowoutstream
 *
 * Description:
 *   Initializes a stream for use with low-level, architecture-specific I/O.
 *
 * Input Parameters:
 *   stream - User allocated, uninitialized instance of struct
 *            lib_lowoutstream_s to be initialized.
 *
 * Returned Value:
 *   None (User allocated instance initialized).
 *
 ****************************************************************************/

void lib_lowoutstream(FAR struct lib_outstream_s *stream)
{
  stream->putc  = lowoutstream_putc;
  stream->puts  = lowoutstream_puts;
  stream->flush = lib_noflush;
  stream->nput  = 0;
}

#endif /* CONFIG_ARCH_LOWPUTC */
