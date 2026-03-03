#include <windows.h>
#include <stdio.h>
#include "ModRM.h"

static _declspec(naked) void opcodes() { _asm{
	mov	dx,es:[esi]
	_emit	0x67
	mov	ax,es:[edi]	// == ax,es:[bx]
	_emit	0x67
	mov	cx,gs:[ebp+8]	// mov ax,gs:[di+8]
	movsx	esi,word ptr es:[edi]
	movsx	esi,word ptr es:[8]
	movzx	eax,word ptr es:[2*ecx+8]
	mov	es,cx
}}


const char *regname(const CONTEXT*ctx, VARPTR destreg) {
 if (destreg.d==&ctx->Eax) return "EAX";
 if (destreg.d==&ctx->Ebx) return "EBX";
 if (destreg.d==&ctx->Ecx) return "ECX";
 if (destreg.d==&ctx->Edx) return "EDX";
 if (destreg.d==&ctx->Ebp) return "EBP";
 if (destreg.d==&ctx->Esp) return "ESP";
 if (destreg.d==&ctx->Esi) return "ESI";
 if (destreg.d==&ctx->Edi) return "EDI";
 return "unk";
}

EXTERN_C void CALLBACK mainCRTStartup() {
/*
 _asm{	mov	edx,0x378
	in	al,dx
 }
*/
 VARPTR code={opcodes};
 CONTEXT ctx;
 ctx.Esi=8;
 ctx.Ebx=0x0A;
 ctx.SegEs=0x40;
 ctx.Ecx=2;
 ctx.Edi=0x10;
 for(;;) {
  UINT prefixes=Prefixes(code);
  VARPTR destreg;
  long srcaddr;
  prefixes|=*code.b&7;
  switch (*code.b++) {
   case 0x8B: {	// if (opcode==0x168B2666) {	// mov dx,es:[esi] with es==0x40, seen in WGif12NT.exe
    srcaddr=ModRM(prefixes,code,&ctx,&destreg);
// Now we have destreg (take it as WORD* when opsize prefix 0x66 is used) and srcaddr
// (which is a poison address and should be 8, 0xA, or 0xC for LPT1, LPT2, or LPT3)
    printf("MOV %s,[%d]\n",regname(&ctx,destreg)+(prefixes&0x200?1:0),srcaddr);
   }break;
   case 0x0F: prefixes&=*code.b|~7; switch (*code.b++) {
    case 0xBF: {
     srcaddr=ModRM(prefixes,code,&ctx,&destreg);
     printf("MOVSX %s,[%d]\n",regname(&ctx,destreg)+(prefixes&0x200?1:0),srcaddr);
    }break;
    case 0xB7: {
     srcaddr=ModRM(prefixes,code,&ctx,&destreg);
     printf("MOVZX %s,[%d]\n",regname(&ctx,destreg)+(prefixes&0x200?1:0),srcaddr);
    }break;
   }break;
   default: ExitProcess(0);
  }
 }
}
