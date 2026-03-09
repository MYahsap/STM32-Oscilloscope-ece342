#include "config.h"
#include "main.h"
#include <string.h>


void print_msg(char * msg) {
  HAL_UART_Transmit(&huart3, (uint8_t *)msg, strlen(msg), 100);
}

void uart_send_bin(uint8_t * buff, unsigned int len) {
  HAL_UART_Transmit(&huart3, (uint8_t *)buff, len, 1000);
}