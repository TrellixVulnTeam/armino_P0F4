// Copyright 2020-2021 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "sdio_hal.h"
#include "sdio_ll.h"


void sdio_hal_slave_cmd_start(uint32_t value)
{
	sdio_ll_set_reg0x0_sd_cmd_start(value);
}

/* REG_0x09:reg0x9->CMD_S_RES_END_INT:0x9[24],Slave only; Slave has finish reponsed the CMD to host side.,0x0,R/W1C*/
bool sdio_hal_slave_get_cmd_response_end_int(void)
{
    return (bool)sdio_ll_get_reg0x9_cmd_s_res_end_int();
}

void sdio_hal_slave_clear_cmd_response_end_int(void)
{
	//write 1 to clear INT status, 0 do nothing.
    sdio_ll_set_reg0x9_cmd_s_res_end_int(1);
}

/* REG_0x0a:reg0xa->TX_FIFO_NEED_WRITE_MASK_CG:0xa[13],1:sd host fifo memory need write mask for clk gate writing use only,RW*/
void sdio_hal_host_set_tx_fifo_need_write_mask_cg(uint32_t value)
{
	//write 1 to mask clock gate of "tx fifo need write".
    sdio_ll_set_reg0xa_tx_fifo_need_write_mask_cg(value);
}

uint32_t sdio_hal_get_read_ready(void)
{
	return sdio_ll_get_reg0xd_rxfifo_rd_ready();
}

uint32_t sdio_hal_get_write_ready(void)
{
	return sdio_ll_get_reg0xd_txfifo_wr_ready();
}

//BK7256 only:BK7256 SDIO host&slave uses only one IP controller and controlled by REG.
void sdio_hal_set_host_slave_mode(sdio_host_slave_mode_t mode)
{
	sdio_ll_set_reg0x10_sd_slave(mode);
}

uint32_t sdio_hal_slave_read_data(void)
{
	return sdio_ll_get_reg0xc_value();
}

void sdio_hal_slave_write_data(uint32_t value)
{
	sdio_ll_set_reg0xd_value(value);
}

uint32_t sdio_hal_get_int_status(void)
{
	return sdio_ll_get_reg0x9_value();
}

uint32_t sdio_hal_slave_get_read_int_status(void)
{
	return sdio_ll_get_reg0x9_dat_s_rd_bus_int();
}

void sdio_hal_slave_clear_read_int_status(void)
{
	sdio_ll_set_reg0x9_dat_s_rd_bus_int(1);
}

uint32_t sdio_hal_slave_get_rx_count(void)
{
	return sdio_ll_get_sd_slave_status_cmd_s_rec_bb_cnt();
}

void sdio_hal_slave_notify_host_next_block(void)
{
	sdio_ll_set_reg0x10_dat_s_rd_mul_blk(0);

	//confirm the reg status is stable.delay time should more then one cycle of SDIO
	for(volatile int i = 0; i < 200; i++);

	sdio_ll_set_reg0x10_dat_s_rd_mul_blk(1);
}

uint32 sdio_hal_slave_get_func_reg_value(void)
{
	return sdio_ll_get_reg0x13_value();
}

//SW:private protocal,host read this reg value as the slave send packet length.
void sdio_hal_slave_set_tx_length(uint32_t len)
{
	sdio_ll_set_reg0x11_value(len);
}

void sdio_hal_slave_clear_stop(void)
{
	sdio_ll_set_reg0x10_cmd_52_stop_clr(1);
}

void sdio_hal_slave_tx_transaction_en(void)
{
	sdio_ll_set_reg0x3_sd_start_wr_en(1);
}

//host write, slave read, if slave fifo is full, slave will stop host to write data
void sdio_hal_slave_rx_clear_host_wait_write_data(void)
{
	sdio_ll_set_reg0x9_dat_s_wr_wai_int(1);
}

#if CONFIG_SOC_BK7256XX
/* REG_0x0d:reg0xd->SAMP_SEL:0xd[26],sample egde of data 0：neg 1：pos,RW*/
uint32_t sdio_hal_slave_get_samp_sel(void)
{
    return sdio_ll_get_reg0xd_samp_sel();
}

void sdio_hal_slave_set_samp_sel(uint32_t value)
{
	sdio_ll_set_reg0xd_samp_sel(value);
}
#endif

#if CONFIG_SDIO_SLAVE
uint32_t sdio_hal_slave_get_cmd_arg0(void)
{
	return sdio_ll_get_reg0x5_value();
}

uint32_t sdio_hal_get_tx_fifo_empty_int_status(void)
{
    return sdio_ll_get_reg0x9_tx_fifo_empt();
}

void sdio_hal_clear_tx_fifo_empty_int_status(void)
{
    sdio_ll_clear_reg0x9_tx_fifo_empt();
}


//NOTES:be care this is for slave write end, and sdio_ll_get_reg0x9_sd_data_wr_end_int is for host write end
uint32_t sdio_hal_slave_get_wr_end_int(void)
{
    return sdio_ll_get_reg0x9_dat_s_wr_wai_int();
}

void sdio_hal_slave_clear_wr_end_int(void)
{
	//write 1 to clear INT status, 0 do nothing.
    sdio_ll_set_reg0x9_dat_s_wr_wai_int(1);
}

void sdio_hal_set_tx_fifo_empty_int(uint32_t value)
{
	//write 1 to mask clock gate of "tx fifo need write".
    sdio_ll_set_reg0xa_tx_fifo_empt_mask(value);
}

void sdio_hal_slave_set_cmd_res_end_int(uint32_t value)
{
	//write 1 to mask clock gate of "tx fifo need write".
    sdio_ll_set_reg0xa_cmd_s_res_end_int_mask(value);
}

void sdio_hal_slave_set_write_end_int(uint32_t value)
{
    sdio_ll_set_reg0xa_dat_s_wr_wai_int_mask(value);
}

void sdio_hal_slave_set_read_end_int(uint32_t value)
{
    sdio_ll_set_reg0xa_dat_s_rd_bus_int_mask(value);
}

void sdio_hal_slave_set_blk_size(uint32_t value)
{
	sdio_ll_set_reg0x3_sd_data_blk_size(value);
}

uint32_t sdio_ll_get_sd_slave_wr_finish(void)
{
	return sdio_ll_get_sd_slave_status_sd_start_wr_en_r3();
}

//WARNING:it will reset fifo status.
void sdio_hal_fifo_reset(void)
{
	sdio_ll_set_reg0xd_tx_fifo_rst();
	sdio_ll_set_reg0xd_rx_fifo_rst();
	sdio_ll_set_reg0xd_sd_sta_rst();
}
#endif
