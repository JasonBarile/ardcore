// Rotating Clock Divider

// Constants related to the Arduino Nano pin use
// Arduino framework defines A0, A1, A2, A3 to refer to relevant pins
const int clkIn = 2;           // the digital (clock) input
const int digPin[2] = {3, 4};  // the digital output pins
const int pinOffset = 5;
byte dac;
byte fn;
volatile int clkState;

void setup() {
  pinMode(clkIn, INPUT);

  // set up the 8-bit DAC output pins
  for (int i = 0; i < 8; i++) {
    pinMode(pinOffset + i, OUTPUT);
    digitalWrite(pinOffset + i, LOW);
  }

  // set up clock interrupt
  attachInterrupt(0, isr, RISING);
}

void loop() {
  //a2 is divide, map to 7 rotations
  //a3 is reset - 5v+ for reset on next tick
  //clock input
  //a1 gate width as a % of clock time
  byte divisions[] = {1, 2, 3, 4, 5, 6, 7, 8};
  byte counters[8] = {1, 2, 3, 4, 5, 6, 7, 8};
  
  byte i;

  dpout();
  byte rot = 0;
  byte dac = 0;
  int pw; //ms
  boolean reset;

  while (1) {
    if (clkState == HIGH) {
      clkState = LOW;

      //tick
      dac = 0;
      for (i = 0; i < 8; i++) {
        counters[i]--;
        if (counters[i] == 0) {
          dac = bitSet(dac, i);
          counters[i] = divisions[(i + rot) % 8];
        }
      }
      dacOutput(dac);
      
      //read rotation and reset
      rot = map(analogRead(A2), 0, 1023, 0, 7);
      reset = analogRead(A3) == 1023;
      pw = 10 + (analogRead(A1) >> 7) * 15; //10-130ms

      if (reset) {
        for (i = 0; i < 8; i++) {
          counters[i] = divisions[(i + rot) % 8];
        }
      }
      delay(pw);
    }
    else {
      dacOutput(0);
    }
  }
}

//  isr() - quickly handle interrupts from the clock input
//  ------------------------------------------------------
void isr()
{
  clkState = HIGH;
}

//  dacOutput(byte) - deal with the DAC output
//  -----------------------------------------
void dacOutput(byte v)
{
  PORTB = (PORTB & B11100000) | (v >> 3);
  PORTD = (PORTD & B00011111) | ((v & B00000111) << 5);
}

void dpout() {
  for (int i = 0; i < 2; i++) {
    pinMode(digPin[i], OUTPUT);
    digitalWrite(digPin[i], LOW);
  }
}
void dpin() {
  pinMode(digPin[0], INPUT);
  pinMode(digPin[1], INPUT);
}

void select() {
  //visual representation of A0, A1
  byte dac;

  while (1) {
    dac = 0;
    bitSet(dac, analogRead(A0) >> 7);
    bitSet(dac, analogRead(A1) >> 7);
    dacOutput(dac);
    delay(100);
  }
}

