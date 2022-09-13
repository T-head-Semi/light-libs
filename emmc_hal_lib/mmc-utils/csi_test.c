#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include "csi_rpmb.h"

#define HALF_SECTOR_BYTES	256
#define BLOCK_NUM		2
#define BLOCK_START		2

uint8_t key[] = {0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 0xa, 0xa, 0xb, 0xb, 0xc, 0xc, 0xd, 0xd, 0xe, 0xe, 0xf, 0xf};

#define DO_IO(func, fd, buf, nbyte)					\
	({												\
		ssize_t ret = 0, r;							\
		do {										\
			r = func(fd, buf + ret, nbyte - ret);	\
			if (r < 0 && errno != EINTR) {			\
				ret = -1;							\
				break;								\
			}										\
			else if (r > 0)							\
				ret += r;							\
		} while (r != 0 && (size_t)ret != nbyte);	\
													\
		ret;										\
	})

int main()
{
	int i, key_fd;
	hal_error_t ret;
	csi_hal_rpmb_ctx_t rpmb_ctx = { 0 };
	unsigned char write_data[HALF_SECTOR_BYTES * BLOCK_NUM], read_data[HALF_SECTOR_BYTES * BLOCK_NUM] = { 0 };
	char *devicefile = "/dev/mmcblk0rpmb";

	for (i = 0; i < (HALF_SECTOR_BYTES * BLOCK_NUM); i++) {
		if (i < 256)
			write_data[i] = i % 256;
		else
			write_data[i] = 256 - (i % 256);
	}

	//memcpy(rpmb_ctx.key_mac, key, sizeof(key));
	key_fd = open("/home/root/rpmbkey", O_RDONLY);
	if (key_fd < 0) {
		perror("can't open key file");
		exit(1);
	}

	ret = DO_IO(read, key_fd, rpmb_ctx.key_mac, sizeof(rpmb_ctx.key_mac));
	if (ret < 0) {
		perror("read the key");
		exit(1);
	} else if (ret != sizeof(key)) {
		printf("Auth key must be %lu bytes length, but we read only %d, exit\n",
			   (unsigned long)sizeof(rpmb_ctx.key_mac),
			   ret);
		exit(1);
	}

	printf("Key ascii:\n");
	for (i = 0; i < sizeof(rpmb_ctx.key_mac); i++)
		printf("%d", rpmb_ctx.key_mac[i]);

	printf("\n");

	printf("Key character:\n");
	for (i = 0; i < sizeof(rpmb_ctx.key_mac); i++)
		printf("%c", rpmb_ctx.key_mac[i]);

	printf("\n");

	rpmb_ctx.rpmb_op_type = MMC_RPMB_WRITE;
	ret = csi_rpmb_init(&rpmb_ctx, devicefile);
	if (ret) {
		printf("failed to init csi rpmb device\n");
		exit(1);
	}

	ret = csi_rpmb_write_block(&rpmb_ctx, BLOCK_START, BLOCK_NUM, write_data);
	if (ret) {
		printf("failed to write rpmb device\n");
		exit(1);
	}

	ret = csi_rpmb_read_block(&rpmb_ctx, BLOCK_START, BLOCK_NUM, read_data);
	if (ret) {
		printf("failed to read rpmb device\n");
		exit(1);
	}

	for (i = 0; i < sizeof(read_data); i++) {
		if (i % 16 == 0)
			printf("\n");
		if (i == 256)
			printf("\n");

		printf("%d\t", read_data[i]);
	}
	printf("\n");

	csi_rpmb_uninit(&rpmb_ctx);

	return 0;
}
