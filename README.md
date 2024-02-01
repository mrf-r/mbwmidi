# HIGH BANDWIDTH MIDI

In my dreams it should be able to simultaneously receive extremely high rate data from multiple potentiometers, encoders, switches and sysex and output a useful stream.

Basically it consists of one common buffer for all the input messages, and multiple buffers for each out - ports. it doesn't matter if port is actual physical output or some internal entity like sequencer input. It should be treated as a bandwidth reducer buffer. The input buffer does not use any filtering.

All internal data are USB Midi v1 messages, but with redefined cable numbers.

### goals:
 - full MIDI 1 support, rpn, nrpn, sysex, system
 - filter same messages on output (when previous value was not sended yet, but we already got new one)
 - filter low priority messages (for ex. pressure is not as critical as noteoff) depending on output port utilisation
 - filter rpn data if address was not set or was filtered
 - simple routing and merging.

### threads:
 - main loop periodic async polling: message processing, handling running status, active sensing
 - uart tx irq
 - uart rx irq
 - usb irq: rx/tx
 - i2c irq: rx/tx

### limitations/requirements:
- all ports should be inited separately, with proper sequence (TODO: example?)
- all midi related interrupts must have same priority and be atomic
- MIDI_ATOMIC_START and END must restore previous state, because it not known from where they will be called. ??????
- sysex may be corrupted when simultaneously received from multiple sources. only the first will be received. in case of uart, data will be lost if not buffered. in case of USB it is possible to hold it.


### TODO
- special relative cc pass-through and optimisation (integrate, not replace)
- port allowable prio: utilisation or new message?
- should port write initiate transmission??
- rpn address source is not analysed. merging rpn is vulnerable
- realtime could not interrupt message on serial midi - need PortRead without rp change
- CC
- CC LSB should have higher prio (will be 3 prio for CC?)
- max port utilisation and filtering captured on each write, need:
    - getMaxUtilisation (call resets it)
    - getFilteredCount
    - getFlushedCount
- separate core, uart, USB/i2c and others (at least for now)
- fix all namings - module suffix: midiUart... midiUsb...
- sysex soft thru?
- change everything to polling???
- sysex simultaneous reception at least for USB/i2c, where we can just wait
