/*-
 * Copyright (c) 2012 Damjan Marion
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <getopt.h>
#include <stdint.h>
#include <libgen.h>

#include "flash32w.h"

#define SOH 0x01
#define STX 0x02
#define EOT 0x04
#define ACK 0x06
#define NAK 0x15

int stm32f_cmd_1_1(uint8_t c, uint8_t *v)
{
	uint8_t cmd[] = {0xAA, 0x01, c, 0x55};
	uint8_t buff[MAX_XFER_SIZE];
	int t;

	serial_send(cmd, sizeof(cmd), &t);
	serial_recv(buff, MAX_XFER_SIZE, &t);
	if ((t != 4) || (buff[0] != 0xBB) || buff[t-1]!=0x55)
		return -1;
	
	*v = buff[2];
	return(0);
}

int stm32f_cmd_1_4(uint8_t c, uint32_t *v)
{
	uint8_t cmd[] = {0xAA, 0x01, c, 0x55};
	uint8_t buff[MAX_XFER_SIZE];
	int t;

	serial_send(cmd, sizeof(cmd), &t);
	serial_recv(buff, MAX_XFER_SIZE, &t);
	if ((t != 7) || (buff[0] != 0xBB) || buff[t-1]!=0x55)
		return -1;
	
	*v = ((buff[5]<<24) | (buff[4]<<16) | (buff[3]<<8) | buff[2]);
	return 0;
}

int stm32f_cmd_2_1(uint8_t c1, uint8_t c2,  uint8_t *v)
{
	uint8_t cmd[] = {0xAA, 0x02, c1, c2, 0x55};
	uint8_t buff[MAX_XFER_SIZE];
	int t;

	serial_send(cmd, sizeof(cmd), &t);
	serial_recv(buff, MAX_XFER_SIZE, &t);
	if ((t != 4) || (buff[0] != 0xBB) || buff[t-1]!=0x55)
		return -1;
	
	*v = (buff[2]);
	return 0;
}

static uint16_t _calc_crc16(uint8_t *data, int count) 
{
	uint16_t crc = 0;
	uint8_t i;

	while(--count >= 0) {
		crc = crc ^ (uint16_t) *data++ << 8;
		for(i=0; i<8; ++i) {
			if (crc & 0x8000)
				crc = crc << 1 ^ 0x1021;
			else
				crc = crc << 1;
		}
	} 
	return crc;
}


int ymodem_send_packet(uint8_t *buff)
{
	int length, t;
	uint16_t crc;

	buff[2] = ~buff[1];
	
	if(buff[0] == STX)
		length = 1024;
	else
		length = 128;

	crc = _calc_crc16(buff+3,length);
	buff[length + 3] = (uint8_t) (crc >> 8);
	buff[length + 4] = (uint8_t) (crc & 0xff);

	while(1) {
		serial_send(buff, length + 5, &t);
		serial_recv(buff, MAX_XFER_SIZE, &t);
		if(buff[0]==NAK)
			continue;
		if(buff[0]==ACK)
			return 0;
		printf("Failed to send YMODEM packet\n");
		exit(1);
	}
}

int stm32f_write_bl(char *filename)
{
	uint8_t cmd[] = {0xAA, 0x01, CMD_DOWNLOAD_IMAGE, 0x55};
	int fd;
	struct stat s;
	uint8_t xx;
	uint8_t buff[1030];
	uint8_t pkt_cnt=0;
	char *base_filename;
	char *file_size;
	int t,r;

	fd = open(filename, O_RDONLY);
	if (fd < 0) {
		printf("Cannot open file %s\n", filename);
		exit(1);
	}
	fstat(fd, &s);
	
	serial_set_baudrate(10);

	r = 100;
	while(stm32f_cmd_1_1(CMD_GET_CODE_TYPE, &xx) != 0) {
		if (!(r--)) {
			printf("Failed to get into bootloader. Restart might help.\n");
			exit(1);
		}
	};
	
	if (xx == 1) {
		printf("Requesting STM32F bootloader...\n");
		stm32f_cmd_1_1(CMD_RUN_BOOTLOADER, &xx);
		serial_close();
		printf("Waiting for device to reset...\n");
		usleep(100000);
		serial_open();
		serial_set_baudrate(10);
	}

	r = 100;
	while(stm32f_cmd_1_1(CMD_GET_CODE_TYPE, &xx) != 0) {
		usleep(100000);
		if (!(r--)) {
			printf("Failed to get into bootloader. Restart might help.\n");
			exit(1);
		}
	};

	if (xx) {
		printf("Failed to get into bootloader. Restart might help.\n");
		exit(1);
	}

	printf("Requesting YMODEM transfer ...\n");
	serial_send(cmd, sizeof(cmd), &t);
	printf("Waiting for handshake ...\n");
	r = 100;
	while (1) {
		serial_recv(buff, MAX_XFER_SIZE, &t);
		if(buff[0]=='C')
			break;
		if (r--== 0) {
			printf("Failed to get YMODEM response\n");
			exit(1);
		}
	}

	/* packet 0 */
	memset(buff, 0, sizeof(buff));
	buff[0] = SOH;
	base_filename = basename(filename);
	file_size = strcpy((char *)buff+3, base_filename) + strlen(base_filename) + 1;
	sprintf(file_size, "%d ", (int) s.st_size);
	printf("Flashing %u bytes from file %s ... \n", (int) s.st_size, base_filename);
	ymodem_send_packet(buff);

	t=0;
	/* data packets */
	while (1) {
		memset(buff, 0, sizeof(buff));
		if (read(fd, buff+3, 1024) <= 0)
			break;
		buff[0] = STX;
		buff[1] = ++pkt_cnt;
		ymodem_send_packet(buff);
		printf("\rWriting %u (%u %%)...", t,
			t * 100 / (int) s.st_size + 1);
		t+=1024;
		fflush(stdout);
	}

	/* send EOT */
	buff[0] = EOT;
	serial_send(buff, 1, &t);
	serial_recv(buff, MAX_XFER_SIZE, &t);
	if(buff[0]!=ACK) {
		printf("EOT not accepted\n");
		exit(1);
	}

	/* last packet */
	memset(buff, 0, sizeof(buff));
	buff[0] = SOH;
	ymodem_send_packet(buff);
	printf("\rWriting complete.          \n");


	close(fd);
	return 0;
}

