/*********************************\
 * Queue interface		 *
\*********************************/
#include <windows.h>
EXTERN_C{
#include <hidsdi.h>
//#include <hidpi.h>
}
#include <winioctl.h>
#include "usb2lpt.h"
#include "usbprint.h"
#include "Redir.h"
#include "inpout32.h"

#ifdef _M_IX86
EXTERN_C LONGLONG _declspec(naked) _cdecl _allshr(LONGLONG ll, BYTE shift) {
 _asm{	// ONLY for shift < 16 here!
	shrd	eax,edx,cl
	sar	edx,cl
	ret
 }
}
#undef UInt32x32To64
ULONGLONG _declspec(naked) _fastcall UInt32x32To64(DWORD x,DWORD y) {
 _asm{	xchg	ecx,eax
	mul	edx
	ret
 }
}
#endif

struct AUTOBUF{	// Auto-expanding byte buffer
 BYTE *buf;	// LocalAlloc buffer pointer
 int wi,ri,len;	// indices, length
 AUTOBUF() { buf=0; len=0; reset();}
 void reset() {wi=ri=0;}
 BYTE put(BYTE);
 BYTE get();
 ~AUTOBUF() {LocalFree(buf);}
};

#define DEFSIZE 4096

BYTE AUTOBUF::put(BYTE b) {
 if (wi==len) {
  len+=DEFSIZE;
  if (buf) {
   BYTE *p=(BYTE*)LocalReAlloc(buf,len,LMEM_MOVEABLE);
   if (!p) {
    LocalFree(buf);
    RaiseException(STATUS_NO_MEMORY,0,0,NULL);
   }
   buf=p;
  }else{
   buf=(BYTE*)LocalAlloc(LMEM_FIXED,len);
   if (!buf) RaiseException(STATUS_NO_MEMORY,0,0,NULL);
  }
 }
 if (buf) buf[wi++]=b;
 return b;
}

BYTE AUTOBUF::get() {
 if (ri==wi) {
  RaiseException(STATUS_NO_MEMORY,0,0,NULL);
  return 0xFF;
 }else if (buf) return buf[ri++];
 else return 0xFF;
}

struct QUEUE{
 QUEUE() { v0=v2=false; pass=0;}
 virtual void out(BYTE o, BYTE b);
 virtual BYTE direct_in(BYTE) const;
 virtual BYTE in(BYTE)=0;	// Implementation needs buffer
 virtual void delay(DWORD)=0;
 virtual int flush(BYTE*)=0;
 virtual int inout(const BYTE*,int,BYTE*,int);	// rlen is CHECKED and COUNTED to the expected IN results here
 virtual ~QUEUE() {}
 AUTOBUF inbuf;	// counted IN instructions (assume as read-only from public)
 static DWORD perf_MHz;
protected:
 bool bitop(BYTE b, BYTE (*op)(BYTE,BYTE));	// relies on out() and direct_in()
 bool bitspin(BYTE b);				// spins upto 1 ms for a reset or set bit
 int shortcut(const BYTE*ucode, int ulen, BYTE*result, int rlen);
 BYTE o0,o2;	// mirror of (mostly!) output registers, for read-back
 bool v0,v2;	// o0 and o2 were written once and valid?
private:
 static BYTE or(BYTE a, BYTE b) {return a|b;}
 static BYTE andnot(BYTE a, BYTE b) {return a&~b;}
 static BYTE xor(BYTE a, BYTE b) {return a^b;}
public:
 BYTE pass;
};

DWORD QUEUE::perf_MHz;

// Save value of mostly out-only registers, for dumb programs
void QUEUE::out(BYTE o, BYTE b) {
 switch (o) {
  case 0: o0=b; v0=true; break;
  case 2: o2=b; v2=true; break;
 }
}

BYTE QUEUE::direct_in(BYTE o) const{
 switch(o) {
  case 0: if (v0) return o0; break;
  case 2: if (v2) return o2; break;
 }
 return 0xFF;
}

// Default behaviour: Parse through single in and out instructions
// Parameters are already checked and valid
int QUEUE::inout(const BYTE*ucode,int ulen,BYTE*result,int rlen) {
 do{
  BYTE o=*ucode++;			// opcode
  if (o&0x10) in(o&0x0F);
  else{
   BYTE b=*ucode++; ulen--;
   switch (o) {
    case 0x20: delay(b*4); break;
    case 0x21: bitop(b,or); break;
    case 0x22: bitop(b,andnot); break;
    case 0x23: bitop(b,xor); break;
    case 0x24: bitspin(b); break;
    default: out(o,b);
   }
  }
 }while(--ulen);
 return flush(result);
}

