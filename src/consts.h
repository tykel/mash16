#ifndef CONSTS_H
#define CONSTS_H

#define MEM_SIZE        0x10000
#define MAX_ADDR        0xFFFF
#define STACK_ADDR      0xFDF0
#define IO_PAD1_ADDR    0xFFF0
#define IO_PAD2_ADDR    0xFFF2

#define FRAME_CYCLES    16667
#define FRAME_DT        16

#define PAD_UP          0x01
#define PAD_DOWN        0x02
#define PAD_LEFT        0x04
#define PAD_RIGHT       0x08
#define PAD_SELECT      0x10
#define PAD_START       0x20
#define PAD_A           0x40
#define PAD_B           0x80

#include <stdint.h>

#endif
