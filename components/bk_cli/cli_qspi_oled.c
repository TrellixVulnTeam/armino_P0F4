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

#include "cli.h"

extern void cli_qspi_oled_display_pure_color_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
extern void cli_qspi_oled_display_picture_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);


#define QSPI_OLED_CMD_CNT	(sizeof(s_qspi_oled_commands) / sizeof(struct cli_command))

static const struct cli_command s_qspi_oled_commands[] = {
	{"qspi_oled_display_pure_color_test", "qspi_oled_display_pure_color_test {start|stop}", cli_qspi_oled_display_pure_color_test_cmd},
	{"qspi_oled_display_picture_test", "qspi_oled_display_picture_test {start|stop} {file name}", cli_qspi_oled_display_picture_test_cmd},
};

int cli_qspi_oled_init(void)
{
	return cli_register_commands(s_qspi_oled_commands, QSPI_OLED_CMD_CNT);
}