bool QUEUE::bitop(BYTE b, BYTE(*op)(BYTE,BYTE)) {
 BYTE o=b>>4;
 BYTE m=1<<(b&7);	// Bitmaske
 out(o,op(direct_in(o),m));
 return true;
}

bool QUEUE::bitspin(BYTE b) {
 BYTE o=b>>4;		// byte offset
 BYTE m=1<<(b&7);	// bit mask
 BYTE x=b&8?0:0xFF;	// xor value
 LARGE_INTEGER sta,mom,end;
 QueryPerformanceCounter(&sta);
 end.QuadPart=UInt32x32To64(1020,perf_MHz);
 do{
  if ((direct_in(o)^x)&m) break;
  Sleep(0);
  QueryPerformanceCounter(&mom);
 }while (mom.QuadPart-sta.QuadPart < end.QuadPart);
 return true;
}

int QUEUE::shortcut(const BYTE*ucode, int ulen, BYTE*result, int rlen) {
 int ret=0;
 if (ulen==2 && rlen==0) QUEUE::out(ucode[0],ucode[1]);
 else if (ulen==1 && rlen==1) switch (ucode[0]) {
  case 0x10: if (v0 && v2 && !(o2&0x20)) {*result=o0; ret++;} break;
  case 0x12: if (v2) {*result=o2; ret++;} break;
 }
 return ret; 
}

/**********************
 * true parallel port *
 **********************/

struct PPQUEUE:QUEUE {
 PPQUEUE(DWORD addr);
private:	// It's a final class
 void out(BYTE,BYTE);
 BYTE direct_in(BYTE) const;
 BYTE in(BYTE);
 void delay(DWORD);
 int flush(BYTE*);
 DWORD a;
 WORD offset2addr(BYTE o) const { return (o&8?HIWORD(a):LOWORD(a))+(o&7); }
// BYTE inbuf[DEFSIZE];
};

PPQUEUE::PPQUEUE(DWORD addr) {
 a=addr;
}

void PPQUEUE::out(BYTE o, BYTE b) {
// Don't use mirrors here
 Out32(offset2addr(o),b);	// Recursion by RedirOut() will never occur here :-)
}

BYTE PPQUEUE::direct_in(BYTE o) const{
// Don't use shortcuts here
 return Inp32(offset2addr(o));	// Recursion by RedirIn() will never occur here :-)
}

BYTE PPQUEUE::in(BYTE o) {
// Don't use shortcuts here
 switch (pass) {
  case 1: return inbuf.put(direct_in(o));
  case 2: return inbuf.get();
  default: return direct_in(o);
 }
}

void PPQUEUE::delay(DWORD us) {
 LARGE_INTEGER sta,mom,end;
 QueryPerformanceCounter(&sta);
 end.QuadPart=UInt32x32To64(us,perf_MHz);
 do{
  Sleep(us/1000);	// mostly, Sleep(0)
  QueryPerformanceCounter(&mom);
 }while (mom.QuadPart-sta.QuadPart < end.QuadPart);
}

int PPQUEUE::flush(BYTE*buf) {
 int i=inbuf.wi;
 memcpy(buf,inbuf.buf,i);
 inbuf.reset();
 return i;
}

/********************************
 * USB Queue (still abstract)	*
 * Handles timeouts		*
 ********************************/

struct USBQUEUE:QUEUE {
 HANDLE hDev;
protected:
 OVERLAPPED o;
 DWORD timeout;
 USBQUEUE(PCTSTR name);
 int ioctl(DWORD code,const void*outbuf,int outlen,void*inbuf,int inlen);
 int write(const void*outbuf,int outlen);
 int read(void*inbuf,int inlen);
 ~USBQUEUE();
};

USBQUEUE::USBQUEUE(PCTSTR name) {
 hDev = CreateFile(name,GENERIC_READ|GENERIC_WRITE,
	FILE_SHARE_READ|FILE_SHARE_WRITE,
	NULL,OPEN_EXISTING,FILE_FLAG_OVERLAPPED|FILE_FLAG_WRITE_THROUGH|FILE_FLAG_NO_BUFFERING,NULL);
 o.hEvent=CreateEvent(NULL,0,0,NULL);
 timeout=500;
}

