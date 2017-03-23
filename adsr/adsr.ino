
// ADSR
//
//  I/O Usage:
//    Knob A0: attack length
//    Knob A1: decay length
//    Knob A2: sustain level
//    Knob A3: release length
//    Analog In 1 (A2): CV control over sustain level (attenuated by A2 knob)
//    Analog In 2 (A3): CV control over release length (attenuated by A3 knob) 
//    Digital Out 1: LED lights when cycle begins again
//    Digital Out 2: not used
//    Clock In: trigger signal to begin the ADSR cycle. Send GATE in here to utilize sustain.
//    Analog Out: ADSR output (0-5v max)
//
//  Arduino Pin Usage:
//
//    A0 = A0 knob
//    A1 = A1 knob
//    A2 = A2 knob/CV input
//    A3 = A3 knob/CV input
//    D0-D1 = unused
//    D2 = Ardcore CLK in (interrupt driven)
//    D3 = Ardcore D0 gate/trigger output
//    D4 = Ardcore D1 gate/trigger output
//    D5-D12 = Ardcore DAC output (pin5 is LSB, 12 is MSB)
//
//  Output Expander:  not used

const int ARDCORE_CLK     = 2;
const int ARDCORE_D0      = 3;
const int ARDCORE_D1      = 4;
const int ARDCORE_DAC_LSB = 5;

// Envelope Stages:
const byte ADSR_IDLE       = 0;
const byte ATTACK          = 1;
const byte DECAY           = 2;
const byte SUSTAIN         = 3;
const byte RELEASE         = 4;

byte stage = ADSR_IDLE;    // the current stage of the envelope

volatile int clk_state = LOW;

int currentValue = 0;
int attack = 0;
int decay = 0;
int sustain = 0;
int gate_level = 0;

// delta per iteration to apply to the output level
float attack_delta = 0;
float decay_delta = 0;
float release_delta = 0;

int rel = 0;
float output_level = 0.0; // current output level
int loops_in_stage = 0;   // how many iterations have we processed in the current stage


void setup() {

  // set up the digital outputs
  pinMode(ARDCORE_D0, OUTPUT);
  digitalWrite(ARDCORE_D0, LOW);

  // set up the 8-bit DAC output pins
  for (int i = 0; i < 8; i++) {
    pinMode(ARDCORE_DAC_LSB + i, OUTPUT);
    digitalWrite(ARDCORE_DAC_LSB + i, LOW);
  }

  attachInterrupt(digitalPinToInterrupt(ARDCORE_CLK), isr, CHANGE);

  Serial.begin(57600);
  Serial.println("ADSR Envelope Generator");
}

void loop() {

  if (clk_state == HIGH)
  {
    // Blink the Ardcore D0 LED to indicate start of cycle
    digitalWrite(ARDCORE_D0, HIGH);
    delay(10);
    digitalWrite(ARDCORE_D0, LOW);

    // reading analog input takes ~100us (0.0001s)
    // reading all 4 takes ~400us (0.0004s) + the blink puts us at a min time of (0.0104s)

    // sample the inputs:
    attack = constrain(analogRead(0), 1, 1023);
    decay = constrain(analogRead(1), 1, 1023);
    sustain = constrain(analogRead(2), 1, 1023);
    rel = constrain(analogRead(3), 1, 1023);

    // pre-compute deltas to save time per iteration
    attack_delta = 255.0 / float(attack);
    decay_delta = (255.0 - float(sustain)) / float(decay);
    release_delta = float(sustain) / float(rel);

    stage = ATTACK;  // restart the envelope
  }

  // Talkin' 'bout my (envelope) generation...
  switch(stage) {
    case ATTACK:
      if (loops_in_stage++ >= attack) {
        loops_in_stage = 0;
        stage = DECAY;
      }
      
      output_level += attack_delta;
      break;
    
    case DECAY:
      if (loops_in_stage++ >= decay) {
        loops_in_stage = 0;
        stage = SUSTAIN;
      }
      
      output_level -= decay_delta;
      break;

    case SUSTAIN:

      // hold the DAC at sustain level until gate level drops back down under 300  (roughly 1.5v)
      if (clk_state == LOW) {
        loops_in_stage = 0;
        stage = RELEASE;
      }

      break;
      
    case RELEASE:

      // Why don't you all f...f...fade away...
      if (loops_in_stage++ >= rel) {
        loops_in_stage = 0;
        stage = ADSR_IDLE;
      }
      
      output_level -= release_delta;

      break;

    default:
      // idle
      break;
  }

  dacOutput(byte(output_level));
}

// CLK input interrupt handler
void isr()
{
  clkstate = !clk_state;
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
