
// Sync LFO - a stupid LFO that outputs only positive voltages 0-5v
//
//  I/O Usage:
//    Knob A0: cycle length
//    Knob A1: not used
//    Knob A2: amplitude
//    Knob A3: not used
//    Analog In 1: not used
//    Analog In 2:
//    Digital Out 1: LED lights when cycle begins again
//    Digital Out 2:
//    Clock In: trigger signal to begin the LFO wave
//    Analog Out: LFO output
//
//  Arduino Pin Usage:
//
//    A0 = A0 knob
//    A1 = A1 knob
//    A2 = A2 CV input
//    A3 = A3 CV input
//    D0-D1 = unused
//    D2 = Ardcore CLK in (interrupt driven)
//    D3 = Ardcore D0 gate/trigger output
//    D4 = Ardcore D1 gate/trigger output
//    D5-D12 = Ardcore DAC output (pin5 is LSB, 12 is MSB)
//
//  Output Expander:  not used
//
//  Tip: Make sure the A2 knob is turned fully CW to allow
//       reading the voltage on A2 input without attenuation.
//

const int ARDCORE_CLK     = 2;
const int ARDCORE_D0      = 3;
const int ARDCORE_D1      = 4;
const int ARDCORE_DAC_LSB = 5;

byte sineWave[] = {0, 0, 0, 0, 1, 1, 1, 2, 2, 3, 4, 5, 6, 6, 7, 9, 10, 11, 12, 14, 15, 17, 18, 20, 22, 23, 25, 27, 29, 31, 33, 35, 37, 40, 42, 44, 47, 49, 52, 54, 57, 59, 62, 65, 68, 70, 73, 76, 79, 82, 85, 88, 91, 94, 97, 100, 103, 106, 109, 112, 115, 118, 122, 125, 128, 131, 134, 137, 140, 143, 146, 150, 153, 156, 159, 162, 165, 168, 171, 174, 177, 179, 182, 185, 188, 191, 193, 196, 198, 201, 204, 206, 208, 211, 213, 215, 218, 220, 222, 224, 226, 228, 230, 232, 233, 235, 237, 238, 240, 241, 242, 244, 245, 246, 247, 248, 249, 250, 251, 251, 252, 252, 253, 253, 254, 254, 254, 254, 254, 254, 254, 254, 253, 253, 252, 252, 251, 251, 250, 249, 248, 247, 246, 245, 244, 242, 241, 240, 238, 237, 235, 233, 232, 230, 228, 226, 224, 222, 220, 218, 215, 213, 211, 208, 206, 204, 201, 198, 196, 193, 191, 188, 185, 182, 179, 177, 174, 171, 168, 165, 162, 159, 156, 153, 150, 146, 143, 140, 137, 134, 131, 128, 125, 122, 118, 115, 112, 109, 106, 103, 100, 97, 94, 91, 88, 85, 82, 79, 76, 73, 70, 68, 65, 62, 59, 57, 54, 52, 49, 47, 44, 42, 40, 37, 35, 33, 31, 29, 27, 25, 23, 22, 20, 18, 17, 15, 14, 12, 11, 10, 9, 7, 6, 6, 5, 4, 3, 2, 2, 1, 1, 1, 0, 0, 0, 0};

volatile bool triggered = false;

int index = 0;
int freq = 0;               // range 0-1023
unsigned int freqDelay = 0;          // range 0-1023
int currentValue = 0;
float amp = 0.0;
float last_amp = 0.0;
unsigned int f = 0;
unsigned int last_f = 0;
unsigned long startMicros = 0;
unsigned long endMicros = 0;

int loops = 0;


void setup() {

  // set up the digital outputs
  pinMode(ARDCORE_D0, OUTPUT);
  digitalWrite(ARDCORE_D0, LOW);

  // set up the 8-bit DAC output pins
  for (int i = 0; i < 8; i++) {
    pinMode(ARDCORE_DAC_LSB + i, OUTPUT);
    digitalWrite(ARDCORE_DAC_LSB + i, LOW);
  }

  // set up the CLK trigger input
  attachInterrupt(digitalPinToInterrupt(ARDCORE_CLK), clk, RISING);

  Serial.begin(57600);
  Serial.println("Syncable LFO");
}

void loop() {

  // we're timing 100 loops to get a sense of how long this routine takes
  if (loops++ == 0) {
    startMicros = micros();
  }

  if (triggered)
  {
    triggered = false;

    // Blink the Ardcore D0 LED to indicate start of cycle
    digitalWrite(ARDCORE_D0, HIGH);
    delay(10);
    digitalWrite(ARDCORE_D0, LOW);

    // reset index into sine table
    index = 0;

    // sample the input
    f = analogRead(0);
    if (f != last_f)
    {
      last_f = f;
      Serial.print("A0 = ");
      Serial.println(f);
    }

    freqDelay = 10 * f;
    Serial.print("freqDelay = ");
    Serial.println(freqDelay);

    amp = (float(analogRead(2)) / 1023.0);
    if (amp != last_amp)
    {
      last_amp = amp;
      Serial.print("Amp = ");
      Serial.println(amp);
    }
  }

  index += 1;

  delayMicroseconds(freqDelay);    // hack hack

  // write it to the DAC:
  // only play the wave once
  if (index < 255) {
    dacOutput(sineWave[index] * amp);
  }

  if (loops == 100) {
    endMicros = micros();
    Serial.print("100 loops took ");
    Serial.println(endMicros - startMicros);
    Serial.println(" microseconds");
  }
}

// CLK input interrupt handler
void clk()
{
  triggered = true;
}

//  dacOutput(byte) - deal with the DAC output
//  -----------------------------------------
void dacOutput(byte v)
{
  PORTB = (PORTB & B11100000) | (v >> 3);
  PORTD = (PORTD & B00011111) | ((v & B00000111) << 5);
}

//  deJitter(int, int) - smooth jitter input
//  ----------------------------------------
int deJitter(int v, int test)
{
  if (abs(v - test) > 8) {
    return v;
  }
  return test;
}
