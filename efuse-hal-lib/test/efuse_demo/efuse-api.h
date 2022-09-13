// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2021 Alibaba Group Holding Limited.
 *
 */
#ifndef _EFUSE_API_H
#define _EFUSE_API_H

typedef enum {
	USR_DSP0_JTAG = 0,
	USR_DSP1_JTAG,
	USR_C910T_JTAG,
	USR_C910R_JTAG,
	USR_C906_JTAG,
	USR_E902_JTAG,
	USR_CHIP_DBG,
	USR_DFT,
} efuse_dbg_type_t;

typedef enum {
	DBG_MODE_ENABLE = 0,
	DBG_MODE_PWD_PROTECT,
	DBG_MODE_DISABLE,
} efuse_dbg_mode_t;

typedef enum {
	BROM_USBBOOT_EN= 0,
	BROM_USBBOOT_DIS = 0x5a5a5a5a,
} brom_usbboot_st_t;

typedef enum {
	BROM_CCT_EN= 0,
	BROM_CCT_DIS = 0x5a5a5a5a,
} brom_cct_st_t;

typedef enum {
	IMAGE_ENCRYPT_DIS= 0,
	IMAGE_ENCRYPT_EN = 0x5a5a5a5a,
} img_encrypt_st_t;

typedef enum {
	SECURE_BOOT_DIS= 0,
	SECURE_BOOT_EN = 0x5a5a5a5a,
} sboot_st_t;

/**
 * csi_efuse_get_chipid() - Get chip id in eFuse
 *
 * @chip_id: pointer to the buffer to store chip id
 *
 * Return: 0 on success or negative code on failure
*/
int csi_efuse_get_chipid(void *chip_id);

/**
 * csi_efuse_get_user_dbg_mode() - Get debug mode in user area
 *
 * @type:	Debug type
 *
 * @dbg_mode:	pointer to the buffer store debug mode
 *
 * Return: 0 on success or negative code on failure
*/
int  csi_efuse_get_user_dbg_mode(efuse_dbg_type_t type, efuse_dbg_mode_t *dbg_mode);

/**
 * csi_efuse_set_user_dbg_mode() - Set debug mode in user area
 *
 * @type:	Debug type
 *
 * @dbg_mode:	debug mode
 *
 * Return: 0 on success or negative code on failure
*/
int  csi_efuse_set_user_dbg_mode(efuse_dbg_type_t type, efuse_dbg_mode_t dbg_mode);

/**
 * csi_efuse_get_boot_offset() - Get BL1's offset in boot media
 *
 * @offset:	pointer to the buffer to store BL1's offset
 *
 * Return: 0 on success or negative code on failure
*/
int csi_efuse_get_boot_offset(unsigned int *offset);

/**
 * csi_efuse_set_boot_offset() - Set BL1's offset in boot media
 *
 * @offset:	Offset value to be set
 *
 * Return: 0 on success or negative code on failure
*/
int  csi_efuse_set_boot_offset(unsigned int  offset);

/**
 * csi_efuse_get_boot_index() - Get BL1's index in boot media
 *
 * @index:	pointer to the buffer to store BL1's index
 *
 * Return: 0 on success or negative code on failure
*/
int  csi_efuse_get_boot_index(unsigned char *index);

/**
 * csi_efuse_set_boot_index() - Set BL1's index in boot media
 *
 * @index:	index value to be set
 *
 * Return: 0 on success or negative code on failure
*/
int  csi_efuse_set_boot_index(unsigned char index);

/**
 * csi_efuse_get_bak_boot_offset() - Get BL1's offset in boot media
 *
 * @offset:	pointer to the buffer to store BL1's offset
 *
 * Return: 0 on success or negative code on failure
*/
int csi_efuse_get_bak_boot_offset(unsigned int *offset);

/**
 * csi_efuse_set_boot_offset() - Set BL1's offset in boot media
 *
 * @offset:	Offset value to be set
 *
 * Return: 0 on success or negative code on failure
*/
int  csi_efuse_set_bak_boot_offset(unsigned int  offset);