// handle timeouts using overlapped I/O
int USBQUEUE::ioctl(DWORD code,const void*outbuf,int outlen,void*inbuf,int inlen) {
 DWORD br;
 if (DeviceIoControl(hDev,code,(void*)outbuf,outlen,inbuf,inlen,&br,&o)
 || GetLastError()==ERROR_IO_PENDING
 && !WaitForSingleObject(o.hEvent,timeout)) {
  GetOverlappedResult(hDev,&o,&br,FALSE);
  return br;
 }
 KRNL(CancelIo(hDev));
 return -1;
}

// handle timeouts using overlapped I/O
int USBQUEUE::write(const void*outbuf,int outlen) {
 DWORD bw;
 o.Offset=o.OffsetHigh=0;	// Must be done here!
 if (WriteFile(hDev,outbuf,outlen,&bw,&o)
 || GetLastError()==ERROR_IO_PENDING
 && !WaitForSingleObject(o.hEvent,timeout)) {
  GetOverlappedResult(hDev,&o,&bw,FALSE);
  return bw;
 }
 KRNL(CancelIo(hDev));
 return -1;
}

USBQUEUE::~USBQUEUE() {
 CloseHandle(o.hEvent);
 CloseHandle(hDev);
}

/**************************
 * USB2LPT (native) queue *
 **************************/

struct USB2LPTQUEUE:USBQUEUE {
 USB2LPTQUEUE(PCTSTR name) : USBQUEUE(name) {}
protected:
 void out(BYTE,BYTE);
 BYTE in(BYTE);
 void delay(DWORD);
 int flush(BYTE*);
 int inout(const BYTE*, int, BYTE*, int);
// ~USB2LPTQUEUE() { if (!ins) flush(0); }
 AUTOBUF ucode;
};

void USB2LPTQUEUE::out(BYTE o, BYTE b) {
 switch (pass) {
  case 2: return;
  default: {
   ucode.put(o);
   ucode.put(b);
   QUEUE::out(o,b);
  }
 }
}

BYTE USB2LPTQUEUE::in(BYTE b) {
 switch (pass) {
  case 1: return ucode.put(b|0x10);
  case 2: return inbuf.get();
  default: {
   ucode.put(b|0x10);
   flush(&b);
   return b;
  }
 }
}

void USB2LPTQUEUE::delay(DWORD us) {
 us/=4;					// USB2LPT dictates 4 us slicing
 while (us>=256) {
  out(0x20,255);		// delay code, maximum delay
  us-=256;
 }
 if (us) out(0x20,(BYTE)(us-1));
}

int USB2LPTQUEUE::flush(BYTE*dat) {
 int ret=0;
 if (ucode.wi) ret=inout(ucode.buf,ucode.wi,inbuf.buf,inbuf.wi);
 if (dat) memcpy(dat,inbuf.buf,ret);
 ucode.reset();
 inbuf.reset();
 return ret;
}

int USB2LPTQUEUE::inout(const BYTE*ucode,int ulen,BYTE*result,int rlen) {
 int ret=shortcut(ucode,ulen,result,rlen);
 if (!ret) ret=ioctl(IOCTL_VLPT_OutIn,ucode,ulen,result,rlen);
 return ret;
}

/***********************
 * USB2LPT (HID) queue *
 ***********************/

struct USB2LPTHIDQUEUE:USB2LPTQUEUE {
 USB2LPTHIDQUEUE(PCTSTR name);
private:	// it's a final class
 int inout(const BYTE*,int,BYTE*,int);
 ~USB2LPTHIDQUEUE() { delete[] IoData; }
 PHIDP_PREPARSED_DATA pd;
 HIDP_CAPS hidcaps;
 BYTE *IoData;
};

USB2LPTHIDQUEUE::USB2LPTHIDQUEUE(PCTSTR name) : USB2LPTQUEUE(name) {
 HidD_(GetPreparsedData(hDev,&pd));
 HidP_(GetCaps(pd,&hidcaps));
 IoData=new BYTE[hidcaps.FeatureReportByteLength];
}

