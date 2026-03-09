#ifndef __CONFIG_H
#define __CONFIG_H

#include "main.h"

extern UART_HandleTypeDef huart3;

void print_msg(char * msg);

void uart_send_bin(uint8_t * buff, unsigned int len);

#endif
