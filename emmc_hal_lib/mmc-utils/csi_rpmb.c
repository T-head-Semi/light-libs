#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "3rdparty/hmac_sha/hmac_sha2.h"
#include "mmc.h"
#include "csi_rpmb.h"

#ifndef offsetof
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif

struct rpmb_frame {
	u_int8_t  stuff[196];
	u_int8_t  key_mac[32];
	u_int8_t  data[256];
	u_int8_t  nonce[16];
	u_int32_t write_counter;
	u_int16_t addr;
	u_int16_t block_count;
	u_int16_t result;
	u_int16_t req_resp;
};

extern int rpmb_read_counter(int dev_fd, unsigned int *cnt);
extern int do_rpmb_op(int fd, const struct rpmb_frame *frame_in,
		struct rpmb_frame *frame_out, unsigned int out_cnt);

/**
  \brief       Initialize rpmb interface.
  \param[in]   ctx    Context to operate
  \return      Error code
*/
hal_error_t csi_rpmb_init(csi_hal_rpmb_ctx_t *ctx, char *device)
{
	int dev_fd;

	assert(device != NULL);

	dev_fd = open(device, O_RDWR);
	if (dev_fd < 0) {
		perror("device open");
		return CSI_HAL_ERROR;
	}

	ctx->device = device;
	ctx->dev_fd = dev_fd;

	return CSI_HAL_SUCCESS;
}

/**
  \brief       De-initialize rpmb.
  \param[in]   ctx    Context to operate
  \return      None
*/
void csi_rpmb_uninit(csi_hal_rpmb_ctx_t *ctx)
{
	close(ctx->dev_fd);
}

int _do_csi_rpmb_write_key(int dev_fd, uint8_t *key)
{
	int ret;
	struct rpmb_frame frame_in = {
		.req_resp = htobe16(MMC_RPMB_WRITE_KEY)
	}, frame_out;

	/* Read the auth key */
	memcpy(frame_in.key_mac, key, sizeof(frame_in.key_mac));

	/* Execute RPMB op */
	ret = do_rpmb_op(dev_fd, &frame_in, &frame_out, 1);
	if (ret != 0) {
		perror("RPMB ioctl failed");
		exit(1);
	}

	/* Check RPMB response */
	if (frame_out.result != 0) {
		printf("RPMB operation failed, retcode 0x%04x\n",
			   be16toh(frame_out.result));
		exit(1);
	}

	return ret;
}

int do_csi_rpmb_write_block(int dev_fd, enum hal_rpmb_op_type type, uint8_t *data, uint8_t *key_value, uint16_t addr, uint32_t blocks)
{
	int ret;
	unsigned char key[32];
	unsigned int cnt;
	struct rpmb_frame frame_in = {
		.req_resp    = htobe16(MMC_RPMB_WRITE),
		.block_count = htobe16(1)
	}, frame_out;

	assert(data != NULL && key_value != NULL && dev_fd >= 0);
	assert(type == MMC_RPMB_WRITE_KEY || type == MMC_RPMB_WRITE);

	if (type == MMC_RPMB_WRITE_KEY)
		return _do_csi_rpmb_write_key(dev_fd, key_value);

	/* Get key mac */
	memcpy(key, key_value, sizeof(key));

	while (blocks) {
		ret = rpmb_read_counter(dev_fd, &cnt);
		/* Check RPMB response */
		if (ret != 0) {
			printf("RPMB read counter operation failed, retcode 0x%04x\n", ret);
			exit(1);
		}
		frame_in.write_counter = htobe32(cnt);

		/* Get block address */
		frame_in.addr = htobe16(addr);

		/* Read 256b data */
		memcpy(frame_in.data, data, sizeof(frame_in.data));

		/* Calculate HMAC SHA256 */
		hmac_sha256(
			key, sizeof(key),
			frame_in.data, sizeof(frame_in) - offsetof(struct rpmb_frame, data),
			frame_in.key_mac, sizeof(frame_in.key_mac));

		/* Execute RPMB op */
		ret = do_rpmb_op(dev_fd, &frame_in, &frame_out, 1);
		if (ret != 0) {
			perror("RPMB ioctl failed");
			exit(1);
		}

		/* Check RPMB response */
		if (frame_out.result != 0) {
			printf("RPMB operation failed, retcode 0x%04x\n", be16toh(frame_out.result));
			exit(1);
		}

		addr += 1;	/* half sector */
		blocks -= 1;
		data += sizeof(frame_in.data);

		memset(&frame_out, 0, sizeof(frame_out));
		memset(&frame_in, 0, sizeof(frame_in));
		frame_in.req_resp = htobe16(MMC_RPMB_WRITE);
		frame_in.block_count = htobe16(1);
	}

	return ret;
}

