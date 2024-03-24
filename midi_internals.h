/*
Copyright (C) 2024 Eugene Chernyh (mrf-r)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef _MIDI_INTERNALS_H
#define _MIDI_INTERNALS_H

// cin filtering
// #define CINBMP_RESERVED1 (1 << 0)
// #define CINBMP_RESERVED2 (1 << 1)
#define CINBMP_2BYTESYSTEMCOMMON (1 << 2)
#define CINBMP_3BYTESYSTEMCOMMON (1 << 3)
#define CINBMP_SYSEX3BYTES (1 << 4)
#define CINBMP_SYSEXEND1 (1 << 5)
#define CINBMP_SYSEXEND2 (1 << 6)
#define CINBMP_SYSEXEND3 (1 << 7)
#define CINBMP_NOTEOFF (1 << 8)
#define CINBMP_NOTEON (1 << 9)
#define CINBMP_POLYKEYPRESS (1 << 10)
#define CINBMP_CONTROLCHANGE (1 << 11)
#define CINBMP_PROGRAMCHANGE (1 << 12)
#define CINBMP_CHANNELPRESSURE (1 << 13)
#define CINBMP_PITCHBEND (1 << 14)
#define CINBMP_SINGLEBYTE (1 << 15)

// out port status
#define STATUS_SYXLOCK 0x01
#define STATUS_OPTIMIZATION_DISABLED 0x02
#define STATUS_OUTPUT_SYX_MODE 0x04
#define STATUS_NOT_RPN 0x20 // whether last one is RPN or NRPN
#define STATUS_NRPNAH_UNDEF 0x40
#define STATUS_NRPNAL_UNDEF 0x80
#define STATUS_NRPN_UNDEF_BMP 0xC0

// out port cn mutex
#define SYSEX_CN_UNLOCK 0xFF

#endif // _MIDI_INTERNALS_H