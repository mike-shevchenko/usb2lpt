// This is the necessary snippet out of Windows DDK, e.g.
// file:///.../ddk/inc/ddk/wdm/wxp/usbprint.h
#pragma once

#define USBPRINT_IOCTL_INDEX  0x0000
#define IOCTL_USBPRINT_GET_LPT_STATUS  CTL_CODE(FILE_DEVICE_UNKNOWN,  \
						USBPRINT_IOCTL_INDEX+12,\
						METHOD_BUFFERED,  \
						FILE_ANY_ACCESS)                                                           

#define IOCTL_USBPRINT_SOFT_RESET	CTL_CODE(FILE_DEVICE_UNKNOWN,  \
						USBPRINT_IOCTL_INDEX+16,\
						METHOD_BUFFERED,  \
						FILE_ANY_ACCESS)                                                           
