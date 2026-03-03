# Microsoft Developer Studio Project File - Name="bootloadHID" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=bootloadHID - Win32 Debug
!MESSAGE "bootloadHID - Win32 Release" (basierend auf  "Win32 (x86) Console Application")
!MESSAGE "bootloadHID - Win32 Debug" (basierend auf  "Win32 (x86) Console Application")

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "bootloadHID - Win32 Release"

# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD CPP /nologo /Gz /W3 /O1 /I "c:\Programme\MSVC\DDK\XP\inc\w2k" /D "WIN32" /FD /TP /GF /c
# ADD RSC /l 0x407
BSC32=bscmake.exe
# ADD BSC32 /nologo
LINK32=link.exe
# ADD LINK32 kernel32.lib shlwapi.lib setupapi.lib hid.lib msvcrt-light.lib /nologo /subsystem:console /machine:I386 /nodefaultlib /libpath:"c:\Programme\MSVC\DDK\XP\lib\w2k\i386" /opt:nowin98 /release /merge:.rdata=.text
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "bootloadHID - Win32 Debug"

# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD CPP /nologo /Gz /W3 /Gm /ZI /Od /I "c:\Programme\MSVC\DDK\XP\inc\w2k" /D "WIN32" /D "_DEBUG" /FR /FD /I /GF /c
# ADD RSC /l 0x407 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BSC32 /nologo
LINK32=link.exe
# ADD LINK32 kernel32.lib shlwapi.lib setupapi.lib hid.lib msvcrt-light.lib /nologo /subsystem:console /debug /machine:I386 /nodefaultlib /pdbtype:sept /libpath:"c:\Programme\MSVC\DDK\XP\lib\w2k\i386"

!ENDIF 

# Begin Target

# Name "bootloadHID - Win32 Release"
# Name "bootloadHID - Win32 Debug"
# Begin Group "Quellcodedateien"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\main.cpp
# End Source File
# End Group
# Begin Group "Header-Dateien"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# End Group
# Begin Group "Ressourcendateien"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
