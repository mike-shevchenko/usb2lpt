#pragma once

#include <windows.h>

// handy union for accessing pointer to different types in C++
union VARPTR{
 void*v;
 BYTE*b;
 char*c;
 WORD*w;
 short*s;
 DWORD*d;
 long*l;
 ULONGLONG*q;
 __int64*i;
};

// Catches opcode prefixes (0xF6, 0xF7, 0xFE, 0xFF, and 0x0F are NO prefixes here).
// The <code> pointer is then advanced to the true instruction.
	// Bit3=SegES, Bit4=SegCS, Bit5=SegSS, Bit6=SegDS, Bit7=SegFS, Bit8=SegGS,
	// Bit9=OpSize, Bit10=AddrSize, Bit11=LOCK, Bit12=REPNE, Bit13=REP/REPE
UINT Prefixes(VARPTR&code);

// Decode ModR/M and following bytes for 32-bit addressing.
// The <code> pointer is then advanced to the next instruction.
// <prefixes> contains decoded prefixes, and possibly Bit 0 and 1 of opcode.
// Returns the address of the (possibly faulting) source.
// <dest> returns the (possibly faulting) destination.
long ModRM(UINT prefixes, VARPTR&code, const CONTEXT*ctx, VARPTR*dest);
