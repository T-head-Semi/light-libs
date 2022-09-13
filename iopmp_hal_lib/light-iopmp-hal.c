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
#include "light-iopmp.h"

static const char *light_iopmp_tap = "/sys/devices/platform/iopmp/light_iopmp_tap";
static const char *light_iopmp_start_addr = "/sys/devices/platform/iopmp/light_iopmp_start_addr";
static const char *light_iopmp_end_addr = "/sys/devices/platform/iopmp/light_iopmp_end_addr";
static const char *light_iopmp_attr = "/sys/devices/platform/iopmp/light_iopmp_attr";
static const char *light_iopmp_lock = "/sys/devices/platform/iopmp/light_iopmp_lock";
static const char *light_iopmp_set = "/sys/devices/platform/iopmp/light_iopmp_set";

/**
 * @brief Light iopmp region permission setting.
 *
 * @param type
 * @param attr
 * @return csi_err_t
 */
int csi_iopmp_set_attr(int type, u_int8_t *start_addr, u_int8_t *end_addr, csi_iopmp_attr_t attr)
{
	int ret = 0;
	int light_iopmp_tap_fd, light_iopmp_start_addr_fd, light_iopmp_end_addr_fd;
	int light_iopmp_attr_fd, light_iopmp_lock_fd, light_iopmp_set_fd;

	light_iopmp_tap_fd = open(light_iopmp_tap, O_RDWR);
	if (light_iopmp_tap_fd < 0) {
		perror("open tap");
		exit(1);
	} else {
		char string[5];
		sprintf(string, "%d\n", type);
		if (write(light_iopmp_tap_fd, string, 5) != 5)
			perror("write(tap)");
		close(light_iopmp_tap_fd);
	}

	light_iopmp_start_addr_fd = open(light_iopmp_start_addr, O_RDWR);
	if (light_iopmp_start_addr_fd < 0) {
		perror("open start_addr");
		exit(1);
	} else {
		char string[20];
		int start = (int)((int64_t)start_addr >> 12);
		sprintf(string, "%d\n", start);
		if (write(light_iopmp_start_addr_fd, string, 20) != 20)
			perror("write(start_addr)");
		close(light_iopmp_start_addr_fd);
	}

	light_iopmp_end_addr_fd = open(light_iopmp_end_addr, O_RDWR);
	if (light_iopmp_end_addr_fd < 0) {
		perror("open end_addr");
		exit(1);
	} else {
		int end = (int)((int64_t)end_addr >> 12);
		char string[20];
		sprintf(string, "%d\n", end);
		if (write(light_iopmp_end_addr_fd, string, 20) != 20)
			perror("write(end_addr)");
		close(light_iopmp_end_addr_fd);
	}

	light_iopmp_attr_fd = open(light_iopmp_attr, O_RDWR);
	if (light_iopmp_attr_fd < 0) {
		perror("open attr");
		exit(1);
	} else {
		char string[20];
		sprintf(string, "%d\n", attr);
		if (write(light_iopmp_attr_fd, string, 20) != 20)
			perror("write(attr)");
		close(light_iopmp_attr_fd);
	}

	light_iopmp_lock_fd = open(light_iopmp_lock, O_RDWR);
	if (light_iopmp_lock_fd < 0) {
		perror("open lock");
		exit(1);
	} else {
		if (write(light_iopmp_lock_fd, "1\n", 2) != 2)
			perror("write(lock)");
		close(light_iopmp_lock_fd);
	}

	light_iopmp_set_fd = open(light_iopmp_set, O_WRONLY);
	if (light_iopmp_set_fd < 0) {
		perror("open set");
		exit(1);
	} else {
		if (write(light_iopmp_set_fd, "1\n", 2) != 2)
			perror("write(set)");
		close(light_iopmp_set_fd);
	}

	return ret;
}

/** iopmp lock
 * @brief  Lock secure iopmp setting.
 *
 * @return csi_err_t
 */
int csi_iopmp_lock(void)
{
	/* dummy API, csi_iopmp_set_attr() should lock iopmp */
	return 0;
}

#if 0 /* demo */
int main(int argc, char *argv[])
{
	csi_iopmp_set_attr(2, (u_int8_t *)0x0, (u_int8_t *)0x200000000, 0xffff);
	return 1;
}
#endif
