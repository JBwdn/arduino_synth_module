#include <Mux.h> // For fast multiplexer reading...
using namespace admux;

#include <MIDI.h>

#include <MozziGuts.h>
#include <mozzi_midi.h>
#include <Oscil.h>
#include <LowPassFilter.h>
#include <ADSR.h>

#include <tables/saw8192_int8.h>

MIDI_CREATE_DEFAULT_INSTANCE();

// Initialise multiplexer: 
Mux mux(Pin(A0, INPUT, PinType::Analog), Pinset(10, 6, 5, 3));

// Init filter:
LowPassFilter lpf;

// Setup Oscillator: 
Oscil<8192, AUDIO_RATE> aOscil(SAW8192_DATA);

// ADSR Envelopes:
ADSR<AUDIO_RATE, AUDIO_RATE> envelope;

boolean note_is_on = false;

// MIDI Debugging LED:
const int RXLED = 17;

// Parameters:
const byte attack_lev = 255;
const byte decay_lev = 150;
const byte sustain = 0;

int relative_oct = 1;

byte filter_freq = 100;
byte filter_res = 100;

unsigned int attack = 100;
unsigned int decay = 100;
unsigned int release_ms = 100;

void setup()
{
  pinMode(RXLED, OUTPUT);
  
  startMozzi();
  
  MIDI.begin(2);
  MIDI.setHandleNoteOn(moduleNoteOn);
  MIDI.setHandleNoteOff(moduleNoteOff);
  
  lpf.setResonance(filter_res);
  lpf.setCutoffFreq(filter_freq);
}

void moduleNoteOn(byte channel, byte pitch, byte velocity)
{
    digitalWrite(RXLED, HIGH);
    envelope.noteOn();
    int freq = mtof(pitch) * pow(2, relative_oct); 
    aOscil.setFreq(freq);
}

void moduleNoteOff(byte channel, byte pitch, byte velocity)
{
    digitalWrite(RXLED, LOW);
    ge.noteOff();
}

void setADSR()
{
    envelope.setADLevels(attack_lev, decay_lev);
    envelope.setTimes(attack, decay, sustain, release_ms);
}

void updateControl()
{
  relative_oct = map(mux.read(7), 0, 2017, -5, 5);
  
  attack = map(mux.read(2), 0, 1027, 10, 1000);
  decay = map(mux.read(3), 0, 1027, 10, 1000);
  release_ms = map(mux.read(4), 0, 1027, 10, 1000);
  setADSR();
    
  byte filter_freq = map(mux.read(1), 0, 1027, 1, 255);
  byte filter_res = map(mux.read(0), 0, 1027, 1, 255);
  lpf.setCutoffFreqAndResonance(filter_freq ,filter_res);
}

int updateAudio()
{
  envelope.update();
  return (int)(envelope.next() * lpf.next(aOscil.next())) >> 8;
}

void loop()
{    
  MIDI.read();
  audioHook();
}