/**
 * csi_efuse_get_bak_boot_index() - Get BL1's index in boot media
 *
 * @index:	pointer to the buffer to store BL1's index
 *
 * Return: 0 on success or negative code on failure
*/
int  csi_efuse_get_bak_boot_index(unsigned char *index);

/**
 * csi_efuse_set_bak_boot_index() - Set BL1's index in boot media
 *
 * @index:	index value to be set
 *
 * Return: 0 on success or negative code on failure
*/
int  csi_efuse_set_bak_boot_index(unsigned char index);

/**
 * csi_efuse_get_usr_brom_usb_fastboot_st() - Get bootrom  USB fastboots tatus in user area
 *
 * @status:	pointer to the buffer to store  USB fastboot status
 *
 * Return: 0 on success or negative code on failure
*/
int  csi_efuse_get_usr_brom_usb_fastboot_st(brom_usbboot_st_t *status);

/**
 * csi_efuse_dis_usr_brom_usb_fastboot() - Disable bootrom USB fastboot status in user area
 *
 * Return: 0 on success or negative code on failure
*/
int  csi_efuse_dis_usr_brom_usb_fastboot(void);

/**
 * csi_efuse_get_usr_brom_cct_st() - Get bootrom CCT status in user area
 *
 * @status:	pointer to the buffer to store CCT
 *
 * Return: 0 on success or negative code on failure
*/
int  csi_efuse_get_usr_brom_cct_st(brom_cct_st_t *status);

/**
 * csi_efuse_dis_usr_brom_cct() - Disable bootrom CCT status in user area
 *
 * Return: 0 on success or negative code on failure
*/
int  csi_efuse_dis_usr_brom_cct(void);

/**
 * csi_efuse_get_bl2_img_encrypt_st() - Get BL2 image encryption flag
 *
 * @encrypt_flag:	pointer to the buffer to store BL2 encryption flag
 *
 * Return: 0 on success or negative code on failure
*/
int  csi_efuse_get_bl2_img_encrypt_st( img_encrypt_st_t *encrypt_flag);

/**
 * csi_efuse_set_bl2_img_encrypt_st() - Set BL2 image encryption flag
 *
 * @encrypt_flag:	pointer to the buffer to store BL2 encryption flag
 *
 * Return: 0 on success or negative code on failure
*/
int  csi_efuse_set_bl2_img_encrypt_st( img_encrypt_st_t encrypt_flag);

/**
 * csi_efuse_get_bl3_img_encrypt_st() - Get BL3 image encryption flag
 *
 * @encrypt_flag:	pointer to the buffer to store BL2 encryption flag
 *
 * Return: 0 on success or negative code on failure
*/
int  csi_efuse_get_bl3_img_encrypt_st( img_encrypt_st_t *encrypt_flag);

/**
 * csi_efuse_set_bl3_img_encrypt_st() - Set BL3 image encryption flag
 *
 * @encrypt_flag:	pointer to the buffer to store BL3 encryption flag
 *
 * Return: 0 on success or negative code on failure
*/
int  csi_efuse_set_bl3_img_encrypt_st( img_encrypt_st_t encrypt_flag);

/**
 * csi_efuse_get_bl4_img_encrypt_st() - Get BL4 image encryption flag
 *
 * @encrypt_flag:	pointer to the buffer to store BL4 encryption flag
 *
 * Return: 0 on success or negative code on failure
*/
int  csi_efuse_get_bl4_img_encrypt_st( img_encrypt_st_t *encrypt_flag);

/**
 * csi_efuse_set_bl4_img_encrypt_st() - Set BL4 image encryption flag
 *
 * @encrypt_flag:	pointer to the buffer to store BL4 encryption flag
 *
 * Return: 0 on success or negative code on failure
*/
int  csi_efuse_set_bl4_img_encrypt_st( img_encrypt_st_t encrypt_flag);

/**
 * csi_efuse_get_bl1_version() - Get BL1 version
 *
 * @version:	pointer to the buffer to store BL1's version in eFuse
 *
 * Return: 0 on success or negative code on failure
*/
int  csi_efuse_get_bl1_version(unsigned long long *version);

