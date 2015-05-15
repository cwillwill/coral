/* Coral: Wooden, Electronic, Memory Audio Game for the Blind
Developed by Luke Hottinger and Chris Williams
*/

#include <Wire.h>
#include <SPI.h>
#include "Adafruit_BLE_UART.h"
#include "box_sound.h"

#define ADAFRUITBLE_REQ 10
#define ADAFRUITBLE_RDY 2
#define ADAFRUITBLE_RST 9

bool bleConnection = false;
bool menu = true;
bool otherPlayer = false;
bool stateChange = false;
bool wait = false;

const int MPU = 0x68;
int16_t AcX, AcY, AcZ, Tmp, GyX, GyY, GyZ;

const int stateHistoryLength = 3;
int stateHistory[stateHistoryLength];
int stateIndex = 0;
int state;
int side;
const int maxTones = 100;
int tones[maxTones];
int toneIndex = 0;
int toneTracker = 0;

int gesture = -1;

int audioPin0 = A0;
int audioPin1 = A1;

Adafruit_BLE_UART uart = Adafruit_BLE_UART(ADAFRUITBLE_REQ, ADAFRUITBLE_RDY, ADAFRUITBLE_RST);

/* Arduino Setup Function */
void setup()
{
  /* Start The Bluetooth Module */
  uart.setRXcallback(rxCallback);
  uart.setACIcallback(aciCallback);
  uart.setDeviceName("Box");
  uart.begin();

  /* Start The Accelerometer */
  Wire.begin();
  Wire.beginTransmission(MPU);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(true);
  Serial.begin(115200);

  createEmptyToneArray();

  playStartUpTone();
}

/* Arduino Loop Function */
void loop()
{
  /* Poll The Bluetooth Module */
  uart.pollACI();

  /* Poll The Accelerometer */
  Wire.beginTransmission(MPU);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU, 14, true);

  if (state == 4)
  {
    clearToMenu();
  }

  if (menu)
  {
    menuMode();
    delay(55);
  }
  else
  {
    /* 2P Mode */
    if (otherPlayer)
    {
      twoPlayerMode();
    }

    /* 1P Mode */
    else
    {
      onePlayerMode();
      /* Set Active Device Heartbeat */
      delay(55);
    }
  }

  //printState();
}

void addToneToArray(int side)
{
  tones[toneIndex] = side;
  toneIndex += 1;
}

void playSideTone(int side)
{
  if (side == 1)
  {
    delay(200);
    playSideOneTone();
  }
  else if (side == 2)
  {
    delay(200);
    playSideTwoTone();
  }
  else if (side == 3)
  {
    delay(200);
    playSideThreeTone();
  }
  else
  {
    delay(200);
    playSideFourTone();
  }
}

void createEmptyToneArray()
{
  toneIndex = 0;
  for (int i = 0; i < maxTones; i++)
  {
    tones[i] = -1;
  }
}

void clearToMenu()
{
  menu = true;
  otherPlayer = false;
  delay(1000);
  playStartUpTone();
  wait = false;
  createEmptyToneArray();
}

void menuMode()
{
  devicePoll();
  if (stateChange == true)
  {
    if (true)
    {
      //printState();
      playMenuTone();
      menuAction(state);
      delay(200);
    }
  }
}

void onePlayerMode()
{
  if (wait)
  {
    //printState();
    delay(100);
    if (gameAction())
    {
      delay(100);
      playCorrectTone();
      delay(500);
      wait = false;
      toneTracker = 0;
    }
    else
    {
      clearToMenu();
    }
  }
  else
  {
    side = random(0, 4);
    Serial.println(side);
    addToneToArray(side);
    playTones();
    wait = true;
  }
}

void playTones()
{
  for (int i = 0; i < toneIndex; i++)
  {
    int sound = tones[i];
    if (sound == 1)
    {
      delay(200);
      playSideOneTone();
    }
    else if (sound == 2)
    {
      delay(200);
      playSideTwoTone();
    }
    else if (sound == 3)
    {
      delay(200);
      playSideThreeTone();
    }
    else
    {
      delay(200);
      playSideFourTone();
    }
  }
}

