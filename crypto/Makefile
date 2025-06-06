############################################################################
# crypto/Makefile
#
# SPDX-License-Identifier: Apache-2.0
#
# Licensed to the Apache Software Foundation (ASF) under one or more
# contributor license agreements.  See the NOTICE file distributed with
# this work for additional information regarding copyright ownership.  The
# ASF licenses this file to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance with the
# License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
# License for the specific language governing permissions and limitations
# under the License.
#
############################################################################

include $(TOPDIR)/Make.defs

ifeq ($(CONFIG_CRYPTO),y)

# Basic

CRYPTO_CSRCS += crypto.c testmngr.c

# cryptodev support

ifeq ($(CONFIG_CRYPTO_CRYPTODEV),y)
  CRYPTO_CSRCS += cryptodev.c
ifeq ($(CONFIG_CRYPTO_CRYPTODEV_SOFTWARE),y)
  CRYPTO_CSRCS += cryptosoft.c
  CRYPTO_CSRCS += xform.c
endif
endif

# Software crypto algorithm

ifeq ($(CONFIG_CRYPTO_SW_AES),y)
  CRYPTO_CSRCS += aes.c
endif
CRYPTO_CSRCS += blake2s.c
CRYPTO_CSRCS += blf.c
CRYPTO_CSRCS += cast.c
CRYPTO_CSRCS += chachapoly.c
CRYPTO_CSRCS += ecb_enc.c
CRYPTO_CSRCS += ecb3_enc.c
CRYPTO_CSRCS += set_key.c
CRYPTO_CSRCS += md5.c
CRYPTO_CSRCS += poly1305.c
CRYPTO_CSRCS += rijndael.c
CRYPTO_CSRCS += rmd160.c
CRYPTO_CSRCS += sha1.c
CRYPTO_CSRCS += sha2.c
CRYPTO_CSRCS += gmac.c
CRYPTO_CSRCS += cmac.c
CRYPTO_CSRCS += hmac.c
ifeq ($(CONFIG_CRYPTO_RANDOM_POOL),y)
  CRYPTO_CSRCS += idgen.c
  CRYPTO_CSRCS += curve25519.c
endif
CRYPTO_CSRCS += key_wrap.c
CRYPTO_CSRCS += siphash.c
CRYPTO_CSRCS += hmac_buff.c
CRYPTO_CSRCS += bn.c

# Entropy pool random number generator

ifeq ($(CONFIG_CRYPTO_RANDOM_POOL),y)
  CRYPTO_CSRCS += random_pool.c
endif

endif # CONFIG_CRYPTO

ASRCS = $(CRYPTO_ASRCS)
AOBJS = $(ASRCS:.S=$(OBJEXT))

CSRCS = $(CRYPTO_CSRCS)
COBJS = $(CSRCS:.c=$(OBJEXT))

SRCS = $(ASRCS) $(CSRCS)
OBJS = $(AOBJS) $(COBJS)

BIN = libcrypto$(LIBEXT)

all: $(BIN)
.PHONY: depend clean distclean

$(AOBJS): %$(OBJEXT): %.S
	$(call ASSEMBLE, $<, $@)

$(COBJS): %$(OBJEXT): %.c
	$(call COMPILE, $<, $@)

$(BIN): $(OBJS)
	$(call ARCHIVE, $@, $(OBJS))

makedepfile: $(CSRCS:.c=.ddc) $(ASRCS:.S=.dds)
	$(call CATFILE, Make.dep, $^)
	$(call DELFILE, $^)

.depend: Makefile $(SRCS) $(TOPDIR)$(DELIM).config
ifeq ($(CONFIG_CRYPTO),y)
	$(Q) $(MAKE) makedepfile
endif
	$(Q) touch $@

depend: .depend

clean:
	$(call DELFILE, $(BIN))
	$(call CLEAN)

distclean: clean
	$(call DELFILE, Make.dep)
	$(call DELFILE, .depend)

-include Make.dep
