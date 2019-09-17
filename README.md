# Open Source version of our HAL (Hardware Abstraction Layer) for Win32 and Linux
Almost all of our projects are written against an hardware abstraction layer that has been ported to various compilers and target systems. This open source release contains implementations of several core modules for Windows (Win32) and Linux. These core modules include OS abstractions for threading and synchronization, filesystem I/O and networking.
This repository does not contain everything we have but everything you should need to build our other open source releases for either Windows or Linux.

## Other Chipset/OS ports beyond Windows and Linux
Several other ports of this HAL exist and even for Windows there are multiple versions of several modules. For example, there is one Network API implementation for the Windows Sockets APIs and there is one implementation that talks to LWIP on a tun/tap driver. 
Currently available ports:
* x86 - Windows / Win32 (available in this Repository)
* x86 - Linux (available in this Repository)
* ARM - uCos II
* ARM - Fujitsu FAMOS
* SH4 - ST OS21 - STAPI
* SH4 - ST Linux - STAPI
* MIPS - MStar Linux
* ARM - Nucleus PLUS

Please contact us if you think that you need any of these ports. Unless the particular implementation is restricted by an NDA, you will probably get what you ask for.

## Other Modules beyond OS basics
The OS abstractions for threading and synchronization, filesystem I/O and networking exist for all platforms. A lot of other modules exist, including interfaces for Smartcard I/O (via PC/SC or hardware interfaces), Common Interface/PCMCIA access, MPEG2 Demultiplexer control (hardware section filtering), access to (de)scrambler hardware and control of hardware audio/video decoders for MPEG2,MPEG4/H.264 and HEVC/H.265. Most of these API modules have been defined and used for use in an embedded digital television environment.
Please contact us if you think that you need any of these interfaces or implementations for a particular platform. Unless the particular implementation is restricted by an NDA, you will probably get what you ask for.

## Documentation
The HAL documentation can be found inline in the corresponding header files.
It has been written in doxygen style.

## Licensing
![CC BY-NC 4.0 Logo](https://i.creativecommons.org/l/by-nc/4.0/88x31.png)

This Open Source version of the HAL has been put under the CC BY-NC 4.0 license.
You can learn from this code and you can use it in non-commercial projects if your documentation says that you do so. You can not use it for commercial software (without asking us first) and you can not claim that this is your own work.

*If you ask* politely, we are also very likely to give (free) permission for pretty much every other use (unless you are Google, Apple, Microsoft, Facebook, Oracle, Adobe, Bayer/Monsanto, Nestlé, a political party or a government).

## Contributing
The copyrights for this package are held by [GkWare e.K.](https://www.gkware.com).
By contributing to this repository, authors grant us an irrevocable worldwide royalty free license to use, modify, distribute and even potentially re-license the contributions and binaries built out of them. Additional authors will be given appropriate credit (once applicable) in a separate AUTHORS file on a global or per platform/module basis.
