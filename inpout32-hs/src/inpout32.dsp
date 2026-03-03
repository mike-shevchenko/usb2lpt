# Microsoft Developer Studio Project File - Name="inpout32" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=inpout32 - Win32 Debug
!MESSAGE "inpout32 - Win32 Release" (basierend auf  "Win32 (x86) Dynamic-Link Library")
!MESSAGE "inpout32 - Win32 Debug" (basierend auf  "Win32 (x86) Dynamic-Link Library")

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "inpout32 - Win32 Release"

# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "../x86"
# PROP Intermediate_Dir "../x86/o"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD CPP /nologo /Gr /MD /W3 /O1 /I "c:\Programme\MSVC\ddk\inc\wxp" /I "../jdk/include" /D "INPOUT_EXPORTS" /FD /GF /c
# ADD MTL /nologo /mktyplib203 /win32
# ADD RSC /l 0x409
BSC32=bscmake.exe
# ADD BSC32 /nologo
LINK32=link.exe
# ADD LINK32 kernel32.lib user32.lib advapi32.lib cfgmgr32.lib setupapi.lib shlwapi.lib ../../msvcrt-light.lib uuid.lib /nologo /dll /pdb:none /machine:I386 /nodefaultlib /libpath:"c:\Programme\MSVC\ddk\lib\wxp\i386" /opt:nowin98 /release /merge:.rdata=.text /largeaddressaware

!ELSEIF  "$(CFG)" == "inpout32 - Win32 Debug"

# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "../Debug"
# PROP Intermediate_Dir "../Debug/o"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD CPP /nologo /Gr /MD /W3 /Gm /ZI /I "c:\Programme\MSVC\ddk\inc\wxp" /I "../jdk/include" /D "_DEBUG" /D "INPOUT_EXPORTS" /FR /FD /c
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BSC32 /nologo
LINK32=link.exe
# ADD LINK32 kernel32.lib user32.lib advapi32.lib cfgmgr32.lib setupapi.lib shlwapi.lib ../../msvcrt-light.lib uuid.lib /nologo /dll /pdb:none /debug /machine:I386 /nodefaultlib /libpath:"c:\Programme\MSVC\ddk\lib\wxp\i386"

!ENDIF 

# Begin Target

# Name "inpout32 - Win32 Release"
# Name "inpout32 - Win32 Debug"

# Begin Group "Source Files"
# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File
SOURCE=inpout32.cpp
# End Source File
# Begin Source File
SOURCE=inpout32.def
# End Source File
# Begin Source File
SOURCE=ioPort.cpp
# ADD CPP /Gd
# End Source File
# Begin Source File
SOURCE=LptGetAddr.cpp
# End Source File
# Begin Source File
SOURCE=ModRM.cpp
# End Source File
# Begin Source File
SOURCE=osversion.cpp
# End Source File
# Begin Source File
SOURCE=Queue.cpp
# End Source File
# Begin Source File
SOURCE=Redir.cpp
# End Source File
# End Group

# Begin Group "Header Files"
# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File
SOURCE=hwinterfacedrv.h
# End Source File
# Begin Source File
SOURCE=inpout32.h
# End Source File
# Begin Source File
SOURCE=ModRM.h
# End Source File
# Begin Source File
SOURCE=Redir.h
# End Source File
# Begin Source File
SOURCE=usb2lpt.h
# End Source File
# Begin Source File
SOURCE=usbprint.h
# End Source File
# End Group

# Begin Group "Resource Files"
# PROP Default_Filter "ico;cur;bmp;gif;jpg;png"
# Begin Source File
SOURCE=inpout32.rc
# End Source File
# End Group

# End Target
# End Project
