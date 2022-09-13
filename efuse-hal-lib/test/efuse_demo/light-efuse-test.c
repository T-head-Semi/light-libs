#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include "efuse-api.h"

void csi_efuse_offset_test()
{
	int ret, i;
	unsigned char data[32] = {0xff, 0xef, 0xdf, 0xcf, 0xbf, 0xaf, 0x9f, 0x8f, 0x7f, 0x6f, 0x5f, 0x4f, 0x3f, 0x2f, 0x1f, 0x0f, 0xfe, 0xee, 0xde, 0xce, 0xbe, 0xae, 0x9e, 0x8e, 0x7e, 0x6e, 0x5e, 0x4e, 0x3e, 0x2e, 0x3f, 0xff};
	unsigned int addr = 50;

	printf("data before set:\n");

	for (i = 0; i < 32; i++)
		printf("0x%x ", data[i]);
	printf("\n");

#if 1
	ret = csi_efuse_write(addr, &data, sizeof(data));
	if (ret < 0)
		return;
	memset(data, 0, 32);
#endif
	ret = csi_efuse_read(addr, &data, sizeof(data));
	if (ret < 0)
		return;

	printf("data after set:\n");

	for (i = 0; i < 32; i++)
		printf("0x%x ", data[i]);
	printf("\n");
}

void csi_efuse_userdata_group_test()
{
	unsigned char group[16] = {0};
	int ret, i;

	for (i = 0; i < 16; i++)
		group[i] = i;

	ret = csi_efuse_set_userdata_group(group, 12);
	if (ret < 0)
		return;

	memset(group, 0, sizeof(group));

	ret = csi_efuse_get_userdata_group(group, 12);
	if (ret < 0)
		return;
#if 0
	for (i = 0; i < 16; i++)
		printf("%x", group[i]);

	ret = csi_efuse_set_userdata_group(group, 44);
	if (ret < 0)
		return;
#endif
	memset(group, 0, sizeof(group));

	ret = csi_efuse_get_userdata_group(group, 44);
	if (ret < 0)
		return;

	for (i = 0; i < 16; i++)
		printf("%x", group[i]);

	printf("\n");
}

void csi_efuse_get_hash_challenge_test()
{
	unsigned char hash[32] = {0};
	int ret, i;

	ret =csi_efuse_get_hash_challenge(&hash);
	if (ret < 0)
		return;

	printf("hash challenge: 0x");
	for (i = 31; i >=0; i--)
		printf("%x", hash[i]);
	printf("\n");
}

void csi_efuse_get_secure_boot_st_test()
{
	sboot_st_t flag;
	int ret;

	ret =csi_efuse_get_secure_boot_st(&flag);
	if (ret < 0)
		return;
	printf("secure boot flag: 0x%x\n", flag);

	if (flag != SECURE_BOOT_DIS && flag != SECURE_BOOT_EN)
		printf("incorrect secure boot flag\n");
}

void csi_efuse_get_bl2_version_test()
{
	unsigned long long version;
	int ret;

	ret = csi_efuse_get_bl2_version(&version);
	if (ret < 0)
		return;
	printf("before setting bl2 version = 0x%llx\n", version);

	version = 0xabcdef123456789a;
	ret = csi_efuse_set_bl2_version(version);
	if (ret < 0)
		return;

	ret = csi_efuse_get_bl2_version(&version);
	if (ret < 0)
		return;
	printf("after setting bl2 version = 0x%llx\n", version);
}

void csi_efuse_get_bl1_version_test()
{
	unsigned long long version;
	int ret;

	ret = csi_efuse_get_bl1_version(&version);
	if (ret < 0)
		return;
	printf("before setting bl1 version = 0x%llx\n", version);

	version = 0xabcdef123456789a;
	ret = csi_efuse_set_bl1_version(version);
	if (ret < 0)
		return;

	ret = csi_efuse_get_bl1_version(&version);
	if (ret < 0)
		return;
	printf("after setting bl1 version = 0x%llx\n", version);
}

void csi_efuse_bl4_img_encrypt_test()
{
	img_encrypt_st_t flag;
	int ret;

	ret = csi_efuse_get_bl4_img_encrypt_st(&flag);
	if (ret < 0)
		return;
	printf("before setting bl4 img flg: 0x%x\n", flag);

	flag = IMAGE_ENCRYPT_EN;
	ret = csi_efuse_set_bl4_img_encrypt_st(flag);
	if (ret < 0)
		return;

	ret = csi_efuse_get_bl4_img_encrypt_st(&flag);
	if (ret < 0)
		return;
	printf("after setting bl4 img flg: 0x%x\n", flag);
}

