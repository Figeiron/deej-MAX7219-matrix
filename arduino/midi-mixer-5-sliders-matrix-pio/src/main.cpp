#include <Arduino.h>

#include <MD_MAX72XX.h>
#include <MIDIUSB.h>
#include <SPI.h>

#define INVERSE_POT 1
#define BRIGHTNESS_LEVEL 0
#define DEBUG_MIDI 0 // 1 = print incoming MIDI to Serial (pio device monitor)

// #define HARDWARE_TYPE MD_MAX72XX::GENERIC_HW
#define HARDWARE_TYPE MD_MAX72XX::PAROLA_HW
// #define HARDWARE_TYPE MD_MAX72XX::ICSTATION_HW
// #define HARDWARE_TYPE MD_MAX72XX::CUSTOM_HW
// #define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4
// Pro Micro hardware SPI: CLK = 15, DIN (MOSI) = 16
#define CS_PIN 3

MD_MAX72XX matrix = MD_MAX72XX(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);

const int NUM_SLIDERS = 5;
const int analogInputs[NUM_SLIDERS] = {A0, A1, A2, A3, A10}; // A10 = pin 10
const int NUM_BUTTONS = 6;
const int buttonInputs[NUM_BUTTONS] = {9, 8, 7, 6, 5, 4};

// Which slider's cursor blinks when MIDI Mixer reports the button's mute as on (-1 = none)
const int buttonSlider[NUM_BUTTONS] = {0, 1, 2, 3, 4, -1};

const uint8_t MIDI_CHANNEL = 0;     // channel 1
const uint8_t SLIDER_CC_BASE = 1;   // sliders -> CC1..CC5
const uint8_t BUTTON_NOTE_BASE = 60; // buttons -> notes 60..65

const uint8_t cursor_rows[NUM_SLIDERS][8] = {
    {0, 1, 2, 3, 4, 5, 6, 7},
    {7, 6},
    {5, 4},
    {2, 3},
    {0, 1}};

const uint8_t cursor_sizes[NUM_SLIDERS] = {8, 2, 2, 2, 2};

const int MAX_ANALOG_VALUE = 1023;
const int MAX_MIDI_VALUE = 127;
const int NUM_COLUMNS = 32;
const int POT_HYSTERESIS = 5;
const unsigned long MATRIX_REFRESH_MS = 30;
const unsigned long BLINK_MS = 250;

int smoothedValues[NUM_SLIDERS];
int sentValues[NUM_SLIDERS];
int buttonValues[NUM_BUTTONS];

// What the matrix shows: volume reported back by MIDI Mixer (or local pot until it answers)
uint8_t displayValues[NUM_SLIDERS];
bool mutedSliders[NUM_SLIDERS];

unsigned long lastMatrixUpdate = 0;

int readSlider(int i)
{
  int value = analogRead(analogInputs[i]);
  return INVERSE_POT ? MAX_ANALOG_VALUE - value : value;
}

void sendControlChange(uint8_t control, uint8_t value)
{
  midiEventPacket_t packet = {0x0B, (uint8_t)(0xB0 | MIDI_CHANNEL), control, value};
  MidiUSB.sendMIDI(packet);
}

void sendNote(uint8_t note, bool on)
{
  midiEventPacket_t packet = {(uint8_t)(on ? 0x09 : 0x08),
                              (uint8_t)((on ? 0x90 : 0x80) | MIDI_CHANNEL),
                              note,
                              (uint8_t)(on ? 127 : 0)};
  MidiUSB.sendMIDI(packet);
}

bool updateSliderValues()
{
  bool sent = false;

  for (int i = 0; i < NUM_SLIDERS; i++)
  {
    smoothedValues[i] = (smoothedValues[i] * 3 + readSlider(i)) / 4;

    int midiValue = smoothedValues[i] >> 3; // 0..1023 -> 0..127
    int sentCenter = sentValues[i] * 8 + 4;

    // Hysteresis keeps a noisy pot from flooding MIDI Mixer with +-1 jitter
    if (sentValues[i] < 0 || (midiValue != sentValues[i] && abs(smoothedValues[i] - sentCenter) > POT_HYSTERESIS))
    {
      sentValues[i] = midiValue;
      displayValues[i] = midiValue; // show local move instantly, MIDI Mixer feedback confirms it
      sendControlChange(SLIDER_CC_BASE + i, midiValue);
      sent = true;
    }
  }

  return sent;
}

