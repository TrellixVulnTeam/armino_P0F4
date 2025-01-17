// Copyright 2022-2023 Beken
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

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <soc/soc.h>

#define REG_JPEG_DEC_BASE_ADDR              SOC_JPEG_DEC_REG_BASE


#define REG_JPEG_ACC_REG0               (*((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x00 * 4)))
#define REG_JPEG_PIPO_REG0              (*((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x01 * 4)))
#define REG_JPEG_PIPO_REG1              (*((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x02 * 4)))
#define REG_JPEG_DCUV                   (*((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x03 * 4)))
#define REG_JPEG_CUR_MASK               (*((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x04 * 4)))
#define REG_JPEG_MCUX                  (*((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x05 * 4)))
#define REG_JPEG_MCUY                   (*((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x06 * 4)))
#define REG_JPEG_CURBYTE                (*((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x07 * 4)))


#define REG_JPEG_CURRUN                 (*((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x09 * 4)))
#define REG_JPEG_XPIXEL                 (*((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x0a * 4)))
#define REG_JPEG_HUF_DATA               (*((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x0b * 4)))

#define REG_JPEG_STATE                   (*((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x0d * 4)))

#define REG_JPED_DEC_RDY                 (REG_JPEG_STATE & 0x1)
#define REG_JPED_RRLOAD                  (REG_JPEG_STATE & 0x2)
#define REG_JPED_DEC_RELOAD              (REG_JPEG_STATE & 0x4)
#define REG_JPED_DEC_SEARCH              (REG_JPEG_STATE & 0x8)
#define REG_JPED_EXT_RELOAD              (REG_JPEG_STATE & 0x10)
#define REG_JPED_EXT_SEARCH              (REG_JPEG_STATE & 0x20)
#define REG_IDCT_BUSY                    (REG_JPEG_STATE & 0x40)
#define TMP_MEM_CLR_BUSY                 (REG_JPEG_STATE & 0x80)
#define REG_JPED_DEC_RDY2                (REG_JPEG_STATE & 0x100)

#define REG_CUR_NBIT                     (*((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x0c * 4)))
#define REG_JPEG_EXT_DATA                (*((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x0e * 4)))
#define REG_JPEG_BLK                     (*((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x0f * 4)))

#define REG_DEC_CMD                     (*((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x08 * 4)))

#define REG_DEC_START                   (REG_DEC_CMD = 0x01);
#define REG_DEC_TMP_CLR                 (REG_DEC_CMD = 0x02);
#define REG_DC_CLR                      (REG_DEC_CMD = 0x04);
#define BASE_LOAD                       (REG_DEC_CMD = 0x08);

#define REG_JPEG_BITS_DC00               (*((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x10 * 4)))
#define REG_JPEG_BITS_DC01               (*((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x11 * 4)))
#define REG_JPEG_BITS_DC02               (*((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x12 * 4)))
#define REG_JPEG_BITS_DC03               (*((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x13 * 4)))
#define REG_JPEG_BITS_DC04               (*((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x14 * 4)))
#define REG_JPEG_BITS_DC05               (*((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x15 * 4)))
#define REG_JPEG_BITS_DC06               (*((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x16 * 4)))
#define REG_JPEG_BITS_DC07               (*((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x17 * 4)))
#define REG_JPEG_BITS_DC08               (*((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x18 * 4)))
#define REG_JPEG_BITS_DC09               (*((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x19 * 4)))
#define REG_JPEG_BITS_DC0A               (*((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x1a * 4)))
#define REG_JPEG_BITS_DC0B               (*((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x1b * 4)))
#define REG_JPEG_BITS_DC0C               (*((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x1c * 4)))
#define REG_JPEG_BITS_DC0D               (*((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x1d * 4)))
#define REG_JPEG_BITS_DC0E               (*((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x1e * 4)))
#define REG_JPEG_BITS_DC0F               (*((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x1f * 4)))

#define REG_JPEG_BITS_DC10               (*((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x20 * 4)))
#define REG_JPEG_BITS_DC11               (*((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x21 * 4)))
#define REG_JPEG_BITS_DC12               (*((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x22 * 4)))
#define REG_JPEG_BITS_DC13               (*((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x23 * 4)))
#define REG_JPEG_BITS_DC14               (*((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x24 * 4)))
#define REG_JPEG_BITS_DC15               (*((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x25 * 4)))
#define REG_JPEG_BITS_DC16               (*((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x26 * 4)))
#define REG_JPEG_BITS_DC17               (*((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x27 * 4)))
#define REG_JPEG_BITS_DC18               (*((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x28 * 4)))
#define REG_JPEG_BITS_DC19               (*((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x29 * 4)))
#define REG_JPEG_BITS_DC1A               (*((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x2a * 4)))
#define REG_JPEG_BITS_DC1B               (*((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x2b * 4)))
#define REG_JPEG_BITS_DC1C               (*((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x2c * 4)))
#define REG_JPEG_BITS_DC1D               (*((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x2d * 4)))
#define REG_JPEG_BITS_DC1E               (*((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x2e * 4)))
#define REG_JPEG_BITS_DC1F               (*((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x2f * 4)))



