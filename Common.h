#ifndef _COMMON_H
#define _COMMON_H

typedef unsigned __int8 uint8;
typedef __int8 int8;
typedef unsigned __int16 uint16;
typedef __int16 int16;
typedef unsigned __int32 uint32;
typedef __int32 int32;

const uint32 MEMORY_SIZE = 0x10000;
const uint32 MAX_ADDRESS = 0x0FFFF;
const uint32 REGS_SIZE	 = 16;

// Shared by CPU and CPU for fast sprite blitting
struct spr_info {
	int32	x;
	int32	y;
	uint32	len;
	const uint8* data;
};

#endif