bool updateButtonsValues()
{
  bool sent = false;

  for (int i = 0; i < NUM_BUTTONS; i++)
  {
    int value = digitalRead(buttonInputs[i]);

    if (value != buttonValues[i])
    {
      buttonValues[i] = value;
      sendNote(BUTTON_NOTE_BASE + i, value == LOW); // INPUT_PULLUP: LOW = pressed
      sent = true;
    }
  }

  return sent;
}

void setButtonFeedback(int button, bool on)
{
  int slider = buttonSlider[button];

  if (slider >= 0)
  {
    mutedSliders[slider] = on;
  }
}

void readMidiFeedback()
{
  midiEventPacket_t packet;

  while ((packet = MidiUSB.read()).header != 0)
  {
    uint8_t type = packet.byte1 & 0xF0;
    uint8_t data1 = packet.byte2;
    uint8_t data2 = packet.byte3;

#if DEBUG_MIDI
    Serial.print(packet.byte1, HEX);
    Serial.print(' ');
    Serial.print(data1);
    Serial.print(' ');
    Serial.println(data2);
#endif

    if (type == 0xB0 && data1 >= SLIDER_CC_BASE && data1 < SLIDER_CC_BASE + NUM_SLIDERS)
    {
      displayValues[data1 - SLIDER_CC_BASE] = data2;
    }
    else if (type == 0xB0 && data1 >= BUTTON_NOTE_BASE && data1 < BUTTON_NOTE_BASE + NUM_BUTTONS)
    {
      // Some feedback modes send button LEDs as CC on the same number
      setButtonFeedback(data1 - BUTTON_NOTE_BASE, data2 > 0);
    }
    else if ((type == 0x90 || type == 0x80) && data1 >= BUTTON_NOTE_BASE && data1 < BUTTON_NOTE_BASE + NUM_BUTTONS)
    {
      setButtonFeedback(data1 - BUTTON_NOTE_BASE, type == 0x90 && data2 > 0);
    }
  }
}

int valueToColumn(int value)
{
  return round(value * (NUM_COLUMNS - 1) / (float)MAX_MIDI_VALUE);
}

void drawCursor(int slider, int column)
{
  for (int j = 0; j < cursor_sizes[slider]; j++)
  {
    if (cursor_rows[slider][j] < 8)
    {
      matrix.setPoint(cursor_rows[slider][j], column, true);
    }
  }
}

void updateMatrix()
{
  bool blinkOff = (millis() / BLINK_MS) % 2;

  matrix.control(MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);
  matrix.clear();

  int mainCursorPos = valueToColumn(displayValues[0]);

  if (!(mutedSliders[0] && blinkOff))
  {
    drawCursor(0, mainCursorPos);
  }

  for (int i = 1; i < NUM_SLIDERS; i++)
  {
    int new_pos = valueToColumn(displayValues[i]);

    if (new_pos == mainCursorPos || (mutedSliders[i] && blinkOff))
    {
      continue;
    }

    drawCursor(i, new_pos);
  }

  matrix.control(MD_MAX72XX::UPDATE, MD_MAX72XX::ON);
}

void initPins()
{
  for (int i = 0; i < NUM_SLIDERS; i++)
  {
    pinMode(analogInputs[i], INPUT);
  }

  for (int i = 0; i < NUM_BUTTONS; i++)
  {
    pinMode(buttonInputs[i], INPUT_PULLUP);
  }
}

void initValues()
{
  for (int i = 0; i < NUM_SLIDERS; i++)
  {
    smoothedValues[i] = readSlider(i);
    sentValues[i] = -1; // forces the first send so MIDI Mixer picks up current positions
    displayValues[i] = smoothedValues[i] >> 3;
    mutedSliders[i] = false;
  }

  for (int i = 0; i < NUM_BUTTONS; i++)
  {
    buttonValues[i] = digitalRead(buttonInputs[i]);
  }
}

void setup()
{
  Serial.begin(115200);
  matrix.begin();
  matrix.clear();
  matrix.control(MD_MAX72XX::INTENSITY, BRIGHTNESS_LEVEL);
  initPins();
  initValues();
}

void loop()
{
  bool sent = updateSliderValues();
  sent |= updateButtonsValues();

  if (sent)
  {
    MidiUSB.flush();
  }

  readMidiFeedback();

  if (millis() - lastMatrixUpdate >= MATRIX_REFRESH_MS)
  {
    lastMatrixUpdate = millis();
    updateMatrix();
  }

  delay(5); // also debounces the buttons
}
