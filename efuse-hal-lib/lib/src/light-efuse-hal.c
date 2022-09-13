// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2021 Alibaba Group Holding Limited.
 *
 */
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include "efuse-api.h"

//#define DEBUG_INFO

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x)			(sizeof(x) / sizeof((x)[0]))
#endif

#define EFUSE_LIT_BLOCK_BIT_WIDTH	128
#define EFUSE_BIG_BLOCK_BIT_WIDTH	256
#define EFUSE_BYTES_PER_LIT_BLOCK	(EFUSE_LIT_BLOCK_BIT_WIDTH >> 3)
#define EFUSE_BYTES_PER_BIG_BLOCK	(EFUSE_BIG_BLOCK_BIT_WIDTH >> 3)

struct func_efuse_info {
	const char *func_name;
	unsigned int block_id;
	unsigned int addr;		/* in byte */
	size_t	len;			/* in byte */
	unsigned int shift;		/* left-shift bit within a byte*/
	unsigned int mask;		/* mask within a byte */
};

struct func_efuse_info efuse_func_array[] = {
	{"UID",				5,	0x50,	20,	0,	0xff},
	{"USR_DSP0_JTAG_MODE",		8,	0x86,	1,	0,	0xff},
	{"USR_DSP1_JTAG_MODE",		8,	0x87,	1,	0,	0xff},
	{"USR_C910T_JTAG_MODE",		8,	0x88,	1,	0,	0xff},
	{"USR_C910R_JTAG_MODE",		8,	0x89,	1,	0,	0xff},
	{"USR_C906_JTAG_MODE",		8,	0x8a,	1,	0,	0xff},
	{"USR_E902_JTAG_MODE",		8,	0x8b,	1,	0,	0xff},
	{"USR_CHIP_DBG_MODE",		8,	0x8c,	1,	0,	0xff},
	{"USR_DFT_MODE",		8,	0x8d,	1,	0,	0xff},
	{"BOOT_OFFSET",			9,	0x90,	4,	0,	0xff},
	{"BOOT_INDEX",			9,	0x94,	1,	0,	0xff},
	{"BOOT_OFFSET_BAK",		9,	0x95,	4,	0,	0xff},
	{"BOOT_INDEX_BAK",		9,	0x99,	1,	0,	0xff},
	{"USR_USB_FASTBOOT_DIS",	9,	0x9a,	1,	4,	0x0f},
	{"USR_BROM_CCT_DIS",		9,      0x9a,   1,      0,      0x0f},
	{"IMAGE_BL2_ENC",		9,	0x9b,	1,	0,	0xff},
	{"IMAGE_BL3_ENC",		9,	0x9c,	1,	0,	0xff},
	{"IMAGE_BL4_ENC",		9,	0x9d,	1,	0,	0xff},
	{"BL1VERSION",			10,	0xa0,	8,	0,	0xff},
	{"BL2VERSION",			10,	0xa8,	8,	0,	0xff},
	{"SECURE_BOOT",			1,	0x10,	1,	0,	0xff},
	{"HASH_DEBUGPK",		25,	0x190,	32,	0,	0xff},
	{"BROM_DCACHE_EN",		1,	0x12,	1,	2,	0x3},
	{"GMAC0_MAC",			11,	0xb0,	6,	0,	0xff},
	{"GMAC1_MAC",			11,	0xb8,	6,	0,	0xff},
	{},
};

static const char *efuse_file = "/sys/bus/nvmem/devices/light-efuse0/nvmem";


size_t efuse_read(int fd, void *buf, const char *func_name)
{
	unsigned int offset, mask, shift;
	size_t len, ret;
	int i, block;
	const char *name;
	off_t pos;

	for (i = 0; i < ARRAY_SIZE(efuse_func_array); i++) {
		if (!strcmp(func_name, efuse_func_array[i].func_name))
			break;
	}

	if (i >= ARRAY_SIZE(efuse_func_array)) {
		printf("invalid efuse function name(%s)\n", name);
		return -EINVAL;
	}

	block	= efuse_func_array[i].block_id;
	name	= efuse_func_array[i].func_name;
	offset	= efuse_func_array[i].addr;
	len	= efuse_func_array[i].len;
	shift	= efuse_func_array[i].shift;
	mask	= efuse_func_array[i].mask;
#ifdef DEBUG_INFO
	printf("[efuse info]: block: %d, name: %s, addr: 0x%x, len: %d mask: 0x%x\n",
			block, name, offset, (int)len, mask);
#endif
	pos = lseek(fd, offset, SEEK_SET);
	if (pos < 0) {
		perror("failed to lseek offset to read");
		return -errno;
	}

#ifdef DEBUG_INFO
	printf("efuse pos = %p\n", pos);
#endif
	if (mask != 0xff) {
		unsigned char data;
		unsigned char rd_buf[len];

		ret = read(fd, rd_buf, len);
		if (ret != len) {
#ifdef DEBUG_INFO
			printf("real read len: %d, expected read len: %d\n", ret, len);
#endif
			perror("failed to read");
			ret = -errno;
			goto out;
		}
		data = (rd_buf[0] >> shift) & mask;
#ifdef DEBUG_INFO
		printf("data = 0x%x\n", data);
#endif
		memcpy(buf, &data, 1);

		if (len > 1) {
			len = len - 1;
			buf = (unsigned char *)buf + 1;
			memcpy(buf, &rd_buf[1], len);
		}
	} else { /* mask == 0xff */
		ret = read(fd, buf, len);
		if (ret != len) {
#ifdef DEBUG_INFO
			printf("real read len: %d, expected read len: %d\n", ret, len);
#endif
			perror("failed to read");
			ret = -errno;
		}
	}

out:
	return ret;
}