int do_csi_rpmb_read_block(int dev_fd, uint8_t *data, uint8_t *key_value, uint16_t addr, uint32_t blocks)
{
	int i, ret;
	unsigned char mac[32];
	hmac_sha256_ctx ctx;
	struct rpmb_frame *frame_out = NULL;
	/*
	 * for reading RPMB, number of blocks is set by CMD23 only, the packet
	 * frame field for that is set to 0. So, the type is not u16 but uint!
	 */
	unsigned int blocks_cnt;
	unsigned char key[32];
	struct rpmb_frame frame_in = {
		.req_resp    = htobe16(MMC_RPMB_READ),
	}, *frame_out_p;

	/* Get key mac */
	memcpy(key, key_value, sizeof(key));

	/* Get block address */
	frame_in.addr = htobe16(addr);

	/* Get blocks count */
	blocks_cnt = blocks;
	if (!blocks_cnt) {
		printf("please, specify valid blocks count number\n");
		exit(1);
	}

	frame_out_p = calloc(sizeof(*frame_out_p), blocks_cnt);
	if (!frame_out_p) {
		printf("can't allocate memory for RPMB outer frames\n");
		exit(1);
	}

	/* Execute RPMB op */
	ret = do_rpmb_op(dev_fd, &frame_in, frame_out_p, blocks_cnt);
	if (ret != 0) {
		perror("RPMB ioctl failed");
		exit(1);
	}

	/* Check RPMB response */
	if (frame_out_p[blocks_cnt - 1].result != 0) {
		printf("RPMB operation failed, retcode 0x%04x\n",
			   be16toh(frame_out_p[blocks_cnt - 1].result));
		exit(1);
	}

	/* Do we have to verify data against key? */
	hmac_sha256_init(&ctx, key, sizeof(key));
	for (i = 0; i < blocks_cnt; i++) {
		frame_out = &frame_out_p[i];
		hmac_sha256_update(&ctx, frame_out->data, sizeof(*frame_out) - offsetof(struct rpmb_frame, data));
	}

	hmac_sha256_final(&ctx, mac, sizeof(mac));

	/* Impossible */
	assert(frame_out);

	/* Compare calculated MAC and MAC from last frame */
	if (memcmp(mac, frame_out->key_mac, sizeof(mac))) {
		printf("RPMB MAC missmatch\n");
		exit(1);
	}

	/* Write data */
	for (i = 0; i < blocks_cnt; i++) {
		struct rpmb_frame *frame_out = &frame_out_p[i];

		memcpy(data, frame_out->data, sizeof(frame_out->data));
		data += sizeof(frame_out->data);
	}

	free(frame_out_p);

	return ret;
}

/**
  \brief       RPMB write data, authenticated data and write to RPMB.
  \param[in]   ctx        Context to operate
  \param[in]   addr [in]        address, address in block.
  \param[in]   blocks [in]      write block number. block size 256 bytes.
  \param[in]   data [in]
  \return      Error code
*/
hal_error_t csi_rpmb_write_block(csi_hal_rpmb_ctx_t *ctx, uint16_t addr,
                                 uint32_t blocks, uint8_t *data)
{
	int ret, dev_fd;
	uint8_t *key_value;
	enum hal_rpmb_op_type type;

	assert(ctx != NULL && data != NULL);

	dev_fd = ctx->dev_fd;
	key_value = ctx->key_mac;
	type = ctx->rpmb_op_type;

	ret = do_csi_rpmb_write_block(dev_fd, type, data, key_value, addr, blocks);
	if (ret)
		return CSI_HAL_ERROR;
	else
		return CSI_HAL_SUCCESS;
}

/**
  \brief       RPMB read data, check .authentication tag and return data.
  \param[in]   ctx        Context to operate
  \param[in]   addr [in]        address
  \param[in]   blocks [in]      write block number.
  \return      Error code
*/
hal_error_t csi_rpmb_read_block(csi_hal_rpmb_ctx_t *ctx, uint16_t addr,
                                uint32_t blocks, uint8_t *data)
{
	int ret, dev_fd;
	uint8_t *key_value;

	assert(ctx != NULL && data != NULL);

	dev_fd = ctx->dev_fd;
	key_value = ctx->key_mac;

	ret = do_csi_rpmb_read_block(dev_fd, data, key_value, addr, blocks);
	if (ret)
		return CSI_HAL_ERROR;
	else
		return CSI_HAL_SUCCESS;
}