#define REG_JPEG_BITS_AC00              (*((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x30 * 4)))
#define REG_JPEG_BITS_AC01              (*((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x31 * 4)))
#define REG_JPEG_BITS_AC02              (*((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x32 * 4)))
#define REG_JPEG_BITS_AC03              (*((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x33 * 4)))
#define REG_JPEG_BITS_AC04              (*((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x34 * 4)))
#define REG_JPEG_BITS_AC05              (*((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x35 * 4)))
#define REG_JPEG_BITS_AC06              (*((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x36 * 4)))
#define REG_JPEG_BITS_AC07              (*((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x37 * 4)))
#define REG_JPEG_BITS_AC08              (*((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x38 * 4)))
#define REG_JPEG_BITS_AC09              (*((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x39 * 4)))
#define REG_JPEG_BITS_AC0A              (*((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x3a * 4)))
#define REG_JPEG_BITS_AC0B              (*((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x3b * 4)))
#define REG_JPEG_BITS_AC0C              (*((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x3c * 4)))
#define REG_JPEG_BITS_AC0D              (*((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x3d * 4)))
#define REG_JPEG_BITS_AC0E              (*((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x3e * 4)))
#define REG_JPEG_BITS_AC0F              (*((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x3f * 4)))

#define REG_JPEG_BITS_AC10              (*((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x40 * 4)))
#define REG_JPEG_BITS_AC11              (*((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x41 * 4)))
#define REG_JPEG_BITS_AC12              (*((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x42 * 4)))
#define REG_JPEG_BITS_AC13              (*((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x43 * 4)))
#define REG_JPEG_BITS_AC14              (*((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x44 * 4)))
#define REG_JPEG_BITS_AC15              (*((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x45 * 4)))
#define REG_JPEG_BITS_AC16              (*((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x46 * 4)))
#define REG_JPEG_BITS_AC17              (*((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x47 * 4)))
#define REG_JPEG_BITS_AC18              (*((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x48 * 4)))
#define REG_JPEG_BITS_AC19              (*((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x49 * 4)))
#define REG_JPEG_BITS_AC1A              (*((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x4a * 4)))
#define REG_JPEG_BITS_AC1B              (*((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x4b * 4)))
#define REG_JPEG_BITS_AC1C              (*((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x4c * 4)))
#define REG_JPEG_BITS_AC1D              (*((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x4d * 4)))
#define REG_JPEG_BITS_AC1E              (*((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x4e * 4)))
#define REG_JPEG_BITS_AC1F              (*((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x4f * 4)))

#define TIME_STATE                       (*((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x50 * 4)))
#define DEBUG0                           (*((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x51 * 4)))
#define DEBUG1                           (*((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x52 * 4)))
#define DEBUG2                           (*((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x53 * 4)))
#define DEBUG3                           (*((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x54 * 4)))
#define DEBUG4                           (*((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x55 * 4)))
#define DEBUG5                           (*((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x56 * 4)))
#define BASE_RADDR                       (*((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x58 * 4)))
#define BASE_WADDR                       (*((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x59 * 4)))
#define BASE_RD_LEN                      (*((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x5a * 4)))
#define BASE_WR_LEN                      (*((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x5b * 4)))
#define BASE_FFDA                        (*((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x5c * 4)))

#define JPEGDEC_INTEN                    (*((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x5e * 4)))
#define JPEGDEC_INTSTAT                  (*((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x5f * 4)))

#define dec_busy_clr                     (JPEGDEC_INTSTAT = 0x01);
#define rrload_clr                       (JPEGDEC_INTSTAT = 0x02);
#define reload_clr                       (JPEGDEC_INTSTAT = 0x04);
#define search_clr                       (JPEGDEC_INTSTAT = 0x08);
#define ext_reload_clr                   (JPEGDEC_INTSTAT = 0x10);
#define ext_bit_clr                      (JPEGDEC_INTSTAT = 0x20);
#define dec_busy2_clr                    (JPEGDEC_INTSTAT = 0x40);
#define mcu_finish_clr                   (JPEGDEC_INTSTAT = 0x80);


#define REG_JPEG_HUF_TABLE00            *((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x80 * 4))
#define REG_JPEG_HUF_TABLE10            *((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0xc0 * 4))
#define REG_JPEG_HUF_TABLE01            *((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x100 * 4))
#define REG_JPEG_HUF_TABLE11            *((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x200 * 4))
#define REG_JPEG_Zig                    *((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x300 * 4))
#define REG_JPEG_TMP0                   *((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x340 * 4))
#define REG_JPEG_DQT_TABLE0             *((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x380 * 4))
#define REG_JPEG_DQT_TABLE1             *((volatile unsigned long *)(REG_JPEG_DEC_BASE_ADDR + 0x3c0 * 4))

#ifdef __cplusplus
}
#endif

