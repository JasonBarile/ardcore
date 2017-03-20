# ardcore
Sketches for the Ardcore Eurorack module

## Sample and Hold
* CV input on A2 jack is sampled on a pulse/trigger input to CLK and output on DAC

## Harmonizer
* CV input on A2 jack is sampled on pulse/trigger input to CLK and output on DAC. Knob A0 controls number of semitones offset added to the input signal. Given the 8-bit DAC on the Ardcore, it's recommended to run the DAC output through a quantizer module to shore up the CV to an actual note.
