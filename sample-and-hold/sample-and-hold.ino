
// Sample and Hold
//
//  I/O Usage:
//    Knob A0: not used
//    Knob A1: not used
//    Knob A2: attenuates input CV on A2 jack
//    Knob A3: not used
//    Analog In 1: input cv to be sampled
//    Analog In 2:
//    Digital Out 1: LED lights when have a new quantised note
//    Digital Out 2:
//    Clock In: trigger signal to begin a sample
//    Analog Out: held CV
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

volatile bool noteChanged = true;
int  sampledCV            = 0;
int  currentValue         = 0;

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
  attachInterrupt(digitalPinToInterrupt(ARDCORE_CLK), sample, RISING);

  Serial.begin(57600);
  Serial.println("Sample and hold");
}

void loop() {
  
  if (noteChanged)
  {
    noteChanged = false;
    delay(5);               // slight delay to allow CV to change after clock

    // sample the input
    int v = deJitter(analogRead(2), currentValue);
    currentValue = v;

    sampledCV = map(v, 0, 1023, 0, 255);
    Serial.println(sampledCV);

    // write it to the DAC:
    dacOutput(byte(sampledCV));

    // Blink the Ardcore D0 LED to indicate the note changed
    digitalWrite(ARDCORE_D0, HIGH);
    delay(20);
    digitalWrite(ARDCORE_D0, LOW);
  }
}

// CLK input interrupt handler
void sample()
{
  noteChanged = true;
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
