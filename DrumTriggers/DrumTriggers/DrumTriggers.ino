//  ============================================================
//
//  Program: ArdCore Drum Trigger Sequencer
//
//  Description: This sketch provides a trigger sequence based
//               on in-memory arrays. It is based on the original 
//               sketch "AC14_GateSequencer" by Darwin Grosse.
//               The output is sent out the 8-bit output on the 
//               Expander module to be used as drum triggers.
//
//   Each pattern has 64 steps. There are multiple patterns for
//   kick, snare, and open & closed hats. Gate outs 4-7 are
//   random (controlled by knobs below)
//
//  I/O Usage:
//    Knob A0: Select between bass drum patterns (trigger out 0)
//    Knob A1: Select between snare patterns (trigger out 1)
//    Knob A2: Select between closed high hat patterns (trigger out 3)
//    Knob A3: Select between open high hat patterns (trigger out 4)
//    Knob A4: (expander): probability of random trigger on beat
//    Knob A5: (expander): density of random triggers on gates 6/7
//
//    Digital Out 1: Duplicate of gate 0
//    Digital Out 2: Duplicate of gate 1
//
//    Analog In A2: unused
//    Analog In A3: unused
//    Analog In A4: unused
//    Analog In A5: unused
//
//    Clock In: External clock input (treated as 16th notes)
//
//    Analog Out: 8-bit output (8 drum triggers)
//     gate 0 - kick
//     gate 1 - snare / clap
//     gate 2 - closed hat
//     gate 3 - open hat
//     gate 4 - random trigger on beat (1)
//     gate 5 - random trigger on beat (2)
//     gate 6 - random trigger anytime (1)
//     gate 7 - random trigger anytime (2)
//    Analog Out 11: (expander) unused
//    Analog Out 13: (expander) unused
//
//  Input Expander: unused
//  Output Expander: 8 bits of output exposed
//
//  ============================================================
//
//  License:
//
//  This software is licensed under the Creative Commons
//  "Attribution-NonCommercial license. This license allows you
//  to tweak and build upon the code for non-commercial purposes,
//  without the requirement to license derivative works on the
//  same terms.
//
//  For more information on the Creative Commons CC BY-NC license,
//  visit http://creativecommons.org/licenses/
//
//  ================= start of global section ==================

//  constants related to the Arduino Nano pin use
const int clkIn = 2;           // the digital (clock) input
const int digPin[2] = {3, 4};  // the digital output pins
const int pinOffset = 5;       // the first DAC pin (from 5-12)
const int triggerTime = 10;    // 10 ms trigger time
const int totalSteps = 64;
const int maxPatternIndex = 3;  // 0 to (N patterns - 1)

//  variables for interrupt handling of the clock input
volatile int clkState = LOW;
int clkStep = 0;

// Pattern Definitions. Make sure each instrument has maxPatternIndex+1 number of
// different patterns.

// kick drum patterns
char kickSeqs[4][64] = {
  {1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,1,0},     // 4 on the floor yo!
  {1,0,0,0,1,0,0,0,1,0,0,1,0,0,0,0,1,0,0,0,1,0,0,0,1,0,0,1,0,0,0,0,1,0,0,0,1,0,0,0,1,0,0,1,0,0,0,0,1,0,0,0,1,0,0,0,1,0,0,1,0,0,0,0},     // drumcell
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},     // 1 only
  {1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0}};    // 1 and 3

char snareSeqs[4][64] = {
  {0,0,0,0,1,0,0,0,0,0,0,0,1,0,1,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,1,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,1,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,1,0},     // snare or clap
  {0,0,0,0,1,0,0,0,0,0,0,0,1,0,1,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,1,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,1,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,1,0},     // snare or clap
  {0,0,0,0,1,0,0,0,0,0,0,0,1,0,1,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,1,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,1,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,1,0},     // snare or clap
  {0,0,0,0,1,0,0,0,0,0,0,0,1,0,1,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,1,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,1,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,1,0}};    // snare or clap

