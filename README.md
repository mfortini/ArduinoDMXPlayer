# ArduinoDMXPlayer
Arduino DMX sequence player

It uses the DMX shield from http://www.open-electronics.org/arduino-dmx-shield-for-christmas-projects/
to send a sequence to a DMX decoder, reading it from a DMXSEQ.CSV file on the root of the connected SD card.

Uses DMXSimple library: https://code.google.com/archive/p/tinkerit/wikis/DmxSimple.wiki

The parser is quite simple:
* CSV file separator is comma (',')
* UNIX end of line characters ('\n')
* any line starting with # (hash) will be ignored
* first column is millisenconds from start of sequence (max 999999999 ms)
* next columns is a sequence of values in [0,255], one for each channel
* number of columns must match number of channels for each row
* no check on the fact that the first column is always increasing
* file must end with a newline (could be fixed)
* last state is idle state

The system will wait for pin 6 to be HIGH, then it will start the sequence. Pin 6 value will be ignored
throughout the sequence.

In case of any error (missing SD card, missing file, malformed file), the system will go in panic mode and
will start blinking all outputs very fast.

NB: it is advisable to connect a 10 kOhm pull-down resistor between pin 6 and GND to avoid spurious
start of sequence detection due to floating input.
