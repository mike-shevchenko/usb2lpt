This are the slightly modified source files for the bootloadHID flash programming utility.
Because - for unknown reason - the flashing process of the low-speed-USB2LPT's bootloadHID
firmware aborts often, this version retries automatically proceeding the flash address.
Moreover, it's adopted for MSVC6 and slightly optimized in executable's size.

Henrik Haftmann, 090702

Changed again, adding a small delay after each HidD_SetFeature.
For recompiling the source, google for "msvcrt-light.lib" and download this ~ 70 kB file.

heha, 110829