int USB2LPTHIDQUEUE::inout(const BYTE*ucode, int ulen, BYTE*result, int rlen) {
 int ret=shortcut(ucode,ulen,result,rlen);
 if (!ret) {
  int maxl=hidcaps.FeatureReportByteLength-1;
  if (maxl>7) maxl--;	// For 64 byte feature requests with 62 byte payload (not for low-speed USB2LPT)
  int i;
  for (i=0; i<ulen;) {
   int l=ulen-i;
   if (l>maxl) l=maxl;
   if (l>7) {
    IoData[0]=(BYTE)maxl;	// 62 for now
    IoData[1]=(BYTE)l;		// actual payload size
    memcpy(IoData+2,ucode+i,l);
   }else{		// prefer shorter transfers
    IoData[0]=(BYTE)l;	// The Report ID is the length byte, i.e. such a report is a PASCAL string
    memcpy(IoData+1,ucode+i,l);
   }
   if (!HidD_(SetFeature(hDev,IoData,hidcaps.FeatureReportByteLength))) return -1;
   i+=l;
  }
  for (i=0; i<rlen;) {
   int l=rlen-i;
   if (l>maxl) l=maxl;
   if (l>7) l=maxl;		// expect long request
   IoData[0]=(BYTE)l;		// expect this feature request
   if (!HidD_(GetFeature(hDev,IoData,hidcaps.FeatureReportByteLength))) break;	// May block!
   l=IoData[0];
   if (!l || l>maxl) continue;	// don't know how to handle
   if (l>7) {
    l=IoData[1];
    if (!l || l>maxl) continue;	// Cannot handle
    if (l>rlen-i) l=rlen-i;	// Too much data from firmware!
    memcpy(result+i,IoData+2,l);
   }else{
    if (l>rlen-i) l=rlen-i;	// Too much data from firmware!
    memcpy(result+i,IoData+1,l);
   }
   i+=l;
  }
  ret=i;
 }
 if (ret>=0) {ulen=0; inbuf.reset();}
 return ret;
}

/******************************
 * USB->ParallelPrinter queue *
 ******************************/

#define USBPRN_MINFLUSHSIZE 64
// A workaround for the buggy "WCH CH340S" (VID_1A86&PID_7584) chip inside "LogiLink" device:
// If write() is followed by ioctl(), the write() data will not output at all!!
// It seems that ioctl() must be in the next USB frame, otherwise, data
// of a non-full packet are lost. It is a silicon (firmware) bug.
// This is important for printing! Because status-checking may eat up some printing data.
// Instead of waiting a millisecond (USB frame) somehow, I simply send a _full_ packet.
// I couldn't measure any performance impact, for sending 64 bytes instead of 1.
// Of-course, this cannot be done for printing, but it's fully okay for the '574 latch.

// The Prolific PL-2305H (VID_06A9&PID_1991) doesn't have this bug.

struct USBPRNQUEUE:USBQUEUE {
 USBPRNQUEUE(PCTSTR name) : USBQUEUE(name) { fill=0; }
private:	// it's a final class
 void out(BYTE,BYTE);
 BYTE in(BYTE);
 void delay(DWORD);
 int flush(BYTE*);
 ~USBPRNQUEUE() { flush_out(); }
 bool flush_out();
 bool put_out(BYTE);
 int fill;
 BYTE outbuf[64]/*,inbuf[64]*/;
};

bool USBPRNQUEUE::flush_out() {
 bool ret=true;
 if (fill) {
#ifdef USBPRN_MINFLUSHSIZE
  if (fill<USBPRN_MINFLUSHSIZE) {
   memset(outbuf+fill,outbuf[fill-1],USBPRN_MINFLUSHSIZE-fill);	// repeat last data byte
   fill=USBPRN_MINFLUSHSIZE;	// form full USB packet
  }
#endif
  ret=write(outbuf,fill)==fill;
 }
 fill=0;
 return ret;
}

bool USBPRNQUEUE::put_out(BYTE b) {
 outbuf[fill++]=b;
 return fill!=elemof(outbuf) || flush_out();
}

void USBPRNQUEUE::out(BYTE o, BYTE b) {
 switch (o) {
  case 0: put_out(b); break;
  case 2: if ((o2^b)&4) {
   flush_out() && ioctl(IOCTL_USBPRINT_SOFT_RESET,NULL,0,NULL,0)==0;
  }break;
 }	// discard all others
 QUEUE::out(o,b);
}

BYTE USBPRNQUEUE::in(BYTE o) {
 switch (o) {
  case 0: return inbuf.put(v0 ? o0 : 0xFF);
  case 1: {
   flush_out();
   BYTE r;
   ioctl(IOCTL_USBPRINT_GET_LPT_STATUS,NULL,0,&r,1);
   return inbuf.put(r|0xC7);	// Emulate "BUSY=L (= not busy), !ACK=H, no pending interrupt"
  }
  case 2: return inbuf.put(v2 ? o2 : 0xCC);
  case 10: return inbuf.put(0x45);	// say "AutoStrobe" to application
  default: return inbuf.put(0xFF);
 }
}

