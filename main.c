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
	
int stm32w_info()
{
	int i;
	uint8_t buff[MAX_XFER_SIZE];
	uint32_t x;
	uint8_t y;
	uint16_t z;
	
	serial_set_baudrate(50);

	printf("\nSTM32F USB-to-UART interface:\n");
	if(stm32f_cmd_1_4(CMD_GET_BL_VERSION, &x)) {
		printf("Communication error\n");
		exit(1);
	}
	printf(" %-32s %u.%u.%u.%u\n", "Bootloader Version:",
		(x>>24) & 0xff, (x>>16) & 0xff, (x>>8) & 0xff, x & 0xff);

	if(stm32f_cmd_1_4(CMD_GET_APP_VERSION, &x)) {
		printf("Communication error\n");
		exit(1);
	}
	printf(" %-32s %u.%u.%u.%u\n","Firmware Version:",
		(x>>24) & 0xff, (x>>16) & 0xff, (x>>8) & 0xff, x & 0xff);

	stm32w_reset();

	serial_set_baudrate(115200);
	
	printf("\nSTM32W108 Device information:\n");
	stm32w_bl_ping();
	stm32w_bl_get(&y);
	printf(" %-32s %u\n","BootLoader Version:",y);

	stm32w_bl_getid(&z);
	printf(" %-32s 0x%04x\n","Device Type:",z);

#define PRINT_EUI64_ADDR(a, s) \
	stm32w_bl_read_mem(a, buff, 8);		\
	printf(" %-32s ", s);				\
	for(i=7;i>0;i--)				\
		printf("%02x:", buff[i]);		\
	printf("%02x\n", buff[0]);

#define PRINT_STRING(a, l, s) \
	stm32w_bl_read_mem(a, buff, l);		\
	printf(" %-32s ", s);				\
	for(i=0;i<l;i++)				\
		printf("%c", ((buff[i]<0x20) || (buff[i]>0x7f)) ? '.' : buff[i]); \
	printf("\n");

	PRINT_EUI64_ADDR(0x080407A2, "Burned-in EUI-64 address:");
	PRINT_EUI64_ADDR(0x080408A2, "CIB EUI-64 address:");
	PRINT_STRING(0x0804081A, 16, "CIB Manufacturer String:");
	PRINT_STRING(0x0804082A, 16, "CIB Manufacturer Board Name:");

	/* Read Option Bytes from CIB */
	stm32w_bl_read_mem(0x08040800, buff, 16);
	printf(" %-32s 0x%02x (%s)\n","CIB Read Protection:", buff[0],
		(buff[0] == 0xa5) ? "inactive" : "active" );
	x = buff[8] | buff[10]<<8 | buff[12]<<16 | buff[14]<<24;
	printf(" %-32s ","CIB Write Protection (pg 0-63):"); 
	for(i=0;i<32;i++) {
		if (x & 1<<i)
	        	printf("n");
		else
			printf("y");
		if (!((i+1)%8))
			printf(" ");
	}
	printf("\n");

	/* Read PHY Config from CIB */
	stm32w_bl_read_mem(0x0804083C, buff, 2);
	printf(" %-32s %02x %02x\n","CIB PHY Config:", buff[0], buff[1]);

	printf("\n");
	return 0;
}

int flash_app(uint32_t addr, char *filename)
{
	struct stat s;
	int fd, i;
	uint8_t buff[MAX_XFER_SIZE];

	fd = open(filename, O_RDONLY);
	if (fd < 0) {
		printf("Cannot open file %s\n", filename);
		exit(1);
	}
	fstat(fd, &s);

	serial_set_baudrate(50);
	stm32w_reset();
	serial_set_baudrate(115200);
	stm32w_bl_ping();

	printf("Erasing flash pages %i to %i ...", 0, (int) s.st_size/1024+1);
	if(stm32w_bl_erase(0,(int) s.st_size/1024+1)) {
		printf("Failed to erase flash.");
		exit(1);
	}
	printf(", done.\n");
	printf("Writing %i bytes from %s to flash:\n",(int) s.st_size, filename);
	for(i=0;i<s.st_size; i+=256) {
		memset(buff, 0xFF, 256);
		if (read(fd, buff, 256) <= 0)
			break;
		if(stm32w_bl_write_mem(addr+i, buff, 256)) {
			printf("Failed to write block to address 0x%08x\n", addr + i);
			exit(1);
		}
		printf("\rWriting 0x%08x (%u %%)...", addr + i,
			i * 100 / (int) s.st_size + 1);
		fflush(stdout);
	}	
	printf(", done.\n");
	return 0;
}

