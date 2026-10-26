#include <Arduino.h>

#include <MD_MAX72XX.h>
#include <MIDIUSB.h>
#include <SPI.h>

#define INVERSE_POT 1
#define BRIGHTNESS_LEVEL 0
#define DEBUG_MIDI 0 // 1 = print incoming MIDI to Serial (pio device monitor)

// #define HARDWARE_TYPE MD_MAX72XX::GENERIC_HW
// #define HARDWARE_TYPE MD_MAX72XX::PAROLA_HW
// #define HARDWARE_TYPE MD_MAX72XX::ICSTATION_HW
// #define HARDWARE_TYPE MD_MAX72XX::CUSTOM_HW
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4
// Pro Micro hardware SPI: CLK = 15, DIN (MOSI) = 16
#define CS_PIN 3

MD_MAX72XX matrix = MD_MAX72XX(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);

const int NUM_SLIDERS = 5;
const int analogInputs[NUM_SLIDERS] = {A0, A1, A2, A3, A10}; // A10 = pin 10
const int NUM_BUTTONS = 6;
const int buttonInputs[NUM_BUTTONS] = {9, 8, 7, 6, 5, 4};

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
const int OVERSAMPLING = 4;
const unsigned long SAMPLE_MS = 4;
const unsigned long FRAME_MS = 8;
const unsigned long BUTTON_DEBOUNCE_MS = 15;
// While a slider is being moved, MIDI Mixer echoes our own values back a bit late;
// ignore that echo for this long so the cursor doesn't jump back and forth
const unsigned long LOCAL_HOLD_MS = 600;
// How far (0..1023) the value must go past a column edge before the cursor moves
const int COLUMN_HYSTERESIS = 8;

int smoothedValues[NUM_SLIDERS];
int sentValues[NUM_SLIDERS];
int buttonValues[NUM_BUTTONS];
unsigned long buttonChangedAt[NUM_BUTTONS];

// What the matrix shows (0..1023): the local pot while it moves, otherwise the volume from MIDI Mixer
int targetValues[NUM_SLIDERS];
unsigned long lastLocalMove[NUM_SLIDERS];
int targetColumns[NUM_SLIDERS];
int shownColumns[NUM_SLIDERS];

unsigned long lastSample = 0;
unsigned long lastFrame = 0;
bool matrixDirty = true;

int readSlider(int i)
{
  // Averaging a few reads removes most ADC noise before smoothing
  int value = 0;
  for (int k = 0; k < OVERSAMPLING; k++)
  {
    value += analogRead(analogInputs[i]);
  }
  value /= OVERSAMPLING;

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

bool updateSliderValues(unsigned long now)
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
      lastLocalMove[i] = now;
      sendControlChange(SLIDER_CC_BASE + i, midiValue);
      sent = true;
    }

    if (now - lastLocalMove[i] < LOCAL_HOLD_MS)
    {
      targetValues[i] = smoothedValues[i]; // full 10-bit resolution, not the 7-bit MIDI value
    }
  }

  return sent;
}

bool updateButtonsValues(unsigned long now)
{
  bool sent = false;

  for (int i = 0; i < NUM_BUTTONS; i++)
  {
    int value = digitalRead(buttonInputs[i]);

    if (value != buttonValues[i] && now - buttonChangedAt[i] >= BUTTON_DEBOUNCE_MS)
    {
      buttonValues[i] = value;
      buttonChangedAt[i] = now;
      sendNote(BUTTON_NOTE_BASE + i, value == LOW); // INPUT_PULLUP: LOW = pressed
      sent = true;
    }
  }

  return sent;
}

void readMidiFeedback(unsigned long now)
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
      int slider = data1 - SLIDER_CC_BASE;

      if (now - lastLocalMove[slider] >= LOCAL_HOLD_MS)
      {
        targetValues[slider] = (long)data2 * MAX_ANALOG_VALUE / MAX_MIDI_VALUE;
      }
    }
  }
}

// 0..1023 -> 0..31, staying on the current column until the value is clearly past its edge
int valueToColumn(int value, int current)
{
  const float perColumn = (MAX_ANALOG_VALUE + 1) / (float)NUM_COLUMNS;
  int column = constrain((int)(value / perColumn), 0, NUM_COLUMNS - 1);

  if (column != current &&
      value >= current * perColumn - COLUMN_HYSTERESIS &&
      value < (current + 1) * perColumn + COLUMN_HYSTERESIS)
  {
    return current;
  }

  return column;
}

// Glide shown cursors toward their targets: big jumps ease out over a few frames
void animateCursors()
{
  for (int i = 0; i < NUM_SLIDERS; i++)
  {
    targetColumns[i] = valueToColumn(targetValues[i], targetColumns[i]);

    int distance = targetColumns[i] - shownColumns[i];
    if (distance == 0)
    {
      continue;
    }

    int step = distance / 3;
    if (step == 0)
    {
      step = distance > 0 ? 1 : -1;
    }

    shownColumns[i] += step;
    matrixDirty = true;
  }
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
  animateCursors();

  // Only push to the MAX7219s when something moved
  if (!matrixDirty)
  {
    return;
  }

  matrixDirty = false;

  matrix.control(MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);
  matrix.clear();

  int mainCursorPos = shownColumns[0];
  drawCursor(0, mainCursorPos);

  for (int i = 1; i < NUM_SLIDERS; i++)
  {
    if (shownColumns[i] == mainCursorPos)
    {
      continue;
    }

    drawCursor(i, shownColumns[i]);
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
    targetValues[i] = smoothedValues[i];
    targetColumns[i] = valueToColumn(targetValues[i], 0);
    shownColumns[i] = targetColumns[i];
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
  unsigned long now = millis();
  bool sent = false;

  if (now - lastSample >= SAMPLE_MS)
  {
    lastSample = now;
    sent |= updateSliderValues(now);
  }

  sent |= updateButtonsValues(now);

  if (sent)
  {
    MidiUSB.flush();
  }

  readMidiFeedback(now);

  if (now - lastFrame >= FRAME_MS)
  {
    lastFrame = now;
    updateMatrix();
  }
}