size_t efuse_write(int fd, const void *buf, const char *func_name)
{
	unsigned int offset, mask, shift;
	size_t len, ret;
	int i, block;
	const char *name;
	off_t pos;

	for (i = 0; i < ARRAY_SIZE(efuse_func_array); i++) {
		if (!strcmp(func_name, efuse_func_array[i].func_name))
			break;
	}

	if (i >= ARRAY_SIZE(efuse_func_array)) {
		printf("invalid efuse function name(%s)\n", name);
		return -EINVAL;
	}

	block	= efuse_func_array[i].block_id;
	name	= efuse_func_array[i].func_name;
	offset	= efuse_func_array[i].addr;
	len	= efuse_func_array[i].len;
	shift	= efuse_func_array[i].shift;
	mask	= efuse_func_array[i].mask;
#ifdef DEBUG_INFO
	printf("efuse info: block: %d, name: %s, addr: 0x%x, len: %d, mask: 0x%x\n",
			block, name, offset, (int)len, mask);
#endif

	pos = lseek(fd, offset, SEEK_SET);
	if (pos < 0) {
		perror("failed to lseek offset to read");
		return -errno;
	}
#ifdef DEBUG_INFO
	printf("efuse pos = %p\n", pos);
#endif
	if (mask != 0xff) {
		unsigned char data;
		unsigned char wr_buf[len];

		memcpy(wr_buf, buf, len);

		ret = read(fd, &data, 1);
		if (ret < 0) {
			perror("failed to read");
			ret = -errno;
			goto out;
		}

		data &= ~(mask << shift);
		data |= (wr_buf[0] & mask) << shift;
		memcpy(&wr_buf[0], &data, 1);

		if (lseek(fd, offset, SEEK_SET) == (off_t)-1) {
			perror("failed to lseek offset to write");
			ret = -errno;
			goto out;
		}

		ret = write(fd, wr_buf, len);
		if (ret != len) {
			perror("failed to write");
			ret = -errno;
		}
	} else { /* mask == 0xff */
		ret = write(fd, buf, len);
		if (ret != len) {
			perror("failed to write");
			ret = -errno;
		}
	}

out:
	return ret;
}

/* according to FuseMap_v1.2.1.xlsx */
size_t efuse_block_read(int fd, unsigned char *buf, unsigned int block_num)
{
	unsigned int width, offset, bytes;
	size_t ret;

	if (block_num >= 42 && block_num <= 47)
		width = EFUSE_BIG_BLOCK_BIT_WIDTH;
	else
		width = EFUSE_LIT_BLOCK_BIT_WIDTH;

	if (block_num <= 42)
		offset = block_num * EFUSE_BYTES_PER_LIT_BLOCK;
	else if (block_num > 42 && block_num <= 48)
		offset = 42 * EFUSE_BYTES_PER_LIT_BLOCK + (block_num - 42) * EFUSE_BYTES_PER_BIG_BLOCK;
	else
		offset = 42 * EFUSE_BYTES_PER_LIT_BLOCK + 6 * EFUSE_BYTES_PER_BIG_BLOCK +
			(block_num - 48) * EFUSE_BYTES_PER_LIT_BLOCK;

	bytes = width / 8;

	if (lseek(fd, offset, SEEK_SET) == (off_t)-1) {
		perror("failed to lseek offset to read");
		return -errno;
	}

	ret = read(fd, buf, bytes);
	if (ret != bytes) {
		perror("failed to read");
		ret = -errno;
	}

	return ret;
}

/* according to FuseMap_v1.2.1.xlsx */
size_t efuse_block_write(int fd, unsigned char *buf, unsigned int block_num)
{
	unsigned int width, offset, bytes;
	size_t ret;

	if (block_num >= 42 && block_num <= 47)
		width = EFUSE_BIG_BLOCK_BIT_WIDTH;
	else
		width = EFUSE_LIT_BLOCK_BIT_WIDTH;

	if (block_num <= 42)
		offset = block_num * EFUSE_BYTES_PER_LIT_BLOCK;
	else if (block_num > 42 && block_num <= 48)
		offset = 42 * EFUSE_BYTES_PER_LIT_BLOCK + (block_num - 42) * EFUSE_BYTES_PER_BIG_BLOCK;
	else
		offset = 42 * EFUSE_BYTES_PER_LIT_BLOCK + 6 * EFUSE_BYTES_PER_BIG_BLOCK +
			(block_num - 48) * EFUSE_BYTES_PER_LIT_BLOCK;

	bytes = width / 8;

	if (lseek(fd, offset, SEEK_SET) == (off_t)-1) {
		perror("failed to lseek offset to write");
		return -errno;
	}

	ret = write(fd, buf, bytes);
	if (ret != bytes) {
		perror("failed to write");
		ret = -errno;
	}

	return ret;
}

/**
 * csi_efuse_get_chipid() - Get chip id in eFuse
 *
 * @chip_id: pointer to the buffer to store chip id
 *
 * Return: 0 on success or negative code on failure
*/
int csi_efuse_get_chipid(void *chip_id)
{
	int ret, fd;

	assert(chip_id);

	fd = open(efuse_file, O_RDONLY);
	if (fd < 0) {
		printf("failed to open efuse device: %s\n", efuse_file);
		perror("open");
		return -errno;
	}

	ret = efuse_read(fd, chip_id, "UID");
	if (ret < 0)
		printf("failed to get 'UID' from efuse\n");
	else
		ret = 0;

	close(fd);

	return ret;
}

