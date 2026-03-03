# Microsoft Developer Studio Project File - Name="LöscheFirmware" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=LöscheFirmware - Win32 Debug
!MESSAGE "LöscheFirmware - Win32 Release" (basierend auf  "Win32 (x86) Console Application")
!MESSAGE "LöscheFirmware - Win32 Debug" (basierend auf  "Win32 (x86) Console Application")

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "LöscheFirmware - Win32 Release"

# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD CPP /nologo /Gz /W3 /O1 /D "WIN32" /D "_CONSOLE" /FD /c
# SUBTRACT CPP /Fr /YX
# ADD RSC /l 0x407 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BSC32 /nologo
LINK32=link.exe
# ADD LINK32 kernel32.lib user32.lib /nologo /subsystem:console /pdb:none /machine:I386 /nodefaultlib /OPT:NOWIN98 /RELEASE /largeaddressaware /merge:.rdata=.text

!ELSEIF  "$(CFG)" == "LöscheFirmware - Win32 Debug"

# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /FD /c
# SUBTRACT CPP /YX
# ADD RSC /l 0x407 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BSC32 /nologo
LINK32=link.exe
# ADD LINK32 kernel32.lib user32.lib /nologo /subsystem:console /debug /machine:I386 /nodefaultlib /pdbtype:sept

!ENDIF 

# Begin Target

# Name "LöscheFirmware - Win32 Release"
# Name "LöscheFirmware - Win32 Debug"
# Begin Group "Quellcodedateien"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=".\LöscheFirmware.c"
# End Source File
# End Group
# Begin Group "Header-Dateien"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# End Group
# Begin Group "Ressourcendateien"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=".\LöscheFirmware.rc"
# End Source File
# End Group
# End Target
# End Project
