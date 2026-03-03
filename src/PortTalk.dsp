# Microsoft Developer Studio Project File - Name="PortTalk" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** NICHT BEARBEITEN **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=PortTalk - Win32 Debug
!MESSAGE "PortTalk - Win32 Release" (basierend auf  "Win32 (x86) Dynamic-Link Library")
!MESSAGE "PortTalk - Win32 Debug" (basierend auf  "Win32 (x86) Dynamic-Link Library")

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "PortTalk - Win32 Release"

# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD CPP /nologo /MT /W3 /O1 /D "WIN32" /D "PORTTALK_EXPORTS" /D "UNICODE" /FD /GF /c
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD RSC /l 0x407 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BSC32 /nologo
LINK32=link.exe
# ADD LINK32 kernel32.lib user32.lib /nologo /dll /machine:I386 /nodefaultlib /opt:nowin98 /largeaddressaware /release
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "PortTalk - Win32 Debug"

# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "H:\Program Files (x86)\FlexRadio Systems\PowerSDR v2.0.16\"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD CPP /nologo /MTd /W3 /Gm /Zi /Od /D "WIN32" /D "_DEBUG" /D "PORTTALK_EXPORTS" /D "UNICODE" /FD /GF /c
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD RSC /l 0x407 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BSC32 /nologo
LINK32=link.exe
# ADD LINK32 kernel32.lib user32.lib /nologo /dll /incremental:no /debug /machine:I386 /nodefaultlib /pdbtype:sept /opt:nowin98 /largeaddressaware /release
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "PortTalk - Win32 Release"
# Name "PortTalk - Win32 Debug"
# Begin Group "Quellcodedateien"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\PortTalk.cpp
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