void csi_efuse_bl3_img_encrypt_test()
{
	img_encrypt_st_t flag;
	int ret;

	ret = csi_efuse_get_bl3_img_encrypt_st(&flag);
	if (ret < 0)
		return;
	printf("before setting bl3 img flg: 0x%x\n", flag);

	flag = IMAGE_ENCRYPT_EN;
	ret = csi_efuse_set_bl3_img_encrypt_st(flag);
	if (ret < 0)
		return;

	ret = csi_efuse_get_bl3_img_encrypt_st(&flag);
	if (ret < 0)
		return;
	printf("after setting bl3 img flg: 0x%x\n", flag);
}

void csi_efuse_bl2_img_encrypt_test()
{
	img_encrypt_st_t flag;
	int ret;

	ret = csi_efuse_get_bl2_img_encrypt_st(&flag);
	if (ret < 0)
		return;
	printf("before setting bl2 img flg: 0x%x\n", flag);

	flag = IMAGE_ENCRYPT_EN;
	ret = csi_efuse_set_bl2_img_encrypt_st(flag);
	if (ret < 0)
		return;

	ret = csi_efuse_get_bl2_img_encrypt_st(&flag);
	if (ret < 0)
		return;
	printf("after setting bl2 img flg: 0x%x\n", flag);
}

void csi_efuse_usr_brom_cct_test()
{
	brom_cct_st_t status;
	int ret;

	printf("csi_efuse_usr_brom_cct_test....\n");

	ret = csi_efuse_get_usr_brom_cct_st(&status);
	if (ret < 0)
		return;
	printf("before setting cct status: 0x%x\n", status);

	ret = csi_efuse_dis_usr_brom_cct();
	if (ret < 0)
		return;

	ret = csi_efuse_get_usr_brom_cct_st(&status);
	if (ret < 0)
		return;

	printf("after setting cct status: 0x%x\n", status);
}

void csi_efuse_usr_brom_usb_fastboot_test()
{
	brom_usbboot_st_t status;
	int ret;

	ret = csi_efuse_get_usr_brom_usb_fastboot_st(&status);
	if (ret < 0)
		return;
	printf("before setting fastboot status: 0x%x\n", status);

	ret = csi_efuse_dis_usr_brom_usb_fastboot();
	if (ret < 0)
		return;

	ret = csi_efuse_get_usr_brom_usb_fastboot_st(&status);
	if (ret < 0)
		return;

	printf("after setting fastboot status: 0x%x\n", status);
}

void csi_efuse_bak_boot_index_test()
{
	int ret;
	unsigned char index;

	ret = csi_efuse_get_bak_boot_index(&index);
	if (ret < 0)
		return;

	printf("\nbefore write bak boot index = %d\n", index);

	index = 50;
	ret = csi_efuse_set_bak_boot_index(index);
	if (ret < 0)
		return;

	ret = csi_efuse_get_bak_boot_index(&index);
	if (ret < 0)
		return;

	printf("\nafter write bak boot index = %d\n", index);
}

void csi_efuse_bak_boot_offset_test()
{
	int ret;
	unsigned int offset;

	ret = csi_efuse_get_bak_boot_offset(&offset);
	if (ret < 0)
		return;

	printf("\nbefore write bak boot offset = %d\n", offset);

	offset = 122;
	ret = csi_efuse_set_bak_boot_offset(offset);
	if (ret < 0)
		return;

	ret = csi_efuse_get_bak_boot_offset(&offset);
	if (ret < 0)
		return;

	printf("\nafter write bak boot offset = %d\n", offset);
}

void csi_efuse_boot_index_test()
{
	unsigned char index;
	int ret;
#if 1
	ret = csi_efuse_set_boot_index(244);
	if (ret < 0)
		return;
#endif
	ret = csi_efuse_get_boot_index(&index);
	if (ret < 0)
		return;

	printf("\nboot index = %d\n", index);
}

void csi_efuse_boot_offset_test()
{
	int ret;
	unsigned int offset = 56;
#if 1
	ret = csi_efuse_set_boot_offset(offset);
	if (ret < 0)
		return;
#endif
	ret = csi_efuse_get_boot_offset(&offset);
	if (ret < 0)
		return;

	printf("\noffset = 0x%x\n", offset);
}

