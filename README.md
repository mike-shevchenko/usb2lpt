# usb2lpt

## Origin

This repo contains the files from https://heha.fwh.is/usb/USB2LPT/index.en.htm, because that site is extremely hard to navigate and contains a download protection - I manually downloaded all the files which I found useful from this web site, and put them in this repo for the only sake of conveniently having them at hand.

Quick start (documentation) - open [index.en.htm](index.en.htm). Note that the .zip files referenced from that page and its nested pages are unpacked and committed as directories in this repo.

The author of the project is Henrik Haftmann. The last update of the project was in 2018.

## The project

The project described on the above web site and represented by files of this repo seems to be a **schematics, firmware and drivers** for a **USB2LPT adapter**, which has two versions of the design: 1.6 aka "slow" on ATmega, and **1.7 aka "fast" on Cypress.** Only the "fast" version is recommended now; it works on a standard **CY7C68013A development board** available on aliexpress for a few USD - no need to solder a PCB, just **connect a DB25 female** connector to its pins.

According to the author, the main advantage of such an adaptor over a typically sold USB-to-LPT one is that it not just supports printers via a Windows driver, but can work with other (presumably, bit-banging) applications via supporting a Windows API for manipulating the LPT port.

## Licensing

I didn't manage to find any licensing info on the above site or in the downloaded files, so we have to consider all the files being proprietary to the author, with no rights granted other than looking at them. Any other use of this repo is the responsibility of the one who uses it.
