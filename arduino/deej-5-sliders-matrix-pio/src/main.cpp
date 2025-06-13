#include <Arduino.h>

#include <MD_MAX72XX.h>
#include <SPI.h>

#define INVERSE_POT 1
#define BRIBRIGHTNESS_LEVEL 0

// #define HARDWARE_TYPE MD_MAX72XX::GENERIC_HW
#define HARDWARE_TYPE MD_MAX72XX::PAROLA_HW
// #define HARDWARE_TYPE MD_MAX72XX::ICSTATION_HW
// #define HARDWARE_TYPE MD_MAX72XX::CUSTOM_HW
// #define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4
#define CLK_PIN 13
#define DATA_PIN 11
#define CS_PIN 10

MD_MAX72XX matrix = MD_MAX72XX(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);

const int NUM_SLIDERS = 5;
const int analogInputs[NUM_SLIDERS] = {A0, A1, A2, A3, A4};
const int NUM_BUTTONS = 6;
const int buttonInputs[NUM_BUTTONS] = {9, 8, 7, 6, 5, 4};

int cursorPositions[NUM_SLIDERS] = {0, 0, 0, 0, 0};

int analogSliderValues[NUM_SLIDERS];
int buttonValues[NUM_BUTTONS];

const uint8_t cursor_rows[NUM_SLIDERS][8] = {
    {0, 1, 2, 3, 4, 5, 6, 7},
    {7, 6},
    {5, 4},
    {2, 3},
    {0, 1}};

const uint8_t cursor_sizes[NUM_SLIDERS] = {8, 2, 2, 2, 2};

const int MAX_ANALOG_VALUE = 1023;
const int NUM_COLUMNS = 32;
const float num_per_1 = (float)MAX_ANALOG_VALUE / NUM_COLUMNS;

void updateSliderValues()
{
  for (int i = 0; i < NUM_SLIDERS; i++)
  {
    if (INVERSE_POT)
    {
      analogSliderValues[i] = MAX_ANALOG_VALUE - analogRead(analogInputs[i]);
    }
    else
    {
      analogSliderValues[i] = analogRead(analogInputs[i]);
    }
  }
}

void updateButtonsValues()
{
  for (int i = 0; i < NUM_BUTTONS; i++)
  {
    buttonValues[i] = digitalRead(buttonInputs[i]);
  }
}

void updateMatrix()
{
  matrix.clear();

  int mainCursorPos = round(analogSliderValues[0] * (NUM_COLUMNS - 1) / (float)MAX_ANALOG_VALUE);
  cursorPositions[0] = mainCursorPos;

  for (int j = 0; j < cursor_sizes[0]; j++)
  {
    if (cursor_rows[0][j] < 8)
    {
      matrix.setPoint(cursor_rows[0][j], mainCursorPos, true);
    }
  }

  for (int i = 1; i < NUM_SLIDERS; i++)
  {
    int new_pos = round(analogSliderValues[i] * (NUM_COLUMNS - 1) / (float)MAX_ANALOG_VALUE);

    if (new_pos == mainCursorPos)
    {
      continue;
    }

    cursorPositions[i] = new_pos;

    for (int j = 0; j < cursor_sizes[i]; j++)
    {
      if (cursor_rows[i][j] < 8)
      {
        matrix.setPoint(cursor_rows[i][j], new_pos, true);
      }
    }
  }

  matrix.update();
}

void sendSliderValues()
{
  String builtString = String("");

  for (int i = 0; i < NUM_SLIDERS; i++)
  {
    builtString += "s";
    builtString += String((int)analogSliderValues[i]);

    if (i < NUM_SLIDERS - 1)
    {
      builtString += String("|");
    }
  }

  if (NUM_BUTTONS > 0)
  {
    builtString += String("|");
  }

  for (int i = 0; i < NUM_BUTTONS; i++)
  {
    builtString += "b";
    builtString += String((int)buttonValues[i]);

    if (i < NUM_BUTTONS - 1)
    {
      builtString += String("|");
    }
  }

  Serial.println(builtString);
}

void printSliderValues()
{
  for (int i = 0; i < NUM_SLIDERS; i++)
  {
    String printedString = String("Slider #") + String(i + 1) + String(": ") + String(analogSliderValues[i]) + String(" mV");
    Serial.write(printedString.c_str());

    if (i < NUM_SLIDERS - 1)
    {
      Serial.write(" | ");
    }
    else
    {
      Serial.write("\n");
    }
  }
}

void printButtonValues()
{
  for (int i = 0; i < NUM_BUTTONS; i++)
  {
    String printedString = String("Button #") + String(i + 1) + String(": ") + String(buttonValues[i]);
    Serial.write(printedString.c_str());

    if (i < NUM_BUTTONS - 1)
    {
      Serial.write(" | ");
    }
    else
    {
      Serial.write("\n");
    }
  }
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

void setup()
{
  Serial.begin(9600);
  matrix.begin();
  matrix.clear();
  matrix.control(MD_MAX72XX::INTENSITY, BRIBRIGHTNESS_LEVEL);
  initPins();
}

void loop()
{
  updateSliderValues();
  updateButtonsValues();
  updateMatrix();
  sendSliderValues(); // Actually send data (all the time)
  // printSliderValues(); // For debug
  // printButtonValues(); // For debug
  delay(10);
}
