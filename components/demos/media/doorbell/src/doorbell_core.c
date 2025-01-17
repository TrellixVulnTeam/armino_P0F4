#include <common/bk_include.h>

#include <os/mem.h>
#include <os/str.h>
#include <os/os.h>
#include <driver/int.h>
#include <common/bk_err.h>

#include "doorbell.h"

#define CMD_CONTAIN(value) doorbell_cmd_contain(argc, argv, value)

static bool doorbell_cmd_contain(int argc, char **argv, char *string)
{
	int i;
	bool ret = false;

	for (i = 0; i < argc; i++)
	{
		if (os_strcmp(argv[i], string) == 0)
		{
			ret = true;
		}
	}

	return ret;
}

void cli_doorbell_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (CMD_CONTAIN("tcp"))
	{
		demo_doorbell_tcp_init();
	}
	else
	{
		demo_doorbell_udp_init();
	}
}

