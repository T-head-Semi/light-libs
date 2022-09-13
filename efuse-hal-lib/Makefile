##
 # Copyright (C) 2021 Alibaba Group Holding Limited
##

ifeq ("$(BUILD_SYSTEM)","YOCTO_BUILD")
  export PATH_TO_SYSROOT=${SYSROOT_DIR}
  export TOOLSCHAIN_PATH=${TOOLCHAIN_DIR}
  export TOOLCHAIN_HOST=${CROSS_COMPILE}
else
  export PATH_TO_SYSROOT=${BUILDROOT_DIR}/output/host/riscv64-buildroot-linux-gnu/sysroot
  export TOOLSCHAIN_PATH=${BUILDROOT_DIR}/output/host
  export TOOLCHAIN_HOST=${TOOLSCHAIN_PATH}/bin/riscv64-unknown-linux-gnu-
endif

default: efuse_lib efuse_test

efuse_lib:
	make -C lib/src ARCH=$(ARCH) CROSS=$(CROSS_COMPILE)

efuse_test: efuse_lib
	make -C test/efuse_demo ARCH=$(ARCH) CROSS=$(CROSS_COMPILE)

.PHONY: clean
clean: clean_lib clean_test

clean_lib:
	make -C lib/src clean

clean_test:
	make -C test/efuse_demo clean
