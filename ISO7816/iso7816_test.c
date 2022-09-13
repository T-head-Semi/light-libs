#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include "dsmart_card_interface.h"
#include "ioctl.h"

int main()
{
	int ret, i;
	struct dsmart_card_baud baud_data;
	unsigned int protocol = DSMART_CARD_PROTOCOL_T0;
	struct dsmart_card_atr  atr = { 0 };
	struct dsmart_card_rcv rcv_data = {};
	struct dsmart_card_xmt xmt_data = {};
	struct dsmart_card_timing timing_data;

	int fd = open("/dev/dsmart_card", O_RDWR);
	if (fd < 0) {
		perror("failed to open iso7816 smart card\n");
		exit(1);
	}

	printf("dsmart card cmd = 0x%x\n", DSMART_CARD_IOCTL_COLD_RESET);
	ret = ioctl(fd, DSMART_CARD_IOCTL_COLD_RESET, &atr);
	if (ret < 0) {
		printf("failed to get atr from slave card(%d)\n", ret);
		exit(1);
	}

	ret = ioctl(fd, DSMART_CARD_IOCTL_ATR_RCV, (unsigned long)&atr);
	if (ret < 0) {
		printf("failed to get atr data from slave card(%d)\n", ret);
		exit(1);
	}

	printf("\nATR data length: %d, result: %d, ATR DATA:\n", atr.len, atr.errval);

	for (i = 0; i < atr.len; i++)
		printf("0x%02x ", atr.atr_buffer[i]);

	printf("\n\nset transmision protocol T0\n");
	ret = ioctl(fd, DSMART_CARD_IOCTL_SET_PROTOCOL, &protocol);
	if (ret < 0) {
		printf("failed to set transmision protocol(%d)\n", ret);
		exit(1);
	}

	baud_data.di = 1;
	baud_data.fi = 1;

	printf("\nset baud rate, fi: %d, di: %d\n", baud_data.fi, baud_data.di);
	ret = ioctl(fd, DSMART_CARD_IOCTL_SET_BAUD, &baud_data);
	if (ret < 0) {
		printf("failed to set baud rate(%d)\n", ret);
		exit(1);
	}


	timing_data.wwt = 9600;
	timing_data.bgt = 0;
	timing_data.cwt = 0;
	timing_data.bwt = 0;
	timing_data.egt = 0;

	printf("\nset timming window, wwt: %d, bgt: %d, cwt: %d, bwt: %d, egt: %d\n", timing_data.wwt, timing_data.bgt, timing_data.cwt, timing_data.bwt, timing_data.egt);
	ret = ioctl(fd, DSMART_CARD_IOCTL_SET_TIMING, &timing_data);
	if (ret < 0) {
		printf("failed to set timing window(%d)\n", ret);
		exit(1);
	}

	printf("\nget data from sim card ");
	ret = ioctl(fd, DSMART_CARD_IOCTL_RCV, &rcv_data);
	if (ret < 0) {
		printf("failed to receive data from sim card\n");
		exit(1);
	}

	printf(", len: %d\n", rcv_data.rcv_length);
	for (i = 0; i < rcv_data.rcv_length; i++) {
		if (i % 8 == 0)
			printf("\n");
		printf("0x%x  ", rcv_data.rcv_buffer[i]);
	}


	printf("\nreset the smart card\n");
	ret = ioctl(fd, DSMART_CARD_IOCTL_WARM_RESET, NULL);
	if (ret < 0) {
		printf("failed to reset the smart card(%d)\n", ret);
		exit(1);
	}


	ret = ioctl(fd, DSMART_CARD_IOCTL_ATR_RCV, (unsigned long)&atr);
	if (ret < 0) {
		printf("failed to get atr data from slave card(%d)\n", ret);
		exit(1);
	}

	printf("\n\nATR data length after warm reset: %d, result: %d, ATR DATA:\n", atr.len, atr.errval);
	for (i = 0; i < atr.len; i++)
		printf("0x%02x ", atr.atr_buffer[i]);

	printf("\n\nterminate the session\n");
	ret = ioctl(fd, DSMART_CARD_IOCTL_DEACTIVATE, NULL);
	if (ret < 0) {
		printf("failed to terminate the session\n");
		exit(1);
	}

	printf("\nsucceed to access smart card\n");

	close(fd);

	return 0;
}