/**
 * csi_efuse_set_bl1_version() - Set BL1 version
 *
 * @version:	pointer to the buffer to store BL1's version in eFuse
 *
 * Return: 0 on success or negative code on failure
*/
int  csi_efuse_set_bl1_version(unsigned long long version);

/**
 * csi_efuse_get_bl2_version() - Get BL2 version
 *
 * @version:	pointer to the buffer to store BL2's version in eFuse
 *
 * Return: 0 on success or negative code on failure
*/
int  csi_efuse_get_bl2_version(unsigned long long *version);

/**
 * csi_efuse_set_bl2_version() - Set BL2 version
 *
 * @version:	pointer to the buffer to store BL2's version in eFuse
 *
 * Return: 0 on success or negative code on failure
*/
int  csi_efuse_set_bl2_version(unsigned long long version);

/**
 * csi_efuse_get_secure_boot_st() - Get seucre boot flag
 *
 * @sboot_flag:	A pointer to the buffer to store secure boot flag
 *
 * Return: 0 on success or negative code on failure
*/
int  csi_efuse_get_secure_boot_st(sboot_st_t *sboot_flag);

/**
 * csi_efuse_get_hash_challenge() - Get hash challenge in eFuse
 *
 * @hash_resp:	pointer to the buffer to store hash response
 *
 * Return: 0 on success or negative code on failure
*/
int  csi_efuse_get_hash_challenge(void * hash_resp);

/**
 * csi_efuse_get_userdata_group() - Get user data in corresponding eFuse block
 *
 * @key:	pointer to the buffer to store user data
 * @block_num:	the block number in eFuse
 *
 * Return: 0 on success or negative code on failure
*/
int  csi_efuse_get_userdata_group(unsigned char *key, unsigned char block_num);

/**
 * csi_efuse_set_userdata_group() - Set user data corresponding eFuse block
 *
 * @key:	pointer to the buffer to store user data
 * @block_num:	the block number in eFuse
 *
 * Return: 0 on success or negative code on failure
*/
int  csi_efuse_set_userdata_group(unsigned char *key, unsigned char block_num);

/**
 * csi_efuse_read() - Read data from eFuse
 *
 * @offset:	offset address
 * @data:	Pointer to a buffer storing the data read from eFuse
 * @cnt:	Number of bytes need to be read
 *
 * Return: number of data items read or error code
*/
int  csi_efuse_read(unsigned int offset, void *data, unsigned int cnt);

/**
 * csi_efuse_write() - Write data to eFuse
 *
 * @offset:	offset address
 * @data:	Pointer to a buffer storing the data write to eFuse
 * @cnt:	Number of bytes need to be write
 *
 * Return: number of data items write or error code
*/
int  csi_efuse_write(unsigned int offset, void *data, unsigned int cnt);

/**
 * csi_dbg_enable_c910t_jtag() - Enable C910 TEE core jtag
 *
 * Return: 0 on success or negative code on failure
*/
int csi_dbg_enable_c910t_jtag(void);

/**
 * csi_dbg_disable_c910t_jtag() - Disable C910 TEE core jtag
 *
 * Return: 0 on success or negative code on failure
*/
int csi_dbg_disable_c910t_jtag(void);

/**
 * csi_efuse_get_gmac_macaddr() - Get gmac0/gmac1 mac address in eFuse
 * @dev_id: '0' means gmac0, '1' means gmac1
 * @mac: the mac address string
 *
 * Return: 0: Success others: Failed
*/
int csi_efuse_get_gmac_macaddr(int dev_id, unsigned char *mac);

/**
 * csi_efuse_set_gmac_macaddr() - Set gmac0/gmac1 mac address in eFuse
 * @dev_id: '0' means gmac0, '1' means gmac1
 * @mac: the mac address string
 *
 * Return: 0: Success others: Failed
*/
int csi_efuse_set_gmac_macaddr(int dev_id, unsigned char *mac);

/**
 * csi_efuse_update_lc_rma() - Upate efuse life cycle RMA
 *
 * Return: 0: Success others: Failed
 */
int csi_efuse_update_lc_rma();

/**
 * csi_efuse_update_lc_rma() - Upate efuse life cycle RIP
 *
 * Return: 0: Success others: Failed
 */
int csi_efuse_update_lc_rip();

#endif