void USBPRNQUEUE::delay(DWORD us) {
 flush_out();		// typically 1 ms delay for completion
 Sleep(us/1000);	// USB is not as precise for microseconds, so Sleep() seems sufficient here
}

int USBPRNQUEUE::flush(BYTE*buf) {
 flush_out();
 int i=inbuf.wi;		// Same code as for PPQUEUE class
 memcpy(buf,inbuf.buf,i);
 inbuf.reset();
 return i;
}

/**********************************************************
 * FT2232 (Asynchronous BitBang mode, limited to 16 I/Os) *
 **********************************************************/

// TODO

/********************************************************
 * PowerSwitch (libusb, limited to 8 bits, output only) *
 ********************************************************/

// TODO

/************************
 ** Exported functions **
 ************************/

// Beware! n = zero-based number! TODO: n can be a standard LPT address (0x378 etc.)
// This is the class factory function.
QUEUE* WINAPI LptOpen(int n, DWORD flags) {
 if (!QUEUE::perf_MHz) {	// initalize static member only once
  LARGE_INTEGER f;
  if (QueryPerformanceFrequency(&f)) {
#ifdef _M_IX86
    QUEUE::perf_MHz=(DWORD)(f.QuadPart>>6)/(1000000>>6);
#else
    QUEUE::perf_MHz=(DWORD)(f.QuadPart/1000000);
#endif
  }
 }
 QUEUE*q=0;
 switch (RedirInfo[n].where) {
  case 1: q=new PPQUEUE(RedirInfo[n].addr); break;	// cannot fail
  case 2: q=new USB2LPTQUEUE(RedirInfo[n].name); goto checkhandle;
  case 3: q=new USB2LPTHIDQUEUE(RedirInfo[n].name); goto checkhandle;
  case 4: q=new USBPRNQUEUE(RedirInfo[n].name);
  checkhandle: 
  if (((USBQUEUE*)q)->hDev==INVALID_HANDLE_VALUE) {delete q; q=0;} break;
 }
 return q;
}

// check microcode, count IN instructions; ulen is surely >0
static int check_ucode(const BYTE*ucode, int ulen) {
 int ins=0;
 do{
  BYTE b=*ucode++;
  if (b>0x24) return -1;	// error, unknown code
  if (b&0x10) ins++;		// count IN
  else{
   if (!--ulen) return -1;	// error, no operand for OUT and WAIT
   ucode++;			// skip operand (any value is allowed)
  }
 }while(--ulen);
 return ins;
}

// Bypass queue and do microcode-based operations
int WINAPI LptInOut(QUEUE*q, const BYTE*ucode, int ulen, BYTE*result, int rlen) {
 if (!q) return -1;
 if (!ulen) return 0;			// No action returning similar like OK
 if (!ucode) return -2;			// ucode must be available
 if ((unsigned)ulen>DEFSIZE) return -3;	// too much or negative
 if (rlen<0) return -4;			// negative
 int i=check_ucode(ucode,ulen);
 if ((unsigned)i>(unsigned)rlen) return -5;// illegal ucode or result buffer too small
 if (i && !result) return -6;		// ucode contains IN but no result buffer given
 return q->inout(ucode,ulen,result,i);
}

// Enqueue an OUT operation
void WINAPI LptOut(QUEUE*q, BYTE offset, BYTE value) {
 if (!q) return;
 if (offset>=16) return;		// invalid offset (0..7 = SPP, 8..11 = ECP, 12..15 = USB2LPT Special)
 q->out(offset,value);
}

// Enqueue an IN operation
BYTE WINAPI LptIn(QUEUE*q, BYTE offset) {
 if (!q) return false;
 if (offset>=16) return false;
 return q->in(offset);
}

// Enqueue a small delay. Currently, < 10 ms is allowed
void WINAPI LptDelay(QUEUE*q, DWORD us) {
 if (!q) return;
 if (us>10000) return;
 q->delay(us);
}

int WINAPI LptFlush(QUEUE*q, BYTE*inbuf, int bufsize) {
 if (!q) return -1;
 if (bufsize<q->inbuf.wi) return -1;	// buffer too small for collected IN instructions
 if (!inbuf && q->inbuf.wi) return -1;	// buffer MUST be provided
 return q->flush(inbuf);
}

BOOL WINAPI LptClose(QUEUE*q) {
 if (!q) return false;
 delete q;
 return GetLastError()==0;
}

BYTE WINAPI LptPass(QUEUE*q, BYTE b) {
 BYTE ret=q->pass;
 if (b<=2) q->pass=b;
 return ret;
}
