#include "ModRM.h"

UINT Prefixes(VARPTR&code) {
 static const BYTE plist[]={0x26,0x2E,0x36,0x3E,0x64,0x65,0x66,0x67,0xF0,0xF2,0xF3};
 UINT ret=0;
 for(const BYTE *p;p=(const BYTE*)memchr(plist,*code.c,sizeof plist);) {
  ret|=8<<(p-plist);
  code.c++;
 }
 return ret;
}

#define FIELDOFFSET(type,field)	((int)&(((type*)0)->field))

#define R(x) FIELDOFFSET(CONTEXT,x)
// This are the arrays for CONTEXT field offsets, in the order of the ModR/M register triplett.
 static const BYTE regofs[]={R(Eax),R(Ecx),R(Edx),R(Ebx),R(Esp),R(Ebp),R(Esi),R(Edi)};
// static const BYTE r8ofs[]={R(Eax),R(Ecx),R(Edx),R(Ebx),R(Eax)+1,R(Ecx)+1,R(Edx)+1,R(Ebx)+1};
#undef R

// This is ModR/M and SIB decoding for 32-bit.
// ASSUMING mov reg1632,mem opcode (no reg8 here)
static long M32(VARPTR&code, const CONTEXT*ctx, long**dest) {
 BYTE modrm=*code.b++;
#define R(x) *(long*)((char*)ctx+(regofs[(x)&7]))
 if (dest) *dest=&R(modrm>>3);	// the register itself
 long srcaddr=R(modrm);		// the register itself (assume Mod!=3 which cannot throw exception)
 if ((modrm&0xC7)==0x05) {
  srcaddr=*code.l++;			// take disp32
 }else if ((modrm&0xC0) != 0xC0) {	// indirect mode
  if ((modrm&7)==4) {			// SIB follows, re-calculate srcaddr
   BYTE sib=*code.c++;
   srcaddr=R(sib);
   if (!(modrm&0xC0) && (sib&7)==5) srcaddr=*code.l++;	// don't take EBP here but disp32
   if ((sib&38)!=0x20) srcaddr+=R(sib>>3)*(1<<(sib>>6));
  }
  switch (modrm>>6) {
   case 1: srcaddr+=*code.c++; break;	// add signed disp8
   case 2: srcaddr+=*code.l++; break;	// add disp32
  }
 }
#undef R
 return srcaddr;
}

// This is for AddrSize (0x67) prefix, 16-bit (80286) mode.
// ASSUMING mov reg,mem3216 opcode (not mem8 here)
static long M16(VARPTR&code, const CONTEXT*ctx, short**dest) {
 BYTE modrm=*code.b++;
#define R(x) *(short*)((char*)ctx+(regofs[(x)&7]))
 if (dest) *dest=&R(modrm>>3);	// the register itself
 long srcaddr=LOWORD(modrm&2&&~modrm&7?ctx->Ebp:ctx->Ebx);	// base is either BX or BP
 if ((modrm&6)==4) srcaddr=0;				// no base
 if ((modrm&0xC7)==6) srcaddr=*code.s++;		// take disp16
 if ((modrm&7)<6) srcaddr+=LOWORD(modrm&1?ctx->Edi:ctx->Esi);// add index, either SI or DI
 switch (modrm>>6) {
  case 1: srcaddr+=*code.c++; break;	// add disp8
  case 2: srcaddr+=*code.s++; break;	// add disp16
 }
#undef R
 return srcaddr;
}

long ModRM(UINT prefixes, VARPTR&code, const CONTEXT*ctx, VARPTR*dest) {
 if (prefixes&0x400) return M16(code,ctx,&dest->s);
 else return M32(code,ctx,&dest->l);
}