void twoPlayerMode()
{
  devicePoll();
  if (bleConnection)
  {
    /* Set Active Device Heartbeat */
    delay(55);
  }
  else
  {
    playWaitingForConnectionTone();

    /* Set Dormant Device Heartbeat */
    delay(500);
  }
}

bool checkKnock()
{
  int mic = analogRead(A2) / 100;
  if (mic != 4 && mic != 5)
  {
    return true;
  }
  else
  {
    return false;
  }
}

void menuAction(int menuState)
{
  if (menuState == 3)
  {
    menu = false;
  }

  if (menuState == 2)
  {
    menu = false;
    otherPlayer = true;
  }
}

bool gameAction()
{
  bool completed = false;
  while(!completed)
  {
    Wire.beginTransmission(MPU);
    Wire.write(0x3B);
    Wire.endTransmission(false);
    Wire.requestFrom(MPU, 14, true);
    devicePoll();
    printState();
    //Serial.println("FUCK");
    if (stateChange == true)
    {
      if (true)
      {
        playMenuTone();
        if (tones[toneTracker] == state)
        {
          toneTracker += 1;
          if (toneTracker == toneIndex)
          {
            return true;
          }
        }
        else
        {
          return false;
        }
      }
    }
    delay(55);
  }
}

bool gameActionSingle(int gameState)
{
  Serial.println(gameState);
  if (gameState == 3)
  {
    if (side == 3)
    {
      playCorrectTone();
      return true;
    }
    else
    {
      playWrongTone();
      //playGameOverTone();
      return false;
    }
  }

  if (gameState == 1)
  {
    if (side == 1)
    {
      playCorrectTone();
      return true;
    }
    else
    {
      playWrongTone();
      //playGameOverTone();
      return false;
    }
  }

  if (gameState == 2)
  {
    if (side == 2)
    {
      playCorrectTone();
      return true;
    }
    else
    {
      playWrongTone();
      //playGameOverTone();
      return false;
    }
  }

  if (gameState == 0)
  {
    if (side == 0)
    {
      playCorrectTone();
      return true;
    }
    else
    {
      playWrongTone();
      //playGameOverTone();
      return false;
    }
  }
}

void devicePoll()
{
  readVals();
  handleState();
}

/* Read Accelerometer Values */
void readVals()
{
  AcX = Wire.read() << 8|Wire.read();    
  AcY = Wire.read() << 8|Wire.read();
  AcZ = Wire.read() << 8|Wire.read();
  Tmp = Wire.read() << 8|Wire.read();
  GyX = Wire.read() << 8|Wire.read();
  GyY = Wire.read() << 8|Wire.read();
  GyZ = Wire.read() << 8|Wire.read();
}

void handleState()
{
  state = detectState();
  if (state == 5 || state == -1)
  {
    stateChange = false;
  }
  else
  {
    stateChange = true;
  }
}

void checkGesture()
{
  if (false)
  {
    //return 1;
  }
  if (false)
  {
    //return 2;
  }
}

/* Detect The Active Side */
int detectState()
{
  String active = activeAxis();
  if (active == "x")
  {
    if (AcX > 0) { return 0; }
    else { return 1; }
  }
  if (active == "y")
  {
    if (AcY > 0) { return 5; }
    else { return 4; }
  }
  if (active == "z")
  {
    if (AcZ > 0) { return 2; }
    else { return 3; }
  }
  else { return -1; }
}

/* Detect The Active Axis */
String activeAxis()
{
  if (abs(AcX) >= (abs(AcY) + abs(AcZ))) { return "x"; }
  else if (abs(AcY) >= (abs(AcX) + abs(AcZ))) { return "y"; }
  else if (abs(AcZ) >= (abs(AcY) + abs(AcX))) { return "z"; }
  else { return "w"; }
}

/* Print The Active Side */
void printState()
{
  /*
  for (int i = 0; i < sizeof(stateHistory); i++)
  {
    Serial.print(String(stateHistory[i]) + " ");
  }
  Serial.println("");*/
  Serial.println(state);
}

