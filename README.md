# RBBB-Clock

A word clock (in German) based on the Arduino RBBB.

## Pins

### Hours

Pins `D8` to `D11` output the current hour as a 4-bit integer. This is passed to
a 4-line decoder to select the correct LED.

### Minutes

The following pins control LEDs for minutes:

- `A1`: vor
- `A2`: nach
- `A3`: zehn
- `A4`: halb
- `A5`: viertel
- `D4`: drei
- `D5`: fünf

### Switches

The increment/decrement switch connects `GND` to `D2`/`D3` respectively.