/**
 * csi_efuse_get_user_dbg_mode() - Get debug mode in user area
 *
 * @type:	Debug type
 *
 * @dbg_mode:	pointer to the buffer store debug mode
 *
 * Return: 0 on success or negative code on failure
*/
int  csi_efuse_get_user_dbg_mode(efuse_dbg_type_t type, efuse_dbg_mode_t *dbg_mode)
{
	int ret, fd;
	char *mode_name;

	assert(dbg_mode);
	if ((type != USR_DSP0_JTAG) && (type != USR_DSP1_JTAG) &&
			(type != USR_C910T_JTAG) && (type != USR_C910R_JTAG) &&
			(type != USR_C906_JTAG) && (type != USR_E902_JTAG) &&
			(type != USR_CHIP_DBG) && (type != USR_DFT))

		return -EINVAL;

	switch (type) {
	case USR_DSP0_JTAG:
		mode_name = "USR_DSP0_JTAG_MODE";
		break;
	case USR_DSP1_JTAG:
		mode_name = "USR_DSP1_JTAG_MODE";
		break;
	case USR_C910T_JTAG:
		mode_name = "USR_C910T_JTAG_MODE";
		break;
	case USR_C910R_JTAG:
		mode_name = "USR_C910R_JTAG_MODE";
		break;
	case USR_C906_JTAG:
		mode_name = "USR_C906_JTAG_MODE";
		break;
	case USR_E902_JTAG:
		mode_name = "USR_E902_JTAG_MODE";
		break;
	case USR_CHIP_DBG:
		mode_name = "USR_CHIP_DBG_MODE";
		break;
	case USR_DFT:
		mode_name = "USR_DFT_MODE";
		break;
	}

	fd = open(efuse_file, O_RDONLY);
	if (fd < 0) {
		printf("failed to open efuse device: %s\n", efuse_file);
		return -errno;
	}

	ret = efuse_read(fd, dbg_mode, mode_name);
	if (ret < 0)
		printf("failed to get %s from efuse\n", mode_name);
	else
		ret = 0;

	close(fd);

	return ret;
}

/**
 * csi_efuse_set_user_dbg_mode() - Set debug mode in user area
 *
 * @type:	Debug type
 *
 * @dbg_mode:	debug mode
 *
 * Return: 0 on success or negative code on failure
*/
int  csi_efuse_set_user_dbg_mode(efuse_dbg_type_t type, efuse_dbg_mode_t dbg_mode)
{
	int ret, fd;
	char *mode_name;

	if ((type != USR_DSP0_JTAG) && (type != USR_DSP1_JTAG) &&
			(type != USR_C910T_JTAG) && (type != USR_C910R_JTAG) &&
			(type != USR_C906_JTAG) && (type != USR_E902_JTAG) &&
			(type != USR_CHIP_DBG) && (type != USR_DFT))
		return -EINVAL;

	if ((dbg_mode != DBG_MODE_ENABLE) && (dbg_mode != DBG_MODE_PWD_PROTECT) &&
			(dbg_mode != DBG_MODE_DISABLE))
		return -EINVAL;

	switch (type) {
	case USR_DSP0_JTAG:
		mode_name = "USR_DSP0_JTAG_MODE";
		break;
	case USR_DSP1_JTAG:
		mode_name = "USR_DSP1_JTAG_MODE";
		break;
	case USR_C910T_JTAG:
		mode_name = "USR_C910T_JTAG_MODE";
		break;
	case USR_C910R_JTAG:
		mode_name = "USR_C910R_JTAG_MODE";
		break;
	case USR_C906_JTAG:
		mode_name = "USR_C906_JTAG_MODE";
		break;
	case USR_E902_JTAG:
		mode_name = "USR_E902_JTAG_MODE";
		break;
	case USR_CHIP_DBG:
		mode_name = "USR_CHIP_DBG_MODE";
		break;
	case USR_DFT:
		mode_name = "USR_DFT_MODE";
		break;
	}

	fd = open(efuse_file, O_RDWR);
	if (fd < 0) {
		printf("failed to open efuse device: %s\n", efuse_file);
		return -errno;
	}

	ret = efuse_write(fd, (unsigned char *)&dbg_mode, mode_name);
	if (ret < 0)
		printf("failed to set %s into efuse\n", mode_name);
	else
		ret = 0;

	close(fd);

	return ret;
}

/**
 * csi_efuse_get_boot_offset() - Get BL1's offset in boot media
 *
 * @offset:	pointer to the buffer to store BL1's offset
 *
 * Return: 0 on success or negative code on failure
*/
int csi_efuse_get_boot_offset(unsigned int *offset)
{
	int ret, fd;

	assert(offset);

	fd = open(efuse_file, O_RDONLY);
	if (fd < 0) {
		printf("failed to open efuse device: %s\n", efuse_file);
		return -errno;
	}

	ret = efuse_read(fd, offset, "BOOT_OFFSET");
	if (ret < 0)
		printf("failed to get 'BOOT_OFFSET' from efuse\n");
	else
		ret = 0;

	close(fd);

	return ret;
}