void csi_efuse_user_dbg_mode_test()
{
	int ret;

	efuse_dbg_type_t type = USR_CHIP_DBG;
	efuse_dbg_mode_t mode = DBG_MODE_PWD_PROTECT;

	ret = csi_efuse_set_user_dbg_mode(type, mode);
	if (ret < 0)
		return;

	ret = csi_efuse_get_user_dbg_mode(type, &mode);
	if (ret < 0)
		return;

	if (mode != DBG_MODE_ENABLE && mode != DBG_MODE_PWD_PROTECT &&
			mode != DBG_MODE_DISABLE) {
		printf("invalid debug mode in efuse\n");
		return;
	}

	printf("\ndebug mode:%d\n", mode);
}

void efuse_chip_id_get_test()
{
	int ret;
	unsigned char uid[20] = {};
#if 0
	ret = csi_efuse_write(0x50, &rw_uid, sizeof(unsigned int));
	if (ret < 0)
		return;
#endif
	ret = csi_efuse_get_chipid(&uid);
	if (ret < 0)
		printf("failed to read uid\n");

	printf("\nuid: ");

	for (int i = 0; i < 20; i++)
		printf("%x", uid[i]);

	printf("\n");
}

void csi_efuse_update_lc_rma_test()
{
	csi_efuse_update_lc_rma();
}

void csi_efuse_update_lc_rip_test()
{
	csi_efuse_update_lc_rip();
}

void csi_efuse_gmac_macaddr_test()
{
	int ret;
	unsigned char mac0[6] = {0x00, 0x22, 0x33, 0x44, 0x55, 0x00};
	unsigned char mac1[6] = {0x00, 0x22, 0x33, 0x44, 0x66, 0x00};
	unsigned char r_mac0[6] = {};
	unsigned char r_mac1[6] = {};

	ret = csi_efuse_set_gmac_macaddr(0, mac0);
	if (ret < 0)
		return;
	ret = csi_efuse_get_gmac_macaddr(0, r_mac0);
	if (ret < 0)
		return;
	printf("gmac mac0 address: %2x:%2x:%2x:%2x:%2x:%2x\n", r_mac0[0], r_mac0[1], r_mac0[2], r_mac0[3], r_mac0[4], r_mac0[5]);

	ret = csi_efuse_set_gmac_macaddr(1, mac1);
	if (ret < 0)
		return;
	ret = csi_efuse_get_gmac_macaddr(1, r_mac1);
	if (ret < 0)
		return;
	printf("gmac mac0 address: %2x:%2x:%2x:%2x:%2x:%2x\n", r_mac1[0], r_mac1[1], r_mac1[2], r_mac1[3], r_mac1[4], r_mac1[5]);
}

int main()
{
#if 0
	csi_efuse_bl4_img_encrypt_test();
	csi_efuse_bl2_img_encrypt_test();
	csi_efuse_bl3_img_encrypt_test();
	csi_efuse_get_bl1_version_test();
	csi_efuse_get_hash_challenge_test();
	csi_efuse_get_secure_boot_st_test();
#endif
	csi_efuse_offset_test();
#if 0
	csi_efuse_usr_brom_cct_test();
	csi_efuse_usr_brom_usb_fastboot_test();
	csi_efuse_boot_index_test();
	csi_efuse_boot_offset_test();
	efuse_chip_id_get_test();
	csi_efuse_get_bl1_version_test();
	csi_efuse_get_bl2_version_test();
	csi_efuse_get_secure_boot_st_test();
	csi_efuse_get_hash_challenge_test();
	csi_efuse_userdata_group_test();
#endif

#if 0
	efuse_chip_id_get_test();
	csi_efuse_user_dbg_mode_test();
	csi_efuse_boot_offset_test();
	csi_efuse_boot_index_test();
	csi_efuse_bak_boot_offset_test();
	csi_efuse_bak_boot_index_test();
	csi_efuse_usr_brom_usb_fastboot_test();
#endif

#if 0
	csi_efuse_usr_brom_cct_test();
	printf("welcome to riscv world!!!\n");

	csi_efuse_bl2_img_encrypt_test();
	csi_efuse_bl3_img_encrypt_test();
	csi_efuse_bl4_img_encrypt_test();
	csi_efuse_get_bl1_version_test();
	csi_efuse_get_bl2_version_test();
	csi_efuse_get_secure_boot_st_test();
	csi_efuse_get_hash_challenge_test();
	csi_efuse_userdata_group_test();
	csi_efuse_offset_test();
#endif

#if 0
	csi_efuse_update_lc_rma_test();
	csi_efuse_update_lc_rip_test();
#endif
#if 0
	csi_efuse_gmac_macaddr_test();
#endif
	return 0;
}
