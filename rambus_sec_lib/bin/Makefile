##############################################################################
# Copyright (c) 2021 by Rambus, Inc. and/or its subsidiaries
# All rights reserved. Unauthorized use (including, without limitation,
# distribution and copying) is strictly prohibited. All use requires,
# and is subject to, explicit written authorization and nondisclosure
# Rambus, Inc. and/or its subsidiaries
#
# For more information or support, please go to our online support system at
# https://sipsupport.rambus.com.
# In case you do not have an account for this system, please send an e-mail
# to sipsupport@rambus.com.
##############################################################################
# check dependencies
ifndef OPENSSL_DIR
$(error OPENSSL_DIR is not set)
endif

# default
all: cipher digest

# cipher executable
cipher:
	${CROSS_COMPILE}gcc -O0 \
	-ggdb \
	 ../src/cipher.c \
	-Wl,-rpath=usr/lib64,-rpath=${OPENSSL_DIR} \
	-I${OPENSSL_DIR}/include \
	-L${OPENSSL_DIR} \
	-lcrypto \
	-o cipher
	ls -la cipher

# digest executable
digest:
	${CROSS_COMPILE}gcc -O0 \
	-ggdb \
	 ../src/digest.c \
	-Wl,-rpath=usr/lib64,-rpath=${OPENSSL_DIR} \
	-I${OPENSSL_DIR}/include \
	-L${OPENSSL_DIR} \
	-lcrypto \
	-o digest
	ls -la digest

clean:
	rm cipher
	rm digest

.PHONY: info
info:
	$(info $$OPENSSL_DIR is [${OPENSSL_DIR}])