/**
 * csi_efuse_set_boot_offset() - Set BL1's offset in boot media
 *
 * @offset:	Offset value to be set
 *
 * Return: 0 on success or negative code on failure
*/
int  csi_efuse_set_boot_offset(unsigned int  offset)
{
	int ret, fd;

	fd = open(efuse_file, O_RDWR);
	if (fd < 0) {
		printf("failed to open efuse device: %s\n", efuse_file);
		return -errno;
	}

	ret = efuse_write(fd, &offset, "BOOT_OFFSET");
	if (ret < 0)
		printf("failed to set 'BOOT_OFFSET' into efuse\n");
	else
		ret = 0;

	close(fd);

	return ret;
}

/**
 * csi_efuse_get_boot_index() - Get BL1's index in boot media
 *
 * @index:	pointer to the buffer to store BL1's index
 *
 * Return: 0 on success or negative code on failure
*/
int  csi_efuse_get_boot_index(unsigned char *index)
{
	int ret, fd;

	assert(index);

	fd = open(efuse_file, O_RDONLY);
	if (fd < 0) {
		printf("failed to open efuse device: %s\n", efuse_file);
		return -errno;
	}

	ret = efuse_read(fd, index, "BOOT_INDEX");
	if (ret < 0)
		printf("failed to get 'BOOT_INDEX' from efuse\n");
	else
		ret = 0;

	close(fd);

	return ret;
}

/**
 * csi_efuse_set_boot_index() - Set BL1's index in boot media
 *
 * @index:	index value to be set
 *
 * Return: 0 on success or negative code on failure
*/
int  csi_efuse_set_boot_index(const unsigned char index)
{
	int ret, fd;

	fd = open(efuse_file, O_RDWR);
	if (fd < 0) {
		printf("failed to open efuse device: %s\n", efuse_file);
		return -errno;
	}

	ret = efuse_write(fd, &index, "BOOT_INDEX");
	if (ret < 0)
		printf("failed to set 'BOOT_INDEX' into efuse\n");
	else
		ret = 0;

	close(fd);

	return ret;
}

/**
 * csi_efuse_get_bak_boot_offset() - Get BL1's offset in boot media
 *
 * @offset:	pointer to the buffer to store BL1's offset
 *
 * Return: 0 on success or negative code on failure
*/
int csi_efuse_get_bak_boot_offset(unsigned int *offset)
{
	int ret, fd;

	assert(offset);

	fd = open(efuse_file, O_RDONLY);
	if (fd < 0) {
		printf("failed to open efuse device: %s\n", efuse_file);
		return -errno;
	}

	ret = efuse_read(fd, offset, "BOOT_OFFSET_BAK");
	if (ret < 0)
		printf("failed to get 'BOOT_OFFSET_BAK' from efuse\n");
	else
		ret = 0;

	close(fd);

	return ret;
}

/**
 * csi_efuse_set_boot_offset() - Set BL1's offset in boot media
 *
 * @offset:	Offset value to be set
 *
 * Return: 0 on success or negative code on failure
*/
int  csi_efuse_set_bak_boot_offset(unsigned int  offset)
{
	int ret, fd;

	fd = open(efuse_file, O_RDWR);
	if (fd < 0) {
		printf("failed to open efuse device: %s\n", efuse_file);
		return -errno;
	}

	ret = efuse_write(fd, &offset, "BOOT_OFFSET_BAK");
	if (ret < 0)
		printf("failed to set 'BOOT_OFFSET_BAK' into efuse\n");
	else
		ret = 0;

	close(fd);

	return ret;
}

/**
 * csi_efuse_get_bak_boot_index() - Get BL1's index in boot media
 *
 * @index:	pointer to the buffer to store BL1's index
 *
 * Return: 0 on success or negative code on failure
*/
int  csi_efuse_get_bak_boot_index(unsigned char *index)
{
	int ret, fd;

	assert(index);

	fd = open(efuse_file, O_RDONLY);
	if (fd < 0) {
		printf("failed to open efuse device: %s\n", efuse_file);
		return -errno;
	}

	ret = efuse_read(fd, index, "BOOT_INDEX_BAK");
	if (ret < 0)
		printf("failed to get 'BOOT_INDEX_BAK' from efuse\n");
	else
		ret = 0;

	close(fd);

	return ret;
}

/**
 * csi_efuse_set_bak_boot_index() - Set BL1's index in boot media
 *
 * @index:	index value to be set
 *
 * Return: 0 on success or negative code on failure
*/
int  csi_efuse_set_bak_boot_index(unsigned char index)
{
	int ret, fd;

	fd = open(efuse_file, O_RDWR);
	if (fd < 0) {
		printf("failed to open efuse device: %s\n", efuse_file);
		return -errno;
	}

	ret = efuse_write(fd, &index, "BOOT_INDEX_BAK");
	if (ret < 0)
		printf("failed to set 'BOOT_INDEX_BAK' into efuse\n");
	else
		ret = 0;

	close(fd);

	return ret;
}

