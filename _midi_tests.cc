#include "core_time.h"
#include "midi.h"
#include "debug_logger.h"
#include "midi_hw.h"

void midi_test()
{
    // test noteoff conversion
    MidiMessageT m, mr, mt;
    MidiTsMessageT mrx;
    m.cn = MIDI_CN_LOCAL;
    m.cin = MIDI_CIN_NOTEON;
    m.byte1 = 0x90;
    m.byte2 = 0x40;
    m.byte2 = 0x00;
    mr.full_word = m.full_word;
    mr.cin = MIDI_CIN_NOTEOFF;
    mr.byte2 = 0x80;

    midiNonSysexWrite(m);
    dbg_printf("send noteon : %d\n",midi_rx_utilization());
    dbg_printf("rcv event ret :%d\n",midiRead(&mrx));
    dbg_printf("rcv event :%08X\n",mrx.mes.full_word);
    dbg_printf("passed :%d\n",mrx.mes.full_word == mr.full_word);

    // overflow message, nwp calc
    for (int i=0;i<257;i++) {
        //delay_ms(5);
        midiNonSysexWrite(m);
        dbg_printf("send %d - %d\n",i,midi_rx_utilization());
    }
    for (int i=0;i<200;i++) {
        //delay_ms(5);
        dbg_printf("receive %d - %d\n",i,midiRead(&mrx));
    }
    // mBufferWrite - buffer cycling - wtf is that?

    // test rx sysex lock
    m.cn = MIDI_CN_LOCAL;
    m.cin = MIDI_CIN_SYSEX3BYTES;
    m.byte1 = 0xF0;
    m.byte2 = 0x40;
    m.byte2 = 0x40;
    dbg_printf("syx start %d\n",midiWrite(m));
    m.byte1 = 0x40;
    dbg_printf("syx cont %d\n",midiWrite(m));
    m.cn = MIDI_CN_UART1;
    dbg_printf("syx try different cn %d\n",midiWrite(m));
    m.cn = MIDI_CN_LOCAL;
    m.cin = MIDI_CIN_SYSEXEND1;
    m.byte1 = 0xF7;
    dbg_printf("syx end %d\n",midiWrite(m));
    m.cn = MIDI_CN_UART1;
    dbg_printf("syx diff1 %d\n",midiWrite(m));
    m.cn = MIDI_CN_USBDEVICE1;
    dbg_printf("syx diff2 %d\n",midiWrite(m));
    // syx overflow???

    // utilization
    dbg_printf("midi util %d\n",midi_rx_utilization());
    dbg_printf("midi syx util %d\n",midi_rxsyx_utilization());
    dbg_printf("receive rx %d\n",midiRead(&mrx));
    dbg_printf("receive syx %d\n",midiSysexRead(&mt));
    dbg_printf("midi util %d\n",midi_rx_utilization());
    dbg_printf("midi syx util %d\n",midi_rxsyx_utilization());

    //////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////
    // port
    m.cin = MIDI_CIN_CONTROLCHANGE;
    m.byte1 = 0xB0;
    m.byte2 = 0x37;
    m.byte3 = 0x22;
    dbg_printf("port tx %d\n",midiPortWrite(&u1midi_out,m));
    // optimization
    dbg_printf("port tx %d\n",midiPortWrite(&u1midi_out,m));
    dbg_printf("port tx %d\n",midiPortWrite(&u1midi_out,m));
    dbg_printf("port tx %d\n",midiPortWrite(&u1midi_out,m));
    dbg_printf("port tx %d\n",midiPortWrite(&u1midi_out,m));
    dbg_printf("port tx %d\n",midiPortWrite(&u1midi_out,m));
    delay_ms(200);
    dbg_printf("midi util %d\n",midi_rx_utilization());
    while (midi_rx_utilization()) {
        dbg_printf("receive: rx %d\n",midiRead(&mrx));
        dbg_printf("  ts: %08X\n",mrx.timestamp);
        dbg_printf("  mes: %08X\n",mrx.mes.full_word);
    }
    // decimation
    m.cin = MIDI_CIN_CHANNELPRESSURE;
    m.byte1 = 0xD0;
    m.byte3 = 0;
    for (int i=0;i<32;i++) {
        m.byte2 = i;
        dbg_printf("port channel pressure tx %d - %d\n",i,midiPortWrite(&u1midi_out,m));
    }
    m.cin = MIDI_CIN_NOTEOFF;
    m.byte1 = 0x80;
    m.byte3 = 0;
    for (int i=0;i<32;i++) {
        m.byte2 = i;
        dbg_printf("port noteoff tx %d - %d\n",i,midiPortWrite(&u1midi_out,m));
    }
    m.cin = MIDI_CIN_SINGLEBYTE;
    m.byte1 = 0xF8;
    dbg_printf("port sync tx %d\n",midiPortWrite(&u1midi_out,m));
    
    delay_ms(200); // wait for all received
    while (midi_rx_utilization()) {
        dbg_printf("receive: rx %d\n",midiRead(&mrx));
        dbg_printf("  ts: %08X\n",mrx.timestamp);
        dbg_printf("  mes: %08X\n",mrx.mes.full_word);
    }

    // nonoptimize
    // sysex lock/unlock, timeout
    // syx unterminated

    // UART runningstatus timer
    // UART active sense timer

    // debug functionality - flush count, etc..

}

// MESSAGE FLUSHED CALCULATORS - all calcs and total sums

// syx lock for regular messages
// syx interruption by regular, timeout, UNTERMINATED SYX
// sysex content received from uart

//MIDI_NO_PROTOCOL_FILTER
//runningstatus_timer
//out sysex from syx out buf??

// output - rt insertion between message bytes

/*
3 threads
    - main - send and process main buffer
    - cr - daemons in outports
    - ports interrupts


    wrap compare - (x > border) && (x < (max-border))
    (24 > 12) and (24 < (16+12)%32)
*/