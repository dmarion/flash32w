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

#include "flash32w.h"
	
int stm32w_reset()
{
	int r = 0;
	uint8_t x;
	r |= stm32f_cmd_2_1(CMD_SET_nBOOTMODE ,1, &x);
	r |= stm32f_cmd_2_1(CMD_SET_nRESET, 0, &x);
	r |= stm32f_cmd_2_1(CMD_SET_nBOOTMODE, 0, &x);
	usleep(100000);
	r |= stm32f_cmd_2_1(CMD_SET_nRESET, 1,  &x);
	usleep(100000);
	r |= stm32f_cmd_2_1(CMD_SET_nBOOTMODE , 1, &x);

	return r;
}

int stm32w_bl_ping()
{
	uint8_t x = 0x7F;
	uint8_t buff[MAX_XFER_SIZE];
	int t;

	serial_send(&x, 1, &t);
	serial_recv(buff, MAX_XFER_SIZE, &t);

	if (buff[0]!=0x79)
		return -1;

	return 0;
}

int stm32w_bl_get(uint8_t *blver)
{
	uint8_t cmd[] = {00, 0xFF};
	uint8_t buff[MAX_XFER_SIZE];
	int t;

	serial_send(cmd, sizeof(cmd), &t);
	serial_recv(buff, MAX_XFER_SIZE, &t);

	if ((t != buff[1]+4) || (buff[0] != 0x79) || buff[t-1]!=0x79)
		return -1;
	
	*blver = buff[2];
	return 0;
}

int stm32w_bl_getid(uint16_t *id)
{
	uint8_t cmd[] = {0x02, 0xFD};
	uint8_t buff[MAX_XFER_SIZE];
	int t;

	serial_send(cmd, sizeof(cmd), &t);
	serial_recv(buff, MAX_XFER_SIZE, &t);

	if ((t != buff[1]+4) || (buff[0] != 0x79) || buff[t-1]!=0x79)
		return -1;
	if (id)
		*id = buff[2]<<8 | buff[3];
	return 0;
}

int stm32w_bl_write_mem(uint32_t addr, uint8_t *data, int len)
{
	uint8_t cmd[] = {0x31, 0xCE};
	uint8_t buff[MAX_XFER_SIZE];
	uint8_t checksum;
	int i,t;
	
	if((len>256) || (len<1))
		return -1;

	/* Send read command */
	serial_send(cmd, sizeof(cmd), &t);
	serial_recv(buff, MAX_XFER_SIZE, &t);

	/* Device should reply with 0x79 */
	if ((t != 1) || (buff[0] != 0x79))
		return -1;

	/* Send start address + XOR checksum */
	buff[0] = addr >> 24;
	buff[1] = (addr >> 16) & 0xFF; 
	buff[2] = (addr >>  8) & 0xFF; 
	buff[3] = addr & 0xFF; 
	buff[4] = buff[0] ^ buff[1] ^ buff[2] ^ buff[3];
	serial_send(buff, 5, &t);
	serial_recv(buff, MAX_XFER_SIZE, &t);

	/* Device should reply with 0x79 */
	if ((t != 1) || (buff[0] != 0x79))
		return -1;

	/* Send length  + XOR checksum */
	buff[0] = (uint8_t) len - 1;
	checksum = (uint8_t) len - 1;
	for(i=1;i<len+1;i++) {
		buff[i]=data[i-1];
		checksum = checksum ^ buff[i];
	}
	buff[len+1] = checksum;

#ifdef DEBUG	
	for(i=0;i<len+2;i++)
		printf("%02x ", buff[i]);
	printf("\n");
#endif

	serial_send(buff, len+2, &t);
	serial_recv(buff, MAX_XFER_SIZE, &t);

	/* Device should reply with 0x79 */
	if ((t != 1) || (buff[0] != 0x79))
			return -1;
	return 0;
}

int stm32w_bl_read_mem(uint32_t addr, uint8_t *data, uint8_t len)
{
	uint8_t cmd[] = {0x11, 0xEE};
	uint8_t buff[MAX_XFER_SIZE];
	int t, to_read;
	
	if(len > 96)
		return -1;

	/* Send read command */
	serial_send(cmd, sizeof(cmd), &t);
	serial_recv(buff, MAX_XFER_SIZE, &t);

	/* Device should reply with 0x79 */
	if ((t != 1) || (buff[0] != 0x79))
		return -1;

	/* Send start address + XOR checksum */
	buff[0] = addr >> 24;
	buff[1] = (addr >> 16) & 0xFF; 
	buff[2] = (addr >>  8) & 0xFF; 
	buff[3] = addr & 0xFF; 
	buff[4] = buff[0] ^ buff[1] ^ buff[2] ^ buff[3];
	serial_send(buff, 5, &t);
	serial_recv(buff, MAX_XFER_SIZE, &t);

	/* Device should reply with 0x79 */
	if ((t != 1) || (buff[0] != 0x79))
		return -1;

	/* Send length  + XOR checksum */
	buff[0] = len - 1;
	buff[1] = (len - 1) ^ 0xFF;
	serial_send(buff, 2, &t);

	/* Read data */
	to_read = len + 1;
	while (to_read && t) {
		serial_recv(buff + len + 1 - to_read, to_read, &t);
		to_read -= t;
	}

	/* First byte must be ACK 0x79, all bytes must be read */
	if((buff[0]!=0x79) || (to_read!=0))
		return -1;

	/* If device is not responding to GETID then something went wrong */
	if (stm32w_bl_getid(NULL) != 0)
		return -1;

	memcpy(data,buff+1,len-to_read);
	return 0;
}

int stm32w_bl_erase(uint8_t start, uint8_t num)
{
	uint8_t cmd[] = {0x43, 0xBC};
	uint8_t buff[MAX_XFER_SIZE];
	uint8_t checksum;
	int i,t;
	
	if((start > 116))
		return -1;

	if((num > 116) || (num == 0))
		return -1;

	if (start + num > 116)
		return -1;

	/* Send read command */
	serial_send(cmd, sizeof(cmd), &t);
	serial_recv(buff, MAX_XFER_SIZE, &t);

	/* Device should reply with 0x79 */
	if ((t != 1) || (buff[0] != 0x79))
			return -1;

	buff[0]=num-1;
	checksum = num-1;
	for (i=1;i<num+1;i++) {
		buff[i]=start + i - 1;
		checksum = checksum ^ buff[i];
	}
	buff[num+1] = checksum;

	serial_send(buff, num+2, &t);
	serial_recv(buff, MAX_XFER_SIZE, &t);

	/* Device should reply with 0x79 */
	if ((t != 1) || (buff[0] != 0x79))
			return -1;
	return 0;
}