/**
 * csi_efuse_get_usr_brom_usb_fastboot_st() - Get bootrom  USB fastboots tatus in user area
 *
 * @status:	pointer to the buffer to store  USB fastboot status
 *
 * Return: 0 on success or negative code on failure
*/
int  csi_efuse_get_usr_brom_usb_fastboot_st(brom_usbboot_st_t *status)
{
	int ret, fd;
	unsigned char tempdata;
	assert(status);

	fd = open(efuse_file, O_RDONLY);
	if (fd < 0) {
		printf("failed to open efuse device: %s\n", efuse_file);
		return -errno;
	}

	ret = efuse_read(fd, &tempdata, "USR_USB_FASTBOOT_DIS");
	if (ret < 0)
		printf("failed to get 'USR_USB_FASTBOOT_DIS' from efuse\n");
	else
		ret = 0;

	if (tempdata == 0xa)
		*status = BROM_USBBOOT_DIS;
	else if (tempdata != BROM_USBBOOT_EN)
		ret = -EINVAL;
	else
		*status = BROM_USBBOOT_EN;

	close(fd);

	return ret;
}

/**
 * csi_efuse_dis_usr_brom_usb_fastboot() - Disable bootrom USB fastboot status in user area
 *
 * Return: 0 on success or negative code on failure
*/
int  csi_efuse_dis_usr_brom_usb_fastboot(void)
{
	int ret, fd;
	unsigned char status = 0xA;	/* disable value */

	fd = open(efuse_file, O_RDWR);
	if (fd < 0) {
		printf("failed to open efuse device: %s\n", efuse_file);
		return -errno;
	}

	ret = efuse_write(fd, &status, "USR_USB_FASTBOOT_DIS");
	if (ret < 0)
		printf("failed to set 'USR_USB_FASTBOOT_DIS' status into efuse\n");
	else
		ret = 0;

	close(fd);

	return ret;
}

/**
 * csi_efuse_get_usr_brom_cct_st() - Get bootrom CCT status in user area
 *
 * @status:	pointer to the buffer to store CCT
 *
 * Return: 0 on success or negative code on failure
*/
int  csi_efuse_get_usr_brom_cct_st(brom_cct_st_t *status)
{
	int ret, fd;
	unsigned char tempdata;

	assert(status);

	fd = open(efuse_file, O_RDONLY);
	if (fd < 0) {
		printf("failed to open efuse device: %s\n", efuse_file);
		return -errno;
	}

	ret = efuse_read(fd, &tempdata, "USR_BROM_CCT_DIS");
	if (ret < 0)
		printf("failed to get 'USR_BROM_CCT_DIS' from efuse\n");
	else
		ret = 0;

	if (tempdata == 0xa)
		*status = BROM_CCT_DIS;
	else if(tempdata != BROM_CCT_EN)
		ret = -EINVAL;
	else
		*status = BROM_CCT_EN;

	close(fd);

	return ret;
}

/**
 * csi_efuse_dis_usr_brom_cct() - Disable bootrom CCT status in user area
 *
 * Return: 0 on success or negative code on failure
*/
int  csi_efuse_dis_usr_brom_cct(void)
{
	int ret, fd;
	unsigned char status = 0xA;	/* disable value */

	fd = open(efuse_file, O_RDWR);
	if (fd < 0) {
		printf("failed to open efuse device: %s\n", efuse_file);
		return -errno;
	}

	ret = efuse_write(fd, &status, "USR_BROM_CCT_DIS");
	if (ret < 0)
		printf("failed to set 'USR_BROM_CCT_DIS' status into efuse\n");
	else
		ret = 0;

	close(fd);

	return ret;
}

/**
 * csi_efuse_get_bl2_img_encrypt_st() - Get BL2 image encryption flag
 *
 * @encrypt_flag:	pointer to the buffer to store BL2 encryption flag
 *
 * Return: 0 on success or negative code on failure
*/
int  csi_efuse_get_bl2_img_encrypt_st(img_encrypt_st_t *encrypt_flag)
{
	int ret, fd;
	unsigned char tempdata;

	assert(encrypt_flag);

	fd = open(efuse_file, O_RDONLY);
	if (fd < 0) {
		printf("failed to open efuse device: %s\n", efuse_file);
		return -errno;
	}

	ret = efuse_read(fd, &tempdata, "IMAGE_BL2_ENC");
	if (ret < 0)
		printf("failed to get 'IMAGE_BL2_ENC' from efuse\n");
	else
		ret = 0;

	if (tempdata == 0x5a)
		*encrypt_flag = IMAGE_ENCRYPT_EN;
	else if (tempdata != IMAGE_ENCRYPT_DIS)
		ret = -EINVAL;
	else
		*encrypt_flag = IMAGE_ENCRYPT_DIS;

	close(fd);

	return ret;
}

/**
 * csi_efuse_set_bl2_img_encrypt_st() - Set BL2 image encryption flag
 *
 * @encrypt_flag:	pointer to the buffer to store BL2 encryption flag
 *
 * Return: 0 on success or negative code on failure
*/
int  csi_efuse_set_bl2_img_encrypt_st(img_encrypt_st_t encrypt_flag)
{
	int ret, fd;

	if ((encrypt_flag != IMAGE_ENCRYPT_DIS) && (encrypt_flag != IMAGE_ENCRYPT_EN))
		return -EINVAL;

	fd = open(efuse_file, O_RDWR);
	if (fd < 0) {
		printf("failed to open efuse device: %s\n", efuse_file);
		return -errno;
	}

	if (encrypt_flag == IMAGE_ENCRYPT_EN)
		encrypt_flag = 0x5a;

	ret = efuse_write(fd, (unsigned char *)&encrypt_flag, "IMAGE_BL2_ENC");
	if (ret < 0)
		printf("failed to get 'IMAGE_BL2_ENC' from efuse\n");
	else
		ret = 0;

	close(fd);

	return ret;
}

