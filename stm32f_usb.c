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
#include <libusb.h>

#include "flash32w.h"

#define USB_VID		0x0483
#define USB_PID1	0x5740
#define USB_PID2	0x5741

#define EP_IN		0x81
#define EP_OUT		0x03
#define USB_IF		0
#define TIMEOUT		500

//#define DEBUG

libusb_device_handle *devh;

int stm32f_usb_open()
{
	int r;
	r = libusb_init(NULL);
	if (r < 0) {
		fprintf(stderr, "failed to init libusb\n");
		exit(1);
	}

	if (!(devh = libusb_open_device_with_vid_pid(NULL, USB_VID, USB_PID1)))
		if (!(devh = libusb_open_device_with_vid_pid(NULL, USB_VID, USB_PID2))) {
			fprintf(stderr, "libusb_open_device_with_vid_pid error %d\n", r);
			exit(1);
		}

	if (libusb_claim_interface(devh, USB_IF) < 0) {
		fprintf(stderr, "usb_claim_interface error %d\n", r);
		exit(1);
	}
	libusb_set_configuration(devh, 1);
	return 0;
}

int stm32f_usb_close()
{
	libusb_release_interface(devh, USB_IF);
	libusb_close(devh);
	libusb_exit(NULL);
	return 0;
}

int stm32f_usb_send(uint8_t *data, int length, int *transfered)
{
#ifdef DEBUG
	int i;
	printf("%s: bulk write %x bytes\t", __func__, length);
	for(i=0;i<length; i++)
		printf("%02x ", (uint8_t) *(data + i));
	printf("\n");
#endif
	libusb_bulk_transfer(devh, EP_OUT, data, length, transfered, TIMEOUT);

	return 0;
}

int stm32f_usb_recv(uint8_t *data, int length, int *transfered)
{
#ifdef DEBUG
	int i;
#endif

	libusb_bulk_transfer(devh, EP_IN, data, length, transfered, TIMEOUT);

#ifdef DEBUG
	printf("%s: bulk read %i bytes\t", __func__, *transfered);
	for(i=0;i<*transfered; i++)
		printf("%02x ", (uint8_t) *(data + i));
	printf("\n");
#endif
	return 0;
}

int stm32f_usb_set_baudrate(uint32_t b)
{
	struct {
		uint32_t baud;
		uint8_t stop, parity, data;
	} __attribute__((__packed__)) cmd = {b, 0, 0, 8};

	/* CDC ACM SET_LINE_CODING */
	libusb_control_transfer(devh, 0x21, 0x20, 0, 0,(uint8_t *) &cmd, 7, TIMEOUT);
	/* CDC ACM SET_CONTROL_LINE_STATE */
	libusb_control_transfer(devh, 0x21, 0x22, 0x03, 0, NULL, 0, TIMEOUT);
	return 0;
}
