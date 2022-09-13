#ifndef _CSI_RPMB_H
#define _CSI_RPMB_H

#include <stdint.h>

#define CSI_HAL_ERROR		-1
#define CSI_HAL_SUCCESS		0

enum hal_rpmb_op_type {
	MMC_RPMB_WRITE_KEY = 0x01,
	MMC_RPMB_READ_CNT  = 0x02,
	MMC_RPMB_WRITE     = 0x03,
	MMC_RPMB_READ      = 0x04,
};

typedef int	hal_error_t;

typedef struct _csi_hal_rpmb_ctx {
	char *device;
	int dev_fd;
	enum hal_rpmb_op_type rpmb_op_type;
	uint8_t key_mac[32];
}csi_hal_rpmb_ctx_t;


/**
  \brief       Initialize rpmb interface.
  \param[in]   ctx    Context to operate
  \return      Error code
*/
hal_error_t csi_rpmb_init(csi_hal_rpmb_ctx_t *ctx,  char *device);

/**
  \brief       De-initialize rpmb.
  \param[in]   ctx    Context to operate
  \return      None
*/
void csi_rpmb_uninit(csi_hal_rpmb_ctx_t *ctx);

/**
  \brief       RPMB read data, check .authentication tag and return data.
  \param[in]   ctx        Context to operate
  \param[in]   addr [in]        address
  \param[in]   blocks [in]      write block number.
  \return      Error code
*/
hal_error_t csi_rpmb_read_block(csi_hal_rpmb_ctx_t *ctx, uint16_t addr,
                                uint32_t blocks, uint8_t *data);

/**
  \brief       RPMB write data, authenticated data and write to RPMB.
  \param[in]   ctx        Context to operate
  \param[in]   addr [in]        address, address in block.
  \param[in]   blocks [in]      write block number. block size 256 bytes.
  \param[in]   data [in]
  \return      Error code
*/
hal_error_t csi_rpmb_write_block(csi_hal_rpmb_ctx_t *ctx, uint16_t addr,
                                 uint32_t blocks, uint8_t *data);


#endif