/**
 * csi_efuse_get_bl3_img_encrypt_st() - Get BL3 image encryption flag
 *
 * @encrypt_flag:	pointer to the buffer to store BL2 encryption flag
 *
 * Return: 0 on success or negative code on failure
*/
int  csi_efuse_get_bl3_img_encrypt_st(img_encrypt_st_t *encrypt_flag)
{
	int ret, fd;
	unsigned char tempdata;

	assert(encrypt_flag);

	fd = open(efuse_file, O_RDONLY);
	if (fd < 0) {
		printf("failed to open efuse device: %s\n", efuse_file);
		return -errno;
	}

	ret = efuse_read(fd, &tempdata, "IMAGE_BL3_ENC");
	if (ret < 0)
		printf("failed to get 'IMAGE_BL3_ENC' from efuse\n");
	else
		ret = 0;

	if (tempdata == 0x5a)
		*encrypt_flag = IMAGE_ENCRYPT_EN;
	else if (tempdata != IMAGE_ENCRYPT_DIS)
		ret = -EINVAL;
	else
		*encrypt_flag = IMAGE_ENCRYPT_DIS;

	close(fd);

	return ret;
}

/**
 * csi_efuse_set_bl3_img_encrypt_st() - Set BL3 image encryption flag
 *
 * @encrypt_flag:	pointer to the buffer to store BL3 encryption flag
 *
 * Return: 0 on success or negative code on failure
*/
int  csi_efuse_set_bl3_img_encrypt_st(img_encrypt_st_t encrypt_flag)
{
	int ret, fd;

	if ((encrypt_flag != IMAGE_ENCRYPT_DIS) && (encrypt_flag != IMAGE_ENCRYPT_EN))
		return -EINVAL;

	fd = open(efuse_file, O_RDWR);
	if (fd < 0) {
		printf("failed to open efuse device: %s\n", efuse_file);
		return -errno;
	}

	if (encrypt_flag == IMAGE_ENCRYPT_EN)
		encrypt_flag = 0x5a;

	ret = efuse_write(fd, (unsigned char *)&encrypt_flag, "IMAGE_BL3_ENC");
	if (ret < 0)
		printf("failed to get 'IMAGE_BL3_ENC' from efuse\n");
	else
		ret = 0;

	close(fd);

	return ret;
}

/**
 * csi_efuse_get_bl4_img_encrypt_st() - Get BL4 image encryption flag
 *
 * @encrypt_flag:	pointer to the buffer to store BL4 encryption flag
 *
 * Return: 0 on success or negative code on failure
*/
int  csi_efuse_get_bl4_img_encrypt_st(img_encrypt_st_t *encrypt_flag)
{
	int ret, fd;
	unsigned char tempdata;

	assert(encrypt_flag);

	fd = open(efuse_file, O_RDONLY);
	if (fd < 0) {
		printf("failed to open efuse device: %s\n", efuse_file);
		return -errno;
	}

	ret = efuse_read(fd, &tempdata, "IMAGE_BL4_ENC");
	if (ret < 0)
		printf("failed to get 'IMAGE_BL4_ENC' from efuse\n");
	else
		ret = 0;

	if (tempdata == 0x5a)
		*encrypt_flag = IMAGE_ENCRYPT_EN;
	else if (tempdata != IMAGE_ENCRYPT_DIS)
		ret = -EINVAL;
	else
		*encrypt_flag = IMAGE_ENCRYPT_DIS;


	close(fd);

	return ret;
}

/**
 * csi_efuse_set_bl4_img_encrypt_st() - Set BL4 image encryption flag
 *
 * @encrypt_flag:	pointer to the buffer to store BL4 encryption flag
 *
 * Return: 0 on success or negative code on failure
*/
int  csi_efuse_set_bl4_img_encrypt_st(img_encrypt_st_t encrypt_flag)
{
	int ret, fd;

	if ((encrypt_flag != IMAGE_ENCRYPT_DIS) && (encrypt_flag != IMAGE_ENCRYPT_EN))
		return -EINVAL;

	fd = open(efuse_file, O_RDWR);
	if (fd < 0) {
		printf("failed to open efuse device: %s\n", efuse_file);
		return -errno;
	}

	if (encrypt_flag == IMAGE_ENCRYPT_EN)
		encrypt_flag = 0x5a;

	ret = efuse_write(fd, (unsigned char *)&encrypt_flag, "IMAGE_BL4_ENC");
	if (ret < 0)
		printf("failed to get 'IMAGE_BL4_ENC' from efuse\n");
	else
		ret = 0;

	close(fd);

	return ret;
}

/**
 * csi_efuse_get_bl1_version() - Get BL1 version
 *
 * @version:	pointer to the buffer to store BL1's version in eFuse
 *
 * Return: 0 on success or negative code on failure
*/
int  csi_efuse_get_bl1_version(unsigned long long *version)
{
	int ret, fd;

	assert(version);

	fd = open(efuse_file, O_RDONLY);
	if (fd < 0) {
		printf("failed to open efuse device: %s\n", efuse_file);
		return -errno;
	}

	ret = efuse_read(fd, version, "BL1VERSION");
	if (ret < 0)
		printf("failed to get 'BL1VERSION' from efuse\n");
	else
		ret = 0;

	close(fd);

	return ret;
}

