
#ifndef _FLASH32W_H_
#define _FLASH32W_H_

#define MAX_XFER_SIZE	256

#define CMD_SET_nRESET			0
#define CMD_SET_nBOOTMODE		1
#define CMD_GET_CODE_TYPE		2
#define CMD_GET_APP_VERSION		3
#define CMD_IS_APP_PRESENT		4
#define CMD_DOWNLOAD_IMAGE		5
#define CMD_RUN_APPLICATION		6
#define CMD_RUN_BOOTLOADER		7
#define CMD_UPLOAD_IMAGE		8
#define CMD_GET_BL_VERSION		9
#define CMD_DOWNLOAD_BL_IMAGE		10
#define CMD_IS_BL_VERSION_OLD		11
#define CMD_ENABLE_SERIAL_PARSING	12

#define serial_set_baudrate(x)		stm32f_usb_set_baudrate(x)
#define serial_open(x)			stm32f_usb_open(x)
#define serial_close(x)			stm32f_usb_close(x)
#define serial_send(x,y,z)		stm32f_usb_send(x,y,z)
#define serial_recv(x,y,z)		stm32f_usb_recv(x,y,z)

int stm32f_usb_open();
int stm32f_usb_close();
int stm32f_usb_send(uint8_t *data, int length, int *transfered);
int stm32f_usb_recv(uint8_t *data, int length, int *transfered);
int stm32f_usb_set_baudrate(uint32_t b);
int stm32f_write_bl(char *filename);
int stm32f_cmd_1_1(uint8_t c, uint8_t *v);
int stm32f_cmd_1_4(uint8_t c, uint32_t *v);
int stm32f_cmd_2_1(uint8_t c1, uint8_t c2,  uint8_t *v);

int stm32w_reset();
int stm32w_bl_ping();
int stm32w_bl_get(uint8_t *blver);
int stm32w_bl_getid(uint16_t *id);
int stm32w_bl_write_mem(uint32_t addr, uint8_t *data, int len);
int stm32w_bl_read_mem(uint32_t addr, uint8_t *data, uint8_t len);
int stm32w_bl_erase(uint8_t start, uint8_t num);

#endif /* _FLASH32W_H_ */

