#pragma once
#include "driver/dvp_camera_types.h"
//#include <driver/hal/hal_gpio_types.h>


#ifdef __cplusplus
extern "C" {
#endif


#define LCD_SPI_CLK_GPIO  GPIO_2
#define LCD_SPI_CSX_GPIO  GPIO_3
#define LCD_SPI_SDA_GPIO  GPIO_4
#define LCD_SPI_RST       GPIO_6

#define LCD_SPI_DELAY     2

void lcd_spi_write_cmd(uint8_t data);
void lcd_spi_init_gpio(void);
void lcd_spi_write_data(uint8_t data);


void lcd_spi_write_hf_word_cmd(unsigned int cmd);
void lcd_spi_write_hf_word_data(unsigned int data);



#ifdef __cplusplus
}
#endif