/**
 * csi_efuse_set_bl1_version() - Set BL1 version
 *
 * @version:	pointer to the buffer to store BL1's version in eFuse
 *
 * Return: 0 on success or negative code on failure
*/
int  csi_efuse_set_bl1_version(unsigned long long version)
{
	int ret, fd;

	fd = open(efuse_file, O_RDWR);
	if (fd < 0) {
		printf("failed to open efuse device: %s\n", efuse_file);
		return -errno;
	}

	ret = efuse_write(fd, &version, "BL1VERSION");
	if (ret < 0)
		printf("failed to set 'BL1VERSION' into efuse\n");
	else
		ret = 0;

	close(fd);

	return ret;
}

/**
 * csi_efuse_get_bl2_version() - Get BL2 version
 *
 * @version:	pointer to the buffer to store BL2's version in eFuse
 *
 * Return: 0 on success or negative code on failure
*/
int  csi_efuse_get_bl2_version(unsigned long long *version)
{
	int ret, fd;

	assert(version);

	fd = open(efuse_file, O_RDONLY);
	if (fd < 0) {
		printf("failed to open efuse device: %s\n", efuse_file);
		return -errno;
	}

	ret = efuse_read(fd, version, "BL2VERSION");
	if (ret < 0)
		printf("failed to get 'BL2VERSION' from efuse\n");
	else
		ret = 0;

	close(fd);

	return ret;
}

/**
 * csi_efuse_set_bl2_version() - Set BL2 version
 *
 * @version:	pointer to the buffer to store BL2's version in eFuse
 *
 * Return: 0 on success or negative code on failure
*/
int  csi_efuse_set_bl2_version(unsigned long long version)
{
	int ret, fd;

	fd = open(efuse_file, O_RDWR);
	if (fd < 0) {
		printf("failed to open efuse device: %s\n", efuse_file);
		return -errno;
	}

	ret = efuse_write(fd, &version, "BL2VERSION");
	if (ret < 0)
		printf("failed to set 'BL2VERSION' into efuse\n");
	else
		ret = 0;

	close(fd);

	return ret;
}

/**
 * csi_efuse_get_secure_boot_st() - Get seucre boot flag
 *
 * @sboot_flag:	A pointer to the buffer to store secure boot flag
 *
 * Return: 0 on success or negative code on failure
*/
int  csi_efuse_get_secure_boot_st(sboot_st_t *sboot_flag)
{
	int ret, fd;
	unsigned char tempdata;

	assert(sboot_flag);

	fd = open(efuse_file, O_RDONLY);
	if (fd < 0) {
		printf("failed to open efuse device: %s\n", efuse_file);
		return -errno;
	}

	ret = efuse_read(fd, &tempdata, "SECURE_BOOT");
	if (ret < 0)
		printf("failed to get 'SECURE_BOOT' from efuse\n");
	else
		ret = 0;

	if (tempdata == 0x5a)
		*sboot_flag = SECURE_BOOT_EN;
	else if (tempdata != SECURE_BOOT_DIS)
		ret = -EINVAL;
	else
		*sboot_flag = SECURE_BOOT_DIS;

	close(fd);

	return ret;
}

/**
 * csi_efuse_get_hash_challenge() - Get hash challenge in eFuse
 *
 * @hash_resp:	pointer to the buffer to store hash response
 *
 * Return: 0 on success or negative code on failure
*/
int  csi_efuse_get_hash_challenge(void * hash_resp)
{
	int ret, fd;

	assert(hash_resp);

	fd = open(efuse_file, O_RDONLY);
	if (fd < 0) {
		printf("failed to open efuse device: %s\n", efuse_file);
		return -errno;
	}

	ret = efuse_read(fd, hash_resp, "HASH_DEBUGPK");
	if (ret < 0)
		printf("failed to get 'HASH_DEBUGPK' from efuse\n");
	else
		ret = 0;

	close(fd);

	return ret;
}

/**
 * csi_efuse_get_userdata_group() - Get user data in corresponding eFuse block
 *
 * @key:	pointer to the buffer to store user data
 * @block_num:	the block number in eFuse
 *
 * Return: 0 on success or negative code on failure
*/
int  csi_efuse_get_userdata_group(unsigned char *key, unsigned char block_num)
{
	int ret, fd;

	assert(key);
	if (block_num >= 58)
		return -EINVAL;

	fd = open(efuse_file, O_RDONLY);
	if (fd < 0) {
		printf("failed to open efuse device: %s\n", efuse_file);
		return -errno;
	}

	ret = efuse_block_read(fd, key, block_num);
	if (ret < 0)
		printf("failed to get block%d data from efuse\n", block_num);
	else
		ret = 0;

	close(fd);

	return ret;
}

/**
 * csi_efuse_set_userdata_group() - Set user data corresponding eFuse block
 *
 * @key:	pointer to the buffer to store user data
 * @block_num:	the block number in eFuse
 *
 * Return: 0 on success or negative code on failure
*/
int  csi_efuse_set_userdata_group(unsigned char *key, unsigned char block_num)
{
	int ret, fd;

	assert(key);
	if (block_num > 58) {
		printf("the block number is out of the scop \n");
		return -EINVAL;
	}

	fd = open(efuse_file, O_RDWR);
	if (fd < 0) {
		printf("failed to open efuse device: %s\n", efuse_file);
		return -errno;
	}

	ret = efuse_block_write(fd, key, block_num);
	if (ret < 0)
		printf("failed to set block%d data into efuse\n", block_num);
	else
		ret = 0;

	close(fd);

	return ret;
}

