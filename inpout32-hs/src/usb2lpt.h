// This is the necessary snippet out of
// http://www.tu-chemnitz.de/~heha/usb2lpt/usb2lpt.zip/src/sys/usb2lpt.h
#pragma once

// same as GUID_DEVINTERFACE_PARALLEL and GUID_PARALLEL_DEVICE (111022)
DEFINE_GUID(Vlpt_GUID,0x97F76EF0L,0xF883,0x11D0,0xAF,0x1F,0x00,0x00,0xF8,0x00,0x84,0x5C);

#define Vlpt_CTL(a,b) CTL_CODE(FILE_DEVICE_UNKNOWN,0x0800+(a),METHOD_##b,FILE_ANY_ACCESS)

#define IOCTL_VLPT_OutIn		Vlpt_CTL(4,BUFFERED)
#define IOCTL_VLPT_GetLastError		Vlpt_CTL(23,BUFFERED)