char closedHatSeqs[4][64] = {
  {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},     // 16ths
  {1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0},     // disco!
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},     // off
  {0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1}};    // 16ths

char openHatSeqs[4][64] = {
  {0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0},     // offbeats (disco 1)
  {0,0,1,0,0,0,1,1,0,0,1,0,0,0,1,1,0,0,1,0,0,0,1,1,0,0,1,0,0,0,1,1,0,0,1,0,0,0,1,1,0,0,1,0,0,0,1,1,0,0,1,0,0,0,1,1,0,0,1,0,0,0,1,1},     // disco 2
  {0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0},     // offbeats (disco 1)
  {0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0}};    // offbeats (disco 1)

// timer variables
unsigned long startOfTriggerMs = 0;
bool trigState = false;
byte kickPattern = 0;
byte snarePattern = 0;
byte closedHatPattern = 0; 
byte openHatPattern = 0;

//  ==================== start of setup() ======================

void setup() {
  // set up the digital (clock) input
  pinMode(clkIn, INPUT);

//  // set up the digital outputs
//  for (int i=0; i<2; i++) {
//    pinMode(digPin[i], OUTPUT);
//    digitalWrite(digPin[i], LOW);
//  }
  
  // set up the 8-bit DAC output pins
  for (int i=0; i<8; i++) {
    pinMode(pinOffset+i, OUTPUT);
    digitalWrite(pinOffset+i, LOW);
  }

  // set up the interrupt  
  attachInterrupt(0, isr, RISING);
}

//  ==================== start of loop() =======================

void loop()
{
  int tmpClock = 0;

  // Read the knobs
  kickPattern = (byte) map(analogRead(0) ,0 , 1023, 0, maxPatternIndex);
  snarePattern = (byte) map(analogRead(1) ,0 , 1023, 0, maxPatternIndex);
  closedHatPattern = (byte) map(analogRead(2) ,0 , 1023, 0, maxPatternIndex);
  openHatPattern = (byte) map(analogRead(3) ,0 , 1023, 0, maxPatternIndex);

//  triggerProb = analogRead(4);  // 0-1023
//  density = analogRead(5);      // 0-1023


// TODO: what was the tmpTimer used for in the original sketch?
// knob1 (A0) used to be the tempo
  
  if (clkState == HIGH) {   // we got a trigger
    tmpClock = 1;
    clkState = LOW;
  }

//  if ((tmpTimer > 0) && ((millis() - clkMilli) > tmpTimer)) {
//    tmpClock = 1;
//  }
  
  if (tmpClock) {
    // update the time
    startOfTriggerMs = millis();  // time at start of trigger

    // output the triggers:
    digitalWrite(pinOffset,   kickSeqs[kickPattern][clkStep]);
    digitalWrite(pinOffset+1, snareSeqs[snarePattern][clkStep]);
    digitalWrite(pinOffset+2, closedHatSeqs[closedHatPattern][clkStep]);
    digitalWrite(pinOffset+3, openHatSeqs[openHatPattern][clkStep]);

    // TODO: 4-5 = random on beat (respecting the proability knob setting)
    // TODO: 6-7 = random anytime (respecting the density knob setting)

    // update the step index
    clkStep++;
    if (clkStep >= totalSteps) {
      clkStep = 0;
    }
    
    trigState = true;
  }

  // is it time to turn off the triggers yet?
  if (trigState && ((millis() - startOfTriggerMs) > triggerTime)) {
    trigState = false;
    for (int i=0; i<8; i++) {
      digitalWrite(pinOffset + i, LOW);
    }
//    for (int i=0; i<2; i++) {
//      digitalWrite(digPin[i], LOW);
//    }
  }
}

//  isr() - quickly handle interrupts from the clock input
//  ------------------------------------------------------
void isr()
{
  clkState = HIGH;
}