/* Print The Raw Accelerometer Data */
void printValsToSerial()
{
  Serial.print("AcX = ");
  Serial.print(AcX);
  Serial.print(" | AcY = ");
  Serial.print(AcY);
  Serial.print(" | AcZ = ");
  Serial.print(AcZ);
  Serial.print(" | Tmp = ");
  Serial.print(Tmp / 340.00 + 36.53);
  Serial.print(" | GyX = ");
  Serial.print(GyX);
  Serial.print(" | GyY = ");
  Serial.print(GyY);
  Serial.print(" | GyZ = ");
  Serial.println(GyZ);
}

/* BLE ACI Event Handler */
void aciCallback(aci_evt_opcode_t event)
{
  switch(event)
  {
    case ACI_EVT_DEVICE_STARTED:
      Serial.println(F("Advertising started"));
      break;
    case ACI_EVT_CONNECTED:
      Serial.println(F("Connected!"));
      bleConnection = true;
      break;
    case ACI_EVT_DISCONNECTED:
      Serial.println(F("Disconnected or advertising timed out"));
      bleConnection = false;
      break;
    default:
      break;
  }
}

/* BLE RX Data Handler */
void rxCallback(uint8_t *buffer, uint8_t len)
{
  String s = "";
  for(int i = 0; i < len; i++)
  {
    s += (char)buffer[i]; 
    switch ((char)buffer[i])
    {
      case 'a':
        playStartUpTone();
        break;

      case 'b':
        playDormantTone();
        break;

      case 'c':
        playMenuTone();
        break;

      case 'd':
        playOnePlayerTone();
        break;

      case 'e':
        playTwoPlayerTone();
        break;

      case 'f':
        playWaitingForConnectionTone();
        break;

      case 'g':
        playCorrectTone();
        break;

      case 'h':
        playWrongTone();
        break;

      case 'i':
        playGameOverTone();
        break;

      case '1':
        playSideOneTone();
        break;

      case '2':
        playSideTwoTone();
        break;

      case '3':
        playSideThreeTone();
        break;

      case '4':
        playSideFourTone();
        break;

      default:
        break;
    }
    Serial.println(s);
  }
}

/***********************************************************/
/************************ Sounds ***************************/
/***********************************************************/

void playStartUpTone()
{
  tone(audioPin0, noteF6s, 150);
  delay(150);
  tone(audioPin0, noteD7s, 150);
  delay(150);
  tone(audioPin0, noteA6s, 150);
  delay(150);
}

void playMenuTone()
{
  tone(audioPin0, noteA6, 150);
  delay(150);
  tone(audioPin0, noteF6, 150);
  delay(150);
}

/* Plays Tone To Let User Know The Devices Is Disconnected */
void playDormantTone()
{
  tone(audioPin0, noteC3, 120);
  delay(250);
  tone(audioPin0, noteA2, 120);
  delay(250);
}

void playOnePlayerTone()
{
  // Tones
}

void playTwoPlayerTone()
{
  // Tones
}

void playWaitingForConnectionTone()
{
  tone(audioPin0, noteC3, 150);
  delay(300);
  tone(audioPin0, noteA2, 150);
  delay(300);
}

void playSideOneTone()
{
  tone(audioPin0, noteF4, 300);
  delay(300);
}

void playSideTwoTone()
{
  tone(audioPin0, noteA4, 300);
  delay(300);
}

void playSideThreeTone()
{
  tone(audioPin0, noteC5, 300);
  delay(300);
}

void playSideFourTone()
{
  tone(audioPin0, noteF5, 300);
  delay(300);
}

void playCorrectTone()
{
  tone(audioPin0, noteG6, 150);
  delay(300);
  tone(audioPin0, noteG6, 150);
  delay(300);
}

void playWrongTone()
{
  tone(audioPin0, noteG1, 150);
  delay(300);
  tone(audioPin0, noteG1, 150);
  delay(300);
}

void playGameOverTone()
{
  tone(audioPin0, noteD7s, 150);
  delay(150);
  tone(audioPin0, noteF6s, 150);
  delay(150);
  tone(audioPin0, noteA6s, 150);
  delay(150);
}
