// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2021 Alibaba Group Holding Limited.
 *
 */
#ifndef _LIGHT_IOPMP_API_H
#define _LIGHT_IOPMP_API_H

#define IOPMP_EMMC      0
#define IOPMP_SDIO0     1
#define IOPMP_SDIO1     2
#define IOPMP_USB0      3
#define IOPMP_AO        4
#define IOPMP_AUD       5
#define IOPMP_CHIP_DBG  6
#define IOPMP_EIP120I   7
#define IOPMP_EIP120II  8
#define IOPMP_EIP120III 9
#define IOPMP_ISP0      10
#define IOPMP_ISP1      11
#define IOPMP_DSP       12
#define IOPMP_DW200     13
#define IOPMP_VENC      14
#define IOPMP_VDEC      15
#define IOPMP_G2D       16
#define IOPMP_FCE       17
#define IOPMP_NPU       18
#define IOPMP0_DPU      19
#define IOPMP1_DPU      20
#define IOPMP_GPU       21
#define IOPMP_GMAC1     22
#define IOPMP_GMAC2     23
#define IOPMP_DMAC      24
#define IOPMP_TEE_DMAC  25

typedef enum {
	CSI_ATTR_R	= 1,
	CSI_ATTR_W	= 2,
} csi_iopmp_attr_t;

/**
 * @brief Light iopmp region permission setting.
 *
 * @param type
 * @param attr
 * @return csi_err_t
 */
int csi_iopmp_set_attr(int type, u_int8_t *start_addr, u_int8_t *end_addr, csi_iopmp_attr_t attr);

/** iopmp lock
 * @brief  Lock secure iopmp setting.
 *
 * @return csi_err_t
 */
int csi_iopmp_lock(void);

#endif
