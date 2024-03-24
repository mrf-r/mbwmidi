# MaxBandWidthMIDI

In my dreams it should be able to simultaneously receive high rate data from multiple inputs, potentiometers, encoders, switches and output (merge) it to a useful stream without creating latched notes and other problems.

The main idea is that the speed of modern microcontrollers allows them to process many more messages than can be transmitted over the communication line, and part of this power is used to limit the flow.

Unfortunately, the MIDI standard does not define message prioritization and behavior in case of insufficient channel bandwidth. Therefore, this library uses my own developed filtering logic. I hope it is compatible with most use cases. Please contact me if you notice any unwanted effects.
In any case, there is always the option to output the stream without filtering.

It consists of one common buffer for all the input messages, and multiple buffers for each output port. it doesn't matter if port is actual physical output or some internal entity like sequencer input. It should be treated as a buffer with some filtering capabilities. The input buffer does not use any filtering.

All internal data are USB Midi v1 messages, but with redefined cable numbers.

### Goals

 - full MIDI 1 support, rpn, nrpn, sysex, system
 - flush deprecated messages (for example, when previous value of CC74 is still waiting for transmission, but we already got a new one)
 - flush low priority messages (for example, loss of aftertouch message is much less critical than loss of notoff) depending on output port utilisation
 - filter rpn data if address was not set or was filtered
 - simple routing and merging. write to output port anything anytime.

### Threads

Currently, the library assumes the use of serial port peripheral interrupts to receive and send bytes.

- For the receiver, this is slightly preferable, because messages have a more precise time stamp (depending on the timer used).
- For the transmitter this is preferable, because allows you to load the transmission channel more efficiently.

In general, I don’t see much point in super-precise time processing, because the processing rate of the main loop is still limited. Most likely, the API will change slightly in the direction of simplification and eliminating the need for atomic methods (inside this library).

### Limitations/Requirements

- all ports should be inited separately, with proper sequence (TODO: example?)
- all midi related interrupts must have same priority and be atomic
- MIDI_ATOMIC_START and END must restore previous state, because it not known from where they will be called. ??????
- sysex may be corrupted when simultaneously received from multiple sources. only the first will be received. in case of uart, data will be lost if not buffered. in case of USB it is possible to hold it.


### TODO

- should port write initiate transmission??
- rpn address source is not analysed. merging rpn is vulnerable
- output: realtime could not interrupt message on serial midi - need PortRead without rp change
- review max port utilisation and filtering captured on each write:
    - getMaxUtilisation (call resets it)
    - getFilteredCount
    - getFlushedCount
- sysex soft thru?
- sysex reception for USB/i2c, where we can just wait
- change everything to polling