int dump_mem(uint32_t addr, uint32_t length)
{
	uint32_t left = length;
	uint8_t data[96];
	uint8_t chars[17];
	int i;

	serial_set_baudrate(50);
	stm32w_reset();
	serial_set_baudrate(115200);
	stm32w_bl_ping();

	chars[16]=0;
	
	printf("%08x: ", addr);
	while (left>0) {
		if (stm32w_bl_read_mem(addr+length-left, data, (left>96) ? 96 : left)<0) {
			printf("\nMemory read error in %u byte block starting at 0x%08x\n", 
				(left>96) ? 96 : left, addr + length - left);
			exit(1);
		}
		for(i=0;i<((left>96) ? 96 : left);i++) {
			printf("%02x ", data[i]);
			chars[i%16]  = ((data[i]<0x20) || (data[i]>0x7e)) ? '.':data[i];
			if(!((i+1)%8))
				printf(" ");
			if(!((i+1)%16)) {
				printf("%s", chars);
				if(left-i-1)
					printf("\n%08x: ", addr + length - left + i + 1);
				
			}
		}
		left -= ((left>96) ? 96 : left);
	}
	chars[i%16] = 0;
	while (((i++))%16) {
		printf("   ");
		if((i%16)==8)
			printf(" ");
	}
	printf(" %s\n", chars);
	return 0;
}

void help()
{
	printf(" -d [-a addr] [-l len]  Dump memory\n");
	printf("   -a <addr>            Start address\n");
	printf("   -l <len>             Length\n");
	printf(" -b <file>              Write boot loader to STM32F interface\n");
	printf(" -f <file> [-a addr]    Write application to STM32W flash\n");
	printf(" -i                     Display device device information\n");
	printf(" -h                     This help\n");
}

int main(int argc, char **argv)
{
	char op;
	char action = 0;
	char *filename = NULL;
	uint32_t addr = 0x08000000;
	uint32_t len = 32;
	extern char *optarg;

	printf("flash32w STM32W Flasher v1.0 (c) 2012 Damjan Marion \n\n");

	while ((op = getopt(argc, argv, "a:b:df:hil:")) != EOF) {
		switch (op) {
		case 'b': case 'f': case 'd': case 'h': case 'i':
			if(action == 0) {
				action = op;
				filename = optarg;
			} else {
				printf("Please specify only one command\n");
				exit(1);
			}
			break;
		case 'a':
			if(*optarg == '0' && *(optarg+1) == 'x') {
				if(sscanf(optarg, "%x", &addr)!=1) {
					printf("Wrong address value.\n");
					exit(1);
				}	
			} else {
				if(sscanf(optarg, "%u", &addr)!=1) {
					printf("Wrong address value.\n");
					exit(1);
				} 
			}
			break;
		case 'l':
			if(*optarg == '0' && *(optarg+1) == 'x') {
				if(sscanf(optarg, "%x", &len)!=1) {
					printf("Wrong length value.\n");
					exit(1);
				}	
			} else {
				if(sscanf(optarg, "%u", &len)!=1) {
					printf("Wrong length value.\n");
					exit(1);
				} 
			}
			break;
		default:
			break;
		}
	}

	if ((action == 0) || (action == 'h')) {
		help();
		exit(1);
	}

	serial_open();

	switch (action) {
		case 'f':
			flash_app(addr, filename);
			break;
		case 'b':
			stm32f_write_bl(filename);
			break;
		case 'd':
			dump_mem(addr, len);
			break;
		case 'i':
			stm32w_info();
			break;
	}
	serial_close();
	return 0;
}