/**
 * csi_efuse_read() - Read data from eFuse
 *
 * @offset:	offset address
 * @data:	Pointer to a buffer storing the data read from eFuse
 * @cnt:	Number of bytes need to be read
 *
 * Return: number of data items read or error code
*/
int  csi_efuse_read(unsigned int offset, void *data, unsigned int cnt)
{
	int ret, fd;

	assert(data);

	fd = open(efuse_file, O_RDONLY);
	if (fd < 0) {
		printf("failed to open efuse device: %s\n", efuse_file);
		return -errno;
	}

	if (lseek(fd, offset, SEEK_SET) == (off_t)-1) {
		perror("failed to lseek offset to read");
		close(fd);
		return -errno;
	}

	ret = read(fd, data, cnt);
	if (ret < 0) {
		perror("failed to read data from efuse");
		ret = -errno;
	}

	close(fd);

	return ret;
}

/**
 * csi_efuse_write() - Write data to eFuse
 *
 * @offset:	offset address
 * @data:	Pointer to a buffer storing the data write to eFuse
 * @cnt:	Number of bytes need to be write
 *
 * Return: number of data items write or error code
*/
int  csi_efuse_write(unsigned int offset, void *data, unsigned int cnt)
{
	int ret, fd;

	assert(data);

	fd = open(efuse_file, O_RDWR);
	if (fd < 0) {
		printf("failed to open efuse device: %s\n", efuse_file);
		return -errno;
	}

	if (lseek(fd, offset, SEEK_SET) == (off_t)-1) {
		perror("failed to lseek offset to write");
		close(fd);
		return -errno;
	}

	ret = write(fd, data, cnt);
	if (ret < 0) {
		perror("failed to write data to efuse");
		ret = -errno;
	}

	close(fd);

	return ret;
}

/**
 * csi_dbg_enable_c910t_jtag() - Enable C910 TEE core jtag
 *
 * Return: 0 on success or negative code on failure
*/
int csi_dbg_enable_c910t_jtag(void)
{

}

/**
 * csi_dbg_disable_c910t_jtag() - Disable C910 TEE core jtag
 *
 * Return: 0 on success or negative code on failure
*/
int csi_dbg_disable_c910t_jtag(void)
{

}

/**
 * csi_efuse_get_gmac_macaddr() - Get gmac0/gmac1 mac address in eFuse
 * @dev_id: '0' means gmac0, '1' means gmac1
 * @mac: the mac address string
 *
 * Return: 0: Success others: Failed
*/
int csi_efuse_get_gmac_macaddr(int dev_id, unsigned char *mac)
{
	int ret, fd;
	char gmac_name[12] = {};

	assert(mac);
	if ((dev_id != 0) && (dev_id != 1))
		return -EINVAL;

	fd = open(efuse_file, O_RDONLY);
	if (fd < 0) {
		printf("failed to open efuse device: %s\n", efuse_file);
		return -errno;
	}

	sprintf(gmac_name, "GMAC%d_MAC", dev_id);

	ret = efuse_read(fd, mac, gmac_name);
	if (ret < 0)
		printf("failed to get %s\n", gmac_name);
	else
		ret = 0;

	close(fd);

	return ret;
}

/**
 * csi_efuse_set_gmac_macaddr() - Set gmac0/gmac1 mac address in eFuse
 * @dev_id: '0' means gmac0, '1' means gmac1
 * @mac: the mac address string
 *
 * Return: 0: Success others: Failed
*/
int csi_efuse_set_gmac_macaddr(int dev_id, unsigned char *mac)
{
	int ret, fd;
	char gmac_name[12] = {};

	assert(mac);
	if ((dev_id != 0) && (dev_id != 1))
		return -EINVAL;

	fd = open(efuse_file, O_RDWR);
	if (fd < 0) {
		printf("failed to open efuse device: %s\n", efuse_file);
		return -errno;
	}

	sprintf(gmac_name, "GMAC%d_MAC", dev_id);

	ret = efuse_write(fd, mac, gmac_name);
	if (ret < 0)
		printf("failed to set '%s' into efuse\n", gmac_name);
	else
		ret = 0;

	close(fd);

	return ret;
}

/**
 * csi_efuse_update_lc_rma() - Upate efuse life cycle RMA
 *
 * Return: 0: Success others: Failed
 */
int csi_efuse_update_lc_rma()
{
	int fd, ret;
	char d = '1';
	const char *dev_path = "/sys/devices/platform/soc/ffff210000.efuse/rma_lc";

	fd = open(dev_path, O_WRONLY);
	if (fd < 0) {
		printf("failed to open device '%s'\n", dev_path);
		return -errno;
	}
	ret = write(fd, &d, 1);
	if (ret < 0){
		printf("failed to update rma lifecycle\n");
		return -errno;
	} else {
		ret = 0;
	}
	close(fd);

	return 0;
}

/**
 * csi_efuse_update_lc_rma() - Upate efuse life cycle RIP
 *
 * Return: 0: Success others: Failed
 */
int csi_efuse_update_lc_rip()
{
	int fd, ret;
	char d = '1';
	const char *dev_path = "/sys/devices/platform/soc/ffff210000.efuse/rip_lc";

	fd = open(dev_path, O_WRONLY);
	if (fd < 0) {
		printf("failed to open device '%s'\n", dev_path);
		return -errno;
	}
	ret = write(fd, &d, 1);
	if (ret < 0){
		printf("failed to update rip lifecycle\n");
		return -errno;
	} else {
		ret = 0;
	}
	close(fd);

	return 0;
}

