#include <FastLED.h>
#include "esp_sleep.h"
#include <FastLED_NeoMatrix.h>   // Marc_Merlin's wrapper (Adafruit_GFX API on FastLED)
#include <Adafruit_GFX.h>
#include <Fonts/TomThumb.h>     // tiny 3x5 font

#define DATA_PIN    18
#define WIDTH       34   // number of columns
#define HEIGHT      16   // number of rows
#define NUM_SENSORS 6 // number of sensors here
#define NUM_LEDS    (WIDTH * HEIGHT)
// --- Calibration settings ---
#define GAMMA_VAL  2.6f     // Gamma correction factor
#define LASER_PIN 42   // GPIO42

#define SEVEN_MIN  (7 * 10000)   // 3 minutes in seconds
#define FIVE_MIN   (5 * 10000)   // 5 minutes in seconds


// int LDR_PINS[NUM_SENSORS] = {14, 13, 12, 11, 10, 9}; // Your pins
int LDR_PINS[NUM_SENSORS] = {12, 13, 14, 9, 10, 11}; // Your pins

int buttonPins[] = {21, 47, 48, 45, 40, 35, 36, 37, 39};
const char* buttonNames[] = {"b0", "b1", "b2", "b3", "b4", "b5", "b6", "b7", "b8"};
int numButtons = sizeof(buttonPins) / sizeof(buttonPins[0]);

CRGB leds[NUM_LEDS];

// Matrix object with serpentine layout
FastLED_NeoMatrix *matrix = new FastLED_NeoMatrix(
  leds, WIDTH, HEIGHT,
  NEO_MATRIX_TOP + NEO_MATRIX_LEFT + NEO_MATRIX_ROWS + NEO_MATRIX_ZIGZAG
);
// Background buffer for storing the static scoreboard
CRGB scoreboardBuffer[WIDTH * HEIGHT];


#define LONG_PRESS_TIME 950   // milliseconds for long press
#define DEBOUNCE_TIME   50    // debounce time in ms
// Per-button state tracking
unsigned long lastDebounceTime[9] = {0};
bool lastButtonState[9] = {0};
bool buttonPressed[9] = {0};
bool longPressReported[9] = {0};
unsigned long pressStartTime[9] = {0};


bool firstLoop = true;


// Function prototypes
int* checkButtonPress();
void teamSelectMenue(uint8_t buttonNo,bool isItLongPress);
void menueSelectionLadder(uint8_t buttonNo,bool isItLongPress);
void twoPlayerSelectMenue(uint8_t buttonNo,bool isItLongPress);
void threePlayerSelectMenue(uint8_t buttonNo,bool isItLongPress);
void gameModeA(int detected, bool isItLongPress);
int checkBalls();


uint8_t gammaCorrect(uint8_t value);
void drawImage(const uint32_t* imageData, uint8_t imgWidth, uint8_t imgHeight, uint8_t startX, uint8_t startY);
uint16_t XY(uint8_t x, uint8_t y);
void playAnimation(
  const uint32_t* frames[],  // array of pointers to frame data
  uint8_t frameCount,        // how many frames in animation
  uint8_t imgWidth, uint8_t imgHeight,
  uint8_t startX, uint8_t startY,
  uint16_t frameDelayMs      // delay between frames
);


//game tracking
uint8_t ChancesLeft[12] = {5,5,5,5,5,5,5,5,5,5,5,5} ;
uint8_t ChancesPerPlayer = 5;
uint8_t CurrentPlayingTeam = 0;
bool gameIsOn = false;
uint8_t PlayerCount = 0;
bool inStartPage = false;




//ball detection logics
int threshold = 900;  // Adjust for your laser brightness
bool laserBlocked[NUM_SENSORS] = {false}; // State for each sensor
unsigned long lastDetectionTime[NUM_SENSORS] = {0};
unsigned long debounceTime = 500; // ms


//Global UI states are defined by these variables
int UIstate = 0;
int selectState = 0;




unsigned long lastActionTime = 0;   // Stores when the last action happened
unsigned long currentTime = 0;      // For holding the current millis()


uint8_t brightness = 120;



const uint32_t SleepModeBackground[] PROGMEM = {
0x008BD6F5, 0x008BD6F5, 0x008CD6F5, 0x008CD6F5, 0x008CD6F5, 0x008CD6F5, 0x008CD6F5, 0x008CD6F5, 0x008CD6F5, 0x008CD6F5, 0x008CD6F5, 0x008CD6F5, 0x008CD6F5, 0x008CD6F5, 0x008BD6F5, 0x00CECECE, 0x00FFFFFF, 0x00FFFFFF, 0x008BD6F5, 0x008CD6F5, 0x008CD6F5, 0x008CD6F5, 0x008CD6F5, 0x008CD6F5, 0x008CD6F5, 0x008CD6F5, 0x008CD6F5, 0x008CD6F5, 0x008BD6F5, 0x008CD6F5, 0x008CD6F5, 0x008CD6F5, 0x008CD6F5, 0x008CD6F5, 
0x00F4F3F3, 0x00F8FCFE, 0x007ACBEB, 0x007BCBEB, 0x007BCBEB, 0x007BCBEB, 0x007BCBEB, 0x007BCBEB, 0x007BCBEB, 0x007BCBEB, 0x007BCBEB, 0x007BCBEB, 0x007BCBEB, 0x007ACBEB, 0x00CFCECE, 0x00CECECE, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x007ACBEB, 0x007BCBEB, 0x007BCBEB, 0x007BCBEB, 0x007BCBEB, 0x007BCBEB, 0x007BCBEB, 0x007BCBEB, 0x007BCBEB, 0x00FCFDFE, 0x0084CEEC, 0x007ACBEB, 0x007BCBEB, 0x007BCBEB, 0x007BCBEB, 
0x00D3D3D3, 0x00FFFFFF, 0x00F9FCFE, 0x007ACBEB, 0x007BCBEB, 0x007BCBEB, 0x007BCBEB, 0x007BCBEB, 0x007BCBEB, 0x007BCBEB, 0x007BCBEB, 0x007BCBEB, 0x007BCBEB, 0x00CECECE, 0x007E9393, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x007ACBEB, 0x007BCBEB, 0x007BCBEB, 0x007BCBEB, 0x007BCBEB, 0x007BCBEB, 0x007BCBEB, 0x007BCBEB, 0x00CCCECF, 0x00FDFDFD, 0x00FFFFFF, 0x00FFFFFF, 0x0089CFEC, 0x007BCBEB, 0x007BCBEB, 
0x00D3D3D3, 0x00FFFFFF, 0x00FFFFFF, 0x00FAFDFE, 0x007BCBEB, 0x007BCBEB, 0x007BCBEB, 0x007BCBEB, 0x007BCBEB, 0x007BCBEB, 0x007BCBEB, 0x007BCBEB, 0x00CECECE, 0x00CECECE, 0x00CECECE, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x007ACBEB, 0x007BCBEB, 0x007BCBEB, 0x007BCBEB, 0x007BCBEB, 0x00CDCECE, 0x00CECECE, 0x00CECECE, 0x00FDFDFD, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x008BD0ED, 0x007BCBEB, 
0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FAFCFE, 0x0061BDE2, 0x0061BDE2, 0x0061BDE2, 0x0061BDE2, 0x0061BDE2, 0x0061BDE2, 0x0061BDE2, 0x00CECECE, 0x00CECECE, 0x00CECECE, 0x0095ADAD, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x0061BDE2, 0x00CECECE, 0x00CECECE, 0x00CECECE, 0x00CECECE, 0x00CECECE, 0x00CECECE, 0x00FDFDFD, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x007BC4E5, 
0x00F8F9F9, 0x00A2B6B6, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FBFDFE, 0x0061BDE2, 0x0061BDE2, 0x0061BDE2, 0x0061BDE2, 0x0062BDE2, 0x00CECECE, 0x00809494, 0x00F2F2F2, 0x00F2F2F2, 0x0096AEAE, 0x0096AEAE, 0x0096AEAE, 0x00FFFFFF, 0x00FFFFFF, 0x0096AEAE, 0x00809494, 0x00CECECE, 0x00CECECE, 0x00CECECE, 0x00CECECE, 0x00CECECE, 0x00FEFEFE, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 
0x00A4B8B8, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FCFDFD, 0x0095AEB0, 0x0062BDE1, 0x00809495, 0x00809495, 0x00819494, 0x00809494, 0x00F2F2F2, 0x0096AEAE, 0x0096AEAE, 0x0096AEAE, 0x0095AEAE, 0x00FFFFFF, 0x00FFFFFF, 0x00809494, 0x00809494, 0x00819494, 0x00819494, 0x00FEFEFE, 0x00FFFFFF, 0x00FFFFFF, 0x009BB2B2, 0x0097AFAF, 0x00FBFCFC, 0x00FFFFFF, 0x00FFFFFF, 0x00A2B7B7, 0x00ECEDED, 
0x00F2EBE5, 0x00FFFFFF, 0x00FAFBFB, 0x0098AFAF, 0x0098AFAF, 0x009DB3B3, 0x00FDFDFD, 0x0096AEAE, 0x00809595, 0x00809494, 0x0096AEAE, 0x0096AEAE, 0x00F1F1F1, 0x0097AFAF, 0x0096AEAE, 0x0096AEAE, 0x0096AEAE, 0x0096AEAE, 0x0098AFAF, 0x0097AFAF, 0x00F1F1F1, 0x0096AEAE, 0x0096AEAE, 0x0096AEAE, 0x0098AFAF, 0x0098AFAF, 0x0098AFAF, 0x0096AEAE, 0x0096AEAE, 0x00FBFCFC, 0x00A0B5B5, 0x0098AFAF, 0x00ECEDED, 0x00F2F2F2, 
0x00FFFFFF, 0x00F9FAFA, 0x0098B0B0, 0x0099B1B2, 0x00C4E2ED, 0x00C4E2ED, 0x00C5E2ED, 0x0096AEAE, 0x0096AEAE, 0x0096AEAE, 0x0097AEAE, 0x00F1F1F1, 0x00F2F2F2, 0x00F1F1F1, 0x00F1F1F1, 0x0096AEAE, 0x0096AEAE, 0x0096AEAE, 0x0096AEAE, 0x003F3736, 0x003F3635, 0x003F3635, 0x003F3635, 0x003F3635, 0x003F3635, 0x003F3635, 0x003F3635, 0x0094ACAC, 0x0096AEAE, 0x00FBFCFC, 0x00FEFEFE, 0x00F2F2F2, 0x00F2F2F2, 0x00F2F2F2, 
0x00C5E3EE, 0x0099B0B0, 0x009AB2B3, 0x00C4E2ED, 0x00C5E3EE, 0x00C5E3EE, 0x00C5E3EE, 0x00C3E1EC, 0x0097AFAF, 0x00C4E2ED, 0x00C4E2ED, 0x00C6E3EE, 0x00F2F2F2, 0x00F2F2F2, 0x00F2F2F2, 0x00F0F1F1, 0x00F1F1F1, 0x0096AEAE, 0x003F3635, 0x003F3635, 0x003F3635, 0x003F3635, 0x003F3635, 0x003F3635, 0x003F3635, 0x003F3635, 0x008B7568, 0x003F3635, 0x0094ABAB, 0x009AB0B0, 0x009AB1B1, 0x00C4E0EB, 0x00C6E3EE, 0x00C6E3EE, 
0x00C5E3EE, 0x00C4E2ED, 0x009AB2B3, 0x00C5E3EE, 0x00C5E3EE, 0x00C5E3EE, 0x00C5E3EE, 0x00C5E3EE, 0x00C4E2EC, 0x00C4E2ED, 0x0049996B, 0x00C5E3EE, 0x00C7E4EE, 0x00C7E3EE, 0x00F2F2F2, 0x00F2F2F2, 0x00F2F2F2, 0x003F3635, 0x003F3635, 0x003F3635, 0x003F3635, 0x003F3635, 0x003F3635, 0x003F3635, 0x003F3635, 0x008B7568, 0x005B493C, 0x008B7568, 0x0046403F, 0x00C0DDE8, 0x00C4E2EC, 0x00C5E3EE, 0x00C5E3EE, 0x00C5E3EE, 
0x00C5E3EE, 0x00C5E3EE, 0x00C3E1EC, 0x00C5E3EE, 0x00C5E3EE, 0x00C5E3EE, 0x00C5E3EE, 0x00C5E3EE, 0x00C5E3EE, 0x00C4E2ED, 0x0049996B, 0x00549D75, 0x00539D75, 0x00C5E3EE, 0x00C7E4EE, 0x00C7E4EF, 0x005D4D43, 0x005D4D43, 0x005D4D43, 0x005D4D43, 0x005D4D43, 0x005D4D43, 0x005D4D43, 0x0058463A, 0x008B7568, 0x005B493C, 0x00221A1B, 0x0059473B, 0x008B7568, 0x0058463A, 0x00C5E3EE, 0x00C5E3EE, 0x00C5E3EE, 0x0065A586, 
0x00569E77, 0x00569E77, 0x00569E77, 0x00569E77, 0x0049996B, 0x00C5E3EE, 0x00C5E3EE, 0x00C3E2EC, 0x00569E77, 0x00569E77, 0x0049996B, 0x0049996B, 0x0049996B, 0x0049996B, 0x0049996B, 0x00C6E4EF, 0x001E1314, 0x0021191A, 0x0021191A, 0x0021191A, 0x0021191A, 0x0021191A, 0x0021191A, 0x0021191A, 0x00251D1E, 0x004D1F0D, 0x004D1F0D, 0x004D1F0D, 0x001B140F, 0x00110B08, 0x00C5E3EE, 0x00C5E3EE, 0x0049996B, 0x004A996C, 
0x0049996B, 0x0049996B, 0x0049996B, 0x0049996B, 0x0049996B, 0x0049996B, 0x00589F79, 0x00589F79, 0x0049996B, 0x0049996B, 0x0049996B, 0x0049996B, 0x0049996B, 0x0049996B, 0x0049996B, 0x00589F79, 0x00C0DDE8, 0x004D1F0D, 0x0047382F, 0x0047382F, 0x0047382F, 0x0047382F, 0x0047382F, 0x0047382F, 0x004D1F0D, 0x00755D4B, 0x00201718, 0x00755D4B, 0x004D1F0D, 0x00C5E3EE, 0x00C5E3EE, 0x0049996B, 0x0049996B, 0x0049996B, 
0x0049996B, 0x0049996B, 0x0048986A, 0x00378760, 0x00388861, 0x0049996B, 0x0049996B, 0x0049996B, 0x0049996B, 0x0049996B, 0x0049996B, 0x0049996B, 0x0049996B, 0x00E1D0E7, 0x0048996A, 0x0049996B, 0x00508169, 0x004D1F0D, 0x00493A30, 0x00493A30, 0x0020181A, 0x0020181A, 0x00493A30, 0x00493A30, 0x004D1F0D, 0x00755D4B, 0x001D1518, 0x00755D4B, 0x004D1F0D, 0x004F7F67, 0x00508169, 0x00B5AF6B, 0x00599B6B, 0x00398861, 
0x00378760, 0x00378760, 0x00378760, 0x0035855F, 0x0035855F, 0x00378660, 0x00287357, 0x00287357, 0x00287357, 0x00378760, 0x00378760, 0x0049996B, 0x0049996B, 0x0049996B, 0x0048996B, 0x00AE4A41, 0x00245E43, 0x004D1F0D, 0x00493A30, 0x00493A30, 0x001A1317, 0x001A1317, 0x00493A30, 0x00493A30, 0x004D1F0D, 0x00755D4B, 0x00755D4B, 0x00755D4B, 0x004D1F0D, 0x002B6547, 0x002A6649, 0x00476F4C, 0x0039855F, 0x0035855F, 
 
};


const uint32_t GameModeAScoreboard[] PROGMEM = {
0x00D0E9F8, 0x00CCE8F8, 0x00C9E6F9, 0x00C5E5F9, 0x00C0E3F8, 0x00BBE1F9, 0x00B6DFF9, 0x00B0DDF9, 0x00AADBFA, 0x00A3D9F9, 0x009CD7FA, 0x0095D5FA, 0x008ED2FB, 0x0086D1FB, 0x007DCFFC, 0x0075CCFB, 0x006DCBFC, 0x0065C9FC, 0x005EC7FD, 0x0056C6FD, 0x004EC5FD, 0x0047C5FD, 0x0041C4FE, 0x003CC4FE, 0x0038C2FE, 0x0035C2FE, 0x0033C2FE, 0x0032C2FE, 0x0031C2FE, 0x0031C2FF, 0x0030C2FF, 0x002FC2FF, 0x002FC2FF, 0x002EC2FF, 
0x00CEE8F8, 0x00CAE7F8, 0x00C7E6F8, 0x00C3E4F8, 0x00BEE2F8, 0x00BAE0F8, 0x00B4DFF8, 0x00AEDCF8, 0x00A8DAF9, 0x00A2D8F8, 0x009AD6F9, 0x0093D4F9, 0x008CD2FA, 0x0084D0FA, 0x007CCEFB, 0x0074CCFB, 0x006CCAFB, 0x0064C9FC, 0x005CC8FC, 0x0055C7FC, 0x004EC5FD, 0x0048C4FD, 0x0042C4FD, 0x003DC3FD, 0x003AC3FE, 0x0038C3FE, 0x0036C2FE, 0x0034C2FE, 0x0033C2FE, 0x0032C2FE, 0x0032C2FE, 0x0031C2FF, 0x0030C2FF, 0x0030C2FF, 
0x00CCE7F7, 0x00C9E6F7, 0x00C5E4F7, 0x00C1E3F7, 0x00BCE1F7, 0x00B7DFF7, 0x00B2DEF7, 0x00ACDCF7, 0x00A6DAF7, 0x009FD7F8, 0x0098D6F8, 0x0091D3F9, 0x008AD1F9, 0x0082CFF9, 0x007BCEFA, 0x0073CBFA, 0x006BCAFA, 0x0064C9FB, 0x005DC7FB, 0x0055C6FC, 0x004FC5FC, 0x0049C5FC, 0x0044C4FD, 0x0040C3FD, 0x003DC3FD, 0x003AC3FD, 0x0039C3FE, 0x0037C3FE, 0x0036C3FE, 0x0035C2FE, 0x0034C2FE, 0x0033C2FE, 0x0032C2FE, 0x0031C2FE, 
0x00CAE6F6, 0x00C7E5F6, 0x00C3E4F6, 0x00BFE2F6, 0x00BAE0F6, 0x00B5DFF6, 0x00B0DDF6, 0x00AADBF6, 0x00A4D8F6, 0x009ED7F7, 0x0097D4F7, 0x0090D3F8, 0x0089D0F7, 0x0081CFF8, 0x007ACDF9, 0x0072CBF9, 0x006BCAF9, 0x0064C8FA, 0x005DC7FA, 0x0057C7FB, 0x0050C5FB, 0x004CC5FC, 0x0046C4FC, 0x0043C4FC, 0x0040C3FD, 0x003EC3FD, 0x003CC3FD, 0x003BC3FD, 0x0039C3FE, 0x0038C3FE, 0x0037C3FE, 0x0036C2FE, 0x0035C2FE, 0x0034C2FE, 
0x00C8E6F5, 0x00C5E4F6, 0x00C1E3F5, 0x00BDE1F5, 0x00B8E0F5, 0x00B3DDF5, 0x00AEDCF5, 0x00A6D6F0, 0x008B5D43, 0x008B5D43, 0x008B5D43, 0x008B5D43, 0x008B5D43, 0x008B5D43, 0x008B5D43, 0x008B5D43, 0x008B5D43, 0x008B5D43, 0x008B5D43, 0x008B5D43, 0x008B5D43, 0x008B5D43, 0x008B5D43, 0x008B5D43, 0x008B5D43, 0x008B5D43, 0x0042C0F9, 0x003FC3FD, 0x003DC3FD, 0x003BC3FD, 0x003AC3FE, 0x0039C3FE, 0x0038C3FE, 0x0037C3FE, 
0x00C6E5F5, 0x00C2E3F5, 0x00BEE2F4, 0x00BAE0F4, 0x00B6DEF4, 0x00B1DDF4, 0x00ACDBF4, 0x00895B42, 0x008B5D43, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x006F432A, 0x006F432A, 0x006F432A, 0x006F432A, 0x006F432A, 0x006F432A, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x006F432A, 0x006F432A, 0x0043C4FC, 0x0041C3FD, 0x003FC3FD, 0x003EC3FD, 0x003CC3FD, 0x003BC3FD, 0x003AC3FE, 
0x00C3E3F4, 0x00BFE2F4, 0x00BBE0F4, 0x00B7DFF3, 0x00B3DEF3, 0x00AEDBF3, 0x00A9DAF3, 0x008B5D43, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00020100, 0x00040101, 0x006F432A, 0x006D4229, 0x006D4229, 0x006F432A, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x006F432A, 0x0048C4FC, 0x0046C4FC, 0x0044C4FC, 0x0042C4FC, 0x0041C3FD, 0x003FC3FD, 0x003EC3FD, 
0x00C0E2F4, 0x00BDE1F3, 0x00B9DFF2, 0x00B5DEF3, 0x00B0DCF2, 0x00ACDBF2, 0x00A7D9F2, 0x008B5D43, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x006F432A, 0x00000000, 0x00000000, 0x006F432A, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x006F432A, 0x004DC5FB, 0x004BC4FB, 0x0049C4FB, 0x0047C4FC, 0x0046C4FC, 0x0044C4FC, 0x0042C4FC, 
0x00BDE1F2, 0x00BAE0F2, 0x00B6DEF2, 0x00B2DCF1, 0x00ADDBF1, 0x00A9D9F1, 0x00A5D8F1, 0x008B5D43, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x006F432A, 0x006F432A, 0x006F432A, 0x006F432A, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x006F432A, 0x0053C5FA, 0x0051C5FA, 0x004FC5FA, 0x004DC5FB, 0x004BC4FB, 0x0049C4FB, 0x0047C4FC, 
0x00BAE0F2, 0x00B6DEF1, 0x00B3DDF1, 0x00AEDCF1, 0x00AADAF0, 0x00A6D8F0, 0x00A2D7EF, 0x008B5D43, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x006F432A, 0x00000000, 0x00000000, 0x006F432A, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x006F432A, 0x0058C6F9, 0x0056C6F9, 0x0054C6F9, 0x0052C5FA, 0x0050C5FA, 0x004FC5FA, 0x004DC5FB, 
0x00B6DEF1, 0x00B3DDF1, 0x00AFDBF0, 0x00ABDAF0, 0x00A7D9EF, 0x00A3D8EF, 0x009FD6EF, 0x006F432A, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x006F432A, 0x006F432A, 0x006F432A, 0x006F432A, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x006F432A, 0x005EC7F7, 0x005CC7F8, 0x005AC6F8, 0x0058C6F9, 0x0056C6F9, 0x0054C6F9, 0x0052C5FA, 
0x00B2DDF1, 0x00AFDBF0, 0x00ABDAF0, 0x00A8D9EF, 0x00A4D8EF, 0x009BD4E4, 0x008ED0D0, 0x006F432A, 0x006F432A, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x006F432A, 0x006F432A, 0x006F432A, 0x006F432A, 0x006F432A, 0x006F432A, 0x000A0402, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x006F432A, 0x006F432A, 0x0064C8F6, 0x0062C7F6, 0x0060C7F7, 0x005EC7F7, 0x005CC6F8, 0x005AC6F8, 0x0058C6F9, 
0x00A8D9E6, 0x0085CDAC, 0x0055C233, 0x0055C233, 0x0051BA31, 0x00459D2C, 0x0055C233, 0x004F311F, 0x006F432A, 0x006F432A, 0x006F432A, 0x006F432A, 0x006F432A, 0x006F432A, 0x006F432A, 0x006F432A, 0x006D4229, 0x006F432A, 0x006F432A, 0x006F432A, 0x006F432A, 0x006F432A, 0x006F432A, 0x006D4229, 0x006F432A, 0x006F432A, 0x004F311F, 0x004EB145, 0x0062C7EC, 0x0062C7EC, 0x0062C7EC, 0x0061C7F6, 0x0060C7F5, 0x005FC7F5, 
0x0057C23A, 0x0055C233, 0x0054C033, 0x004FB32F, 0x00449B29, 0x00C1F376, 0x008CD755, 0x0059B832, 0x004F311F, 0x004F311F, 0x004F311F, 0x004F311F, 0x004F311F, 0x004F311F, 0x004F311F, 0x004F311F, 0x004F311F, 0x004F311F, 0x004F311F, 0x004F311F, 0x004F311F, 0x004F311F, 0x004F311F, 0x004F311F, 0x004F311F, 0x004F311F, 0x0057BE33, 0x003E8D26, 0x0050B630, 0x00459D2A, 0x0051BA31, 0x0056C255, 0x0061C6DE, 0x0061C7E5, 
0x0051BA31, 0x00409127, 0x0033751F, 0x00409227, 0x003E8D26, 0x00439828, 0x004FB32F, 0x0054C033, 0x0055C233, 0x00409227, 0x004FB32F, 0x008B5D43, 0x006F432A, 0x006F432A, 0x004FB32F, 0x00409227, 0x00409227, 0x00409227, 0x0055C233, 0x0049A72C, 0x008B5D43, 0x006F432A, 0x006F432A, 0x0048A52C, 0x00409227, 0x0055C233, 0x004EB12F, 0x00367C21, 0x00B1E36C, 0x00B1E56C, 0x00409227, 0x0054BF32, 0x0048A62C, 0x0051BA31, 
0x0051BA31, 0x00449B2A, 0x0085BC51, 0x00C1F376, 0x00367C21, 0x00367C21, 0x00409227, 0x004FB32F, 0x0055C233, 0x004FB32F, 0x0054C033, 0x008B5D43, 0x006F432A, 0x006F432A, 0x0055C233, 0x004AA92C, 0x0033751F, 0x00409227, 0x00C1F376, 0x00409227, 0x008B5D43, 0x006F432A, 0x006F432A, 0x00419528, 0x00409227, 0x00C1F376, 0x00409227, 0x00409227, 0x006AAB40, 0x0071C044, 0x00409227, 0x004BAD2D, 0x0033751F, 0x00459D2A, 
};



const uint32_t GameModeBScoreboard[] PROGMEM = {
0x00D0E9F8, 0x00CCE8F8, 0x00C9E6F9, 0x00C5E5F9, 0x00C0E3F8, 0x00BBE1F9, 0x00B6DFF9, 0x00928E85, 0x00AADBFA, 0x00A3D9F9, 0x009CD7FA, 0x0095D5FA, 0x008ED2FB, 0x0086D1FB, 0x007DCFFC, 0x0075CCFB, 0x006DCBFC, 0x0065C9FC, 0x005EC7FD, 0x0056C6FD, 0x004EC5FD, 0x0047C5FD, 0x0041C4FE, 0x003CC4FE, 0x0038C2FE, 0x0035C2FE, 0x00928E85, 0x0032C2FE, 0x0031C2FE, 0x0031C2FF, 0x0030C2FF, 0x002FC2FF, 0x002FC2FF, 0x002EC2FF, 
0x00CEE8F8, 0x00CAE7F8, 0x00C7E6F8, 0x00C3E4F8, 0x00BEE2F8, 0x00BAE0F8, 0x00B4DFF8, 0x00928E85, 0x00A8DAF9, 0x00A2D8F8, 0x009AD6F9, 0x0093D4F9, 0x008CD2FA, 0x0084D0FA, 0x007CCEFB, 0x0074CCFB, 0x006CCAFB, 0x0064C9FC, 0x005CC8FC, 0x0055C7FC, 0x004EC5FD, 0x0048C4FD, 0x0042C4FD, 0x003DC3FD, 0x003AC3FE, 0x0038C3FE, 0x00928E85, 0x0034C2FE, 0x0033C2FE, 0x0032C2FE, 0x0041C4FC, 0x0035C2FE, 0x0030C2FF, 0x0030C2FF, 
0x00CCE7F7, 0x00C9E6F7, 0x00C5E4F7, 0x00C5E4F7, 0x00BBB8B2, 0x00BBB8B2, 0x00BBB8B2, 0x00BBB8B2, 0x00BBB8B2, 0x00BBB8B2, 0x00BBB8B2, 0x00BBB8B2, 0x00BBB8B2, 0x00BBB8B2, 0x00BBB8B2, 0x00BBB8B2, 0x00BBB8B2, 0x00BBB8B2, 0x00BBB8B2, 0x00BBB8B2, 0x00BBB8B2, 0x00BBB8B2, 0x00BBB8B2, 0x00BBB8B2, 0x00BBB8B2, 0x00BBB8B2, 0x00BBB8B2, 0x00BBB8B2, 0x00BBB8B2, 0x00BBB8B2, 0x0046C2F9, 0x0037C2FE, 0x0032C2FE, 0x0031C2FE, 
0x00CAE6F6, 0x00C7E5F6, 0x00C3E4F6, 0x00BBB8B2, 0x00BBB8B2, 0x00928E85, 0x00928E85, 0x00928E85, 0x00928E85, 0x00928E85, 0x00928E85, 0x00928E85, 0x00928E85, 0x00928E85, 0x00928E85, 0x00928E85, 0x00928E85, 0x00928E85, 0x00928E85, 0x00928E85, 0x00928E85, 0x00928E85, 0x00928E85, 0x00928E85, 0x00928E85, 0x00928E85, 0x00928E85, 0x00928E85, 0x00928E85, 0x00928E85, 0x00928E85, 0x0036C2FE, 0x0035C2FE, 0x0034C2FE, 
0x00C8E6F5, 0x00C5E4F6, 0x00C1E3F5, 0x00BBB8B2, 0x00928E85, 0x00928E85, 0x00928E85, 0x00928E85, 0x00928E85, 0x00928E85, 0x00928E85, 0x00928E85, 0x00000000, 0x00000000, 0x00928E85, 0x00928E85, 0x00928E85, 0x00928E85, 0x00928E85, 0x00928E85, 0x00928E85, 0x00000000, 0x00000000, 0x00928E85, 0x00928E85, 0x00928E85, 0x00928E85, 0x00928E85, 0x00928E85, 0x00928E85, 0x00928F87, 0x0039C3FE, 0x0038C3FE, 0x0037C3FE, 
0x00C6E5F5, 0x00C2E3F5, 0x00BEE2F4, 0x00BBB8B2, 0x00928E85, 0x00928E85, 0x00928E85, 0x00928E85, 0x00928E85, 0x00928E85, 0x00928E85, 0x00928E85, 0x00928E85, 0x00928E85, 0x00928E85, 0x00928E85, 0x00928E85, 0x00928E85, 0x00928E85, 0x00928E85, 0x00928E85, 0x00928E85, 0x00928E85, 0x00928E85, 0x00928E85, 0x00928E85, 0x00928E85, 0x00928E85, 0x00928E85, 0x00928E85, 0x00928E85, 0x003CC3FD, 0x003BC3FD, 0x003AC3FE, 
0x00C3E3F4, 0x00BFE2F4, 0x00BBE0F4, 0x00928E86, 0x00928E85, 0x00928E85, 0x00928E85, 0x00928E85, 0x00928E85, 0x00928E85, 0x00928E85, 0x00928E85, 0x00000000, 0x00000000, 0x00928E85, 0x00928E85, 0x00928E85, 0x00928E85, 0x00928E85, 0x00928E85, 0x00928E85, 0x00000000, 0x00000000, 0x00928E85, 0x00928E85, 0x00928E85, 0x00928E85, 0x00928E85, 0x00928E85, 0x00928E85, 0x00928E86, 0x0041C3FD, 0x003FC3FD, 0x003EC3FD, 
0x00C0E2F4, 0x00BDE1F3, 0x00B9DFF2, 0x0064625B, 0x00928E85, 0x00928E85, 0x00928E85, 0x00928E85, 0x00928E85, 0x00928E85, 0x00928E85, 0x00928E85, 0x008F8B82, 0x00928E85, 0x00928E85, 0x00928E85, 0x00928E85, 0x00928E85, 0x00928E85, 0x00928E85, 0x00928E85, 0x00928E85, 0x00928E85, 0x00928E85, 0x00928E85, 0x00928E85, 0x00928E85, 0x00928E85, 0x00928E85, 0x00928E85, 0x0064625B, 0x0046C4FC, 0x0044C4FC, 0x0042C4FC, 
0x00BDE1F2, 0x00BAE0F2, 0x00B6DEF2, 0x00B6DEF2, 0x0064625B, 0x0064625B, 0x0064625B, 0x0064625B, 0x0064625B, 0x0064625B, 0x0064625B, 0x0064625B, 0x0064625B, 0x0064625B, 0x0064625B, 0x0064625B, 0x0064625B, 0x0064625B, 0x0064625B, 0x0064625B, 0x0064625B, 0x0064625B, 0x0064625B, 0x0064625B, 0x0064625B, 0x0064625B, 0x0064625B, 0x0064625B, 0x0064625B, 0x0064625B, 0x0052C5FA, 0x004BC4FB, 0x0049C4FB, 0x0047C4FC, 
0x00BAE0F2, 0x00B6DEF1, 0x00B3DDF1, 0x00B1DDF1, 0x00AADAF0, 0x00A6D8F0, 0x00A2D7EF, 0x009DD5F0, 0x0099D4EF, 0x0094D3F0, 0x0090D1F0, 0x008BD0F0, 0x0087CFF0, 0x0083CDF1, 0x007ECDF0, 0x007ACCF1, 0x0076CBF2, 0x0073CAF2, 0x006FC9F3, 0x006CC9F4, 0x0069C9F4, 0x0067C8F5, 0x0064C8F6, 0x0062C7F6, 0x005FC7F7, 0x005DC7F8, 0x005BC6F8, 0x0058C6F9, 0x0056C6F9, 0x0054C6F9, 0x0052C5FA, 0x0050C5FA, 0x004FC5FA, 0x004DC5FB, 
0x00B6DEF1, 0x00B3DDF1, 0x00AFDBF0, 0x00ABDAF0, 0x00A7D9EF, 0x00A3D8EF, 0x009FD6EF, 0x009BD4EF, 0x0097D3EE, 0x0093D2EF, 0x008FD1EE, 0x008BD0EF, 0x0088CFEF, 0x0084CEF0, 0x0082CEF0, 0x007CCCF0, 0x0079CCF0, 0x0076CBF1, 0x0073CAF2, 0x006BC9F4, 0x0067C8F5, 0x0067C8F5, 0x0069C9F4, 0x0067C8F5, 0x0065C8F6, 0x0063C7F6, 0x0060C7F7, 0x005EC7F7, 0x005CC7F8, 0x005AC6F8, 0x0058C6F9, 0x0056C6F9, 0x0054C6F9, 0x0052C5FA, 
0x00B2DDF1, 0x00AFDBF0, 0x00ABDAF0, 0x00A8D9EF, 0x00A4D8EF, 0x009BD4E4, 0x008ED0D0, 0x0080CCB8, 0x0064C573, 0x0058C243, 0x0059C34F, 0x006AC7A2, 0x0084CEF0, 0x0084CEF0, 0x0084CEF0, 0x0084CEF0, 0x0055C233, 0x00C1F376, 0x0055C233, 0x005FC5BA, 0x0064C7DD, 0x0067C8F5, 0x0063C7D4, 0x0067C8F5, 0x0066C8E4, 0x0066C7EB, 0x0066C8F5, 0x0064C8F6, 0x0062C7F6, 0x0060C7F7, 0x005EC7F7, 0x005CC6F8, 0x005AC6F8, 0x0058C6F9, 
0x00A8D9E6, 0x0085CDAC, 0x0055C233, 0x0055C233, 0x0051BA31, 0x00459D2C, 0x0055C233, 0x0055C233, 0x0055C233, 0x0055C233, 0x004FB430, 0x0055C233, 0x0068C69A, 0x0084CEF0, 0x0084CEF0, 0x0055C233, 0x004BAB2D, 0x0054BF32, 0x0055C233, 0x0055C233, 0x0055C233, 0x0055C233, 0x0064C7DF, 0x0066C8F1, 0x0059C37D, 0x0050B740, 0x0052BA47, 0x0056BAA3, 0x0062C7EC, 0x0062C7EC, 0x0062C7EC, 0x0061C7F6, 0x0060C7F5, 0x005FC7F5, 
0x0057C23A, 0x0055C233, 0x0054C033, 0x004FB32F, 0x00449B29, 0x00C1F376, 0x008CD755, 0x0055C233, 0x0055C233, 0x004FB430, 0x00459C2A, 0x0055C233, 0x0055C233, 0x006AC7A2, 0x0054C033, 0x004FB32F, 0x00316F1E, 0x004BAB2D, 0x004EB12F, 0x0055C233, 0x0055C233, 0x004EB145, 0x0052BC3B, 0x0055C233, 0x0050B83F, 0x004EB145, 0x0052BA3D, 0x003E8D26, 0x0050B630, 0x00459D2A, 0x0051BA31, 0x0056C255, 0x0061C6DE, 0x0061C7E5, 
0x0051BA31, 0x00409127, 0x0033751F, 0x00409227, 0x003E8D26, 0x00439828, 0x004FB32F, 0x0054C033, 0x0055C233, 0x00409227, 0x00C1F376, 0x0055C233, 0x0055C233, 0x0055C233, 0x004FB32F, 0x00429628, 0x00409227, 0x00409227, 0x0086C051, 0x00409227, 0x0055C233, 0x0055C233, 0x0055C233, 0x00409227, 0x00409227, 0x0055C233, 0x004EB12F, 0x00367C21, 0x00B1E36C, 0x00B1E56C, 0x00409227, 0x0054BF32, 0x0048A62C, 0x0051BA31, 
0x0051BA31, 0x00449B2A, 0x0085BC51, 0x00C1F376, 0x00367C21, 0x00367C21, 0x00409227, 0x004FB32F, 0x0055C233, 0x004FB32F, 0x008ACF54, 0x00459C2A, 0x0050B630, 0x0055C233, 0x00459C2A, 0x00459C2A, 0x0033751F, 0x00409227, 0x00C1F376, 0x00409227, 0x0049A62C, 0x0055C233, 0x004FB442, 0x004CAC40, 0x00409227, 0x00C1F376, 0x00409227, 0x00409227, 0x006AAB40, 0x0071C044, 0x00409227, 0x004BAD2D, 0x0033751F, 0x00459D2A, 
};






void setup() {
  Serial.begin(115200);
  delay(500); // Give time for Serial Monitor to open

  matrix->begin();
  matrix->setTextWrap(false);
  matrix->setFont(&TomThumb);
  matrix->setTextSize(1);

  analogReadResolution(12); // ESP32-S3 (0–4095)
  // Optional: print initialization
  for (int i = 0; i < NUM_SENSORS; i++) {
    Serial.print("Sensor ");
    Serial.print(i);
    Serial.print(" on pin ");
    Serial.println(LDR_PINS[i]);
  }

  FastLED.setBrightness(brightness); // adjust as needed
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
  FastLED.clear();

  for (int i = 0; i < numButtons; i++) {
    pinMode(buttonPins[i], INPUT); // external pull-down resistors
  }
  pinMode(LASER_PIN, OUTPUT);  // Set GPIO42 as output

  // Set the wake-up source to the button (HIGH = pressed)
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_21, 1);
  delay(200);
  digitalWrite(LASER_PIN, HIGH);
}




void loop() {

  currentTime = millis();
  int* result = checkButtonPress();
  int sensorValues[NUM_SENSORS];


  // Step 1: Get all readings
  readSensors(sensorValues);

  // Step 2: Pass readings to detection function
  int detected = detectBall(sensorValues);

  if (((currentTime-lastActionTime) > FIVE_MIN)||firstLoop) {
    Serial.println("here at the start page");
    inStartPage = true;
    firstLoop = false;
    PlayerCount = 0;
    gameIsOn = false;
    UIstate = 0;
    selectState = 0;
    FastLED.clear();
    drawImage(SleepModeBackground, 34, 16, 0, 0); // draw at (4, 2)

    // Filled rounded rectangle
    matrix->fillRoundRect(1, 0, 22, 7, 2, matrix->Color(255, 180, 70));

    matrix->setCursor(2, 6);  // X=0, Y=6 baseline
    // matrix->setTextColor(matrix->Color(255, 50, 230));
    matrix->setTextColor(matrix->Color(0, 0, 0));
    matrix->print("START");
    FastLED.show();
    delay(50);
  }

  if ((currentTime-lastActionTime) > SEVEN_MIN){
    Serial.println("here at the sleep page");
    sleepMode();
    lastActionTime = millis();
  }


// int* result = checkButtonPress();
  
  if (result[0] != -1) {
    Serial.print(buttonNames[result[0]]);
    Serial.print(" - ");
    Serial.println(result[1] == 0 ? "SHORT" : "LONG");
    menueSelectionLadder(result[0],result[1]);
  }

  if ((detected != -1 || result[1] == 1)&&gameIsOn){
    Serial.println("here i am before calling gameModeA via loop");
    Serial.println(detected);
    
    if(UIstate == 3) gameModeA(detected, result[1]);
    if(UIstate == 4) gameModeB(detected, result[1]);

    //Debug: Print all readings
    Serial.print("LDR readings: ");
    for (int i = 0; i < NUM_SENSORS; i++) {
      Serial.print(sensorValues[i]);
      Serial.print(" ");
    }
    Serial.println();
    
    lastActionTime = currentTime;
  }

  
  if(result[0] == 4 || result[0] == 2){
    if(result[0] == 2) brightness += 10;
    else brightness -=10;
    brightness = constrain(brightness, 80, 230);
    FastLED.setBrightness(brightness); // adjust as needed
    delay(50);
  }



  // Serial.println("Loop ends");
}



int* checkButtonPress() {
  static int result[2] = {-1, 0};
  result[0] = -1; // default: no event

  for (int i = 0; i < numButtons; i++) {
    int reading = digitalRead(buttonPins[i]);

    // If state changed, reset debounce timer
    if (reading != lastButtonState[i]) {
      lastDebounceTime[i] = millis();
      lastButtonState[i] = reading;
    }

    // If stable for debounce time
    if ((millis() - lastDebounceTime[i]) > DEBOUNCE_TIME) {
      
      // Button just pressed
      if (reading == HIGH && !buttonPressed[i]) {
        buttonPressed[i] = true;
        longPressReported[i] = false;
        pressStartTime[i] = millis();
      }

      // While holding: detect long press instantly
      if (buttonPressed[i] && !longPressReported[i] &&
          (millis() - pressStartTime[i] >= LONG_PRESS_TIME)) {
        longPressReported[i] = true;
        result[0] = i;
        result[1] = 1; // long press
        return result; // report immediately
      }

      // Button just released (short press only if long press wasn't already reported)
      if (reading == LOW && buttonPressed[i]) {
        buttonPressed[i] = false;
        unsigned long pressDuration = millis() - pressStartTime[i];

        if (!longPressReported[i]) { // only short press if no long press was reported
          result[0] = i;
          result[1] = 0; // short press
          return result;
        }
      }
    }
  }

  return result; // no event
}

void teamSelectMenue(uint8_t buttonNo,bool isItLongPress){
  static bool firstCall = true;
  Serial.println("Here at team selection");
  // Serial.print(buttonNo);
  // Serial.print(" - ");
  // Serial.println(isItLongPress);

  FastLED.clear();
  matrix->fillRect(
  0,   // X position
  0,   // Y position
  34,  // Width
  16,   // Height
  matrix->Color(16, 122, 176) // Color (R,G,B)
  );

  if(firstCall||inStartPage){
    Serial.println("team select menue first select");
    // Filled rounded rectangle
    matrix->fillRoundRect(
      3,   // X position
      0,   // Y position
      28,  // Width
      7,   // Height
      3,   // Corner radius (pixels)
      matrix->Color(92, 186, 76) // Blue
    );

    // Filled rounded rectangle
    matrix->fillRoundRect(
      3,   // X position
      8,   // Y position
      28,  // Width
      7,   // Height
      3,   // Corner radius (pixels)
      matrix->Color(243, 123, 46) // Blue
    );

    matrix->setTextColor(matrix->Color(0, 0, 0));

    matrix->setCursor(6, 6);  // X=0, Y=6 baseline
    matrix->print("2 TEAM");

    matrix->setCursor(6, 14);  // X=0, Y=6 baseline
    matrix->print("3 TEAM");
    matrix->show();

    firstCall = false;
    inStartPage = false;
  }
  else{
    Serial.println("team select menue not first call");
    if((buttonNo==5)||(buttonNo==7)){
      if(selectState==0) selectState = 1;
      else selectState = 0;
    }
    if(selectState == 0){
      // Filled rounded rectangle
      matrix->fillRoundRect(
        3,   // X position
        0,   // Y position
        28,  // Width
        7,   // Height
        3,   // Corner radius (pixels)
        matrix->Color(92, 186, 76) // Blue
      );

      // Filled rounded rectangle
      matrix->fillRoundRect(
        3,   // X position
        8,   // Y position
        28,  // Width
        7,   // Height
        3,   // Corner radius (pixels)
        matrix->Color(243, 123, 46) // Blue
      );

      matrix->setTextColor(matrix->Color(0, 0, 0));

      matrix->setCursor(6, 6);  // X=0, Y=6 baseline
      matrix->print("2 TEAM");

      matrix->setCursor(6, 14);  // X=0, Y=6 baseline
      matrix->print("3 TEAM");
      matrix->show();
    }

    else{
      // Filled rounded rectangle
      matrix->fillRoundRect(
        3,   // X position
        0,   // Y position
        28,  // Width
        7,   // Height
        3,   // Corner radius (pixels)
        matrix->Color(243, 123, 46) // Blue
      );

      // Filled rounded rectangle
      matrix->fillRoundRect(
        3,   // X position
        8,   // Y position
        28,  // Width
        7,   // Height
        3,   // Corner radius (pixels)
        
        matrix->Color(92, 186, 76) // Blue
      );

      matrix->setTextColor(matrix->Color(0, 0, 0));

      matrix->setCursor(6, 6);  // X=0, Y=6 baseline
      matrix->print("2 TEAM");

      matrix->setCursor(6, 14);  // X=0, Y=6 baseline
      matrix->print("3 TEAM");
      matrix->show();
    }

    if(buttonNo == 0){
      // UIstate = 1;
      // selectionState = 0;
      firstCall = true;
      if(selectState == 0){
        UIstate = 1;
        Serial.println("Calling 2PSM");
        twoPlayerSelectMenue(-1,isItLongPress);
      }
      else {
        UIstate = 2;
        Serial.println("Calling 3PSM");
        threePlayerSelectMenue(-1,isItLongPress);
      }

    }
  }
  Serial.println(" ");
  Serial.println("at the end of team select");
  delay(100);
}

void menueSelectionLadder(uint8_t buttonNo,bool isItLongPress){
  Serial.println("Here at menue ladder");
  if(UIstate == 0) teamSelectMenue(buttonNo, isItLongPress);
  else if(UIstate == 1) twoPlayerSelectMenue(buttonNo, isItLongPress);
  else if(UIstate == 2) threePlayerSelectMenue(buttonNo, isItLongPress);
  // else if(UIstate == 3) gameModeA(buttonNo);
  lastActionTime = currentTime;
  delay(50);
}


// === Gamma Correction ===
uint8_t gammaCorrect(uint8_t value) {
  return pow(value / 255.0, GAMMA_VAL) * 255.0 + 0.5;
}


// === General-purpose Image Drawing ===
void drawImage(const uint32_t* imageData, uint8_t imgWidth, uint8_t imgHeight, uint8_t startX, uint8_t startY) {
  uint16_t i = 0; // index in image data array
  
  for (uint8_t iy = 0; iy < imgHeight; iy++) {
    for (uint8_t ix = 0; ix < imgWidth; ix++) {
      
      uint8_t destX = startX + ix;
      uint8_t destY = startY + iy;

      // Skip if outside matrix boundaries
      if (destX >= WIDTH || destY >= HEIGHT) continue;

      // Read color from PROGMEM
      uint32_t color = pgm_read_dword(&(imageData[i++]));

      uint8_t r = (color >> 16) & 0xFF;
      uint8_t g = (color >> 8)  & 0xFF;
      uint8_t b =  color        & 0xFF;

      leds[XY(destX, destY)] = CRGB(
        gammaCorrect(r),
        gammaCorrect(g),
        gammaCorrect(b)
      );
    }
  }
  // delay(50);
}


// === Serpentine mapping ===
uint16_t XY(uint8_t x, uint8_t y) {
  if (y % 2 == 0) {
    return (y * WIDTH) + x;
  } else {
    return (y * WIDTH) + (WIDTH - 1 - x);
  }
}


// === Play Animation from multiple PROGMEM frames ===
void playAnimation(const uint32_t* frames[], uint8_t frameCount,
                   uint8_t imgWidth, uint8_t imgHeight,
                   uint8_t startX, uint8_t startY,
                   uint16_t frameDelayMs) {
  for (uint8_t f = 0; f < frameCount; f++) {
    // FastLED.clear();
    drawImage(frames[f], imgWidth, imgHeight, startX, startY);
    FastLED.show();
    delay(frameDelayMs);
  }
}


void sleepMode(){
  Serial.println("Entering light sleep until button press...");
  FastLED.clear();
  FastLED.show();
  PlayerCount = 0;
  gameIsOn = false;
  UIstate = 0;
  selectState = 0;
  digitalWrite(LASER_PIN, LOW);


  delay(50);
  esp_light_sleep_start();
  delay(500);
  digitalWrite(LASER_PIN, HIGH);
  Serial.println("Exited the sleep mode...");
  menueSelectionLadder(-1, 0);
}




void twoPlayerSelectMenue(uint8_t buttonNo, bool isItLongPress) {
  static bool firstCall = true;
  Serial.println("Here at 2team menue");

  int startX[] = {2, 18}; // X positions for columns (2 columns now)
  int startY[] = {1, 9};  // Y positions for rows (still 2 rows)
  int cols = 2;           // number of columns
  int totalItems = 4;     // total menu items (0 to 3)
  
  FastLED.clear();
  matrix->fillRect(
    0,   // X position
    0,   // Y position
    34,  // Width
    16,   // Height
    matrix->Color(16, 122, 176) // Color (R,G,B)
  );

  if (firstCall) {
    Serial.println("2team menue first call");
    selectState = 0;

    for (int y = 0; y < 2; y++) {       // rows
      for (int x = 0; x < cols; x++) {  // columns
        int index = y * cols + x;       // 0 to 3
        int color = (index == selectState) ? 
                    matrix->Color(92, 186, 76) : // green
                    matrix->Color(243, 123, 46); // orange

        matrix->fillRoundRect(
          startX[x],
          startY[y],
          14,  // Width
          7,   // Height
          3,   // Corner radius
          color
        );
      }
    }
    // matrix->show();
    firstCall = false;
  }
  else {
    if(!isItLongPress){
      // Navigation logic
      if (buttonNo == 6) selectState += 1; 
      else if (buttonNo == 8) selectState -= 1;
      else if (buttonNo == 5){
        if(selectState<cols) selectState += 3; // move down
        else selectState -= 2; 
      }
      else if (buttonNo == 7){
        if(selectState<cols) selectState += 2; // move up
        else selectState -=1;
      }

      // Wrap-around for 0 to totalItems-1
      if (selectState >= totalItems) selectState = 0;
      else if (selectState < 0) selectState = totalItems - 1;

      // Draw all items again
      for (int y = 0; y < 2; y++) {       // rows
        for (int x = 0; x < cols; x++) {  // columns
          int index = y * cols + x;
          int color = (index == selectState) ? 
                      matrix->Color(92, 186, 76) : 
                      matrix->Color(243, 123, 46);

          matrix->fillRoundRect(
            startX[x],
            startY[y],
            14,  // Width
            7,   // Height
            3,   // Corner radius
            color
          );
        }
      }
    }
  }
  matrix->setTextColor(matrix->Color(0, 0, 0));
  matrix->setCursor(5, 7);  // X=0, Y=6 baseline
  matrix->print("2P");
  matrix->setCursor(21, 7);  // X=0, Y=6 baseline
  matrix->print("4P");
  matrix->setCursor(5, 15);  // X=0, Y=6 baseline
  matrix->print("6P");
  matrix->setCursor(21, 15);  // X=0, Y=6 baseline
  matrix->print("8P");
  matrix->show();


  if(isItLongPress){
    FastLED.clear();
    PlayerCount = 0;
    selectState = 0;
    UIstate = 0;
    firstCall = true;
    gameIsOn = false;
    menueSelectionLadder(-1, 0);
  }
  else if(buttonNo == 0){
    PlayerCount = 2 + (selectState*2);
    selectState = 0;
    UIstate = 3;
    firstCall = true;
    gameIsOn = true;
    gameModeA(-1 , 0);
  }
  delay(50);
}


void threePlayerSelectMenue(uint8_t buttonNo,bool isItLongPress){
  static bool firstCall = true;
  Serial.println("Here at 3team menue");

  int startX[] = {2, 18}; // X positions for columns (2 columns now)
  int startY[] = {1, 9};  // Y positions for rows (still 2 rows)
  int cols = 2;           // number of columns
  int totalItems = 4;     // total menu items (0 to 3)
  
  FastLED.clear();
  matrix->fillRect(
    0,   // X position
    0,   // Y position
    34,  // Width
    16,   // Height
    matrix->Color(16, 122, 176) // Color (R,G,B)
  );

  if (firstCall) {
    selectState = 0;

    for (int y = 0; y < 2; y++) {       // rows
      for (int x = 0; x < cols; x++) {  // columns
        int index = y * cols + x;       // 0 to 3
        int color = (index == selectState) ? 
                    matrix->Color(92, 186, 76) : // green
                    matrix->Color(243, 123, 46); // orange

        matrix->fillRoundRect(
          startX[x],
          startY[y],
          14,  // Width
          7,   // Height
          3,   // Corner radius
          color
        );
      }
    }
    // matrix->show();
    firstCall = false;
  }
  else {
    if(!isItLongPress){
      // Navigation logic
      if (buttonNo == 6) selectState += 1; 
      else if (buttonNo == 8) selectState -= 1;
      else if (buttonNo == 5){
        if(selectState<cols) selectState += 3; // move down
        else selectState -= 2; 
      }
      else if (buttonNo == 7){
        if(selectState<cols) selectState += 2; // move up
        else selectState -=1;
      }

      // Wrap-around for 0 to totalItems-1
      if (selectState >= totalItems) selectState = 0;
      else if (selectState < 0) selectState = totalItems - 1;

      // Draw all items again
      for (int y = 0; y < 2; y++) {       // rows
        for (int x = 0; x < cols; x++) {  // columns
          int index = y * cols + x;
          int color = (index == selectState) ? 
                      matrix->Color(92, 186, 76) : 
                      matrix->Color(243, 123, 46);

          matrix->fillRoundRect(
            startX[x],
            startY[y],
            14,  // Width
            7,   // Height
            3,   // Corner radius
            color
          );
        }
      }
    }
  }
  matrix->setTextColor(matrix->Color(0, 0, 0));
  matrix->setCursor(6, 7);  // X=0, Y=6 baseline
  matrix->print("3P");
  matrix->setCursor(22, 7);  // X=0, Y=6 baseline
  matrix->print("6P");
  matrix->setCursor(6, 15);  // X=0, Y=6 baseline
  matrix->print("9P");
  matrix->setCursor(22, 15);  // X=0, Y=6 baseline
  matrix->print("1P");
  matrix->show();

  delay(50);

  if(isItLongPress){
    FastLED.clear();
    PlayerCount = 0;
    selectState = 0;
    UIstate = 0;
    firstCall = true;
    gameIsOn = false;
    menueSelectionLadder(-1, 0);
    return;
  }
  else if(buttonNo == 0){
    PlayerCount = 3 + (selectState*3);
    selectState = 0;
    UIstate = 4;
    firstCall = true;
    gameIsOn = true;
    gameModeB(-1 , 0);
  }
}



// Function 1: Read all sensor values
void readSensors(int readings[]) {
  for (int i = 0; i < NUM_SENSORS; i++) {
    readings[i] = analogRead(LDR_PINS[i]);
  }
}


// Function 2: Detect which ball (if any) based on passed readings
int detectBall(int readings[]) {
  unsigned long now = millis();

  for (int i = 0; i < NUM_SENSORS; i++) {
    bool isLaserCut = (readings[i] < threshold);

    if (isLaserCut && !laserBlocked[i] && (now - lastDetectionTime[i] >= debounceTime)) {
      laserBlocked[i] = true;
      lastDetectionTime[i] = now;
      return i; // Return which sensor was triggered
    }

    if (!isLaserCut) {
      laserBlocked[i] = false;
    }
  }

  return -1; // No detection
}


void gameModeA(int detected, bool isItLongPress){
  FastLED.clear();
  static uint8_t teamAScore = 0;
  static uint8_t teamBScore = 0;
  Serial.println("here i am at the gameModeA");
  Serial.println(detected);
  Serial.println(PlayerCount);

  if((detected ==-1)&&!isItLongPress){
    for(uint8_t i = 0; i < 12 ; i++) ChancesLeft[i] = ChancesPerPlayer;
    teamAScore = 0;
    teamBScore = 0;
    drawScoreboardA(teamAScore, teamBScore);
  }
  else if(isItLongPress){
    UIstate = 0;
    selectState = 0;
    gameIsOn = false;
    menueSelectionLadder(-1, 0);
    return;
  }
  else{
    if(detected % 2 == 0) teamBScore += 1;
    else teamAScore += 1;
    for (uint8_t i = 0 ; i < PlayerCount ; i++ ){
      if(ChancesLeft[i] != 0){
        ChancesLeft[i] = ChancesLeft[i]-1;
        break;
      }
    }
    
    if(detected<3){
      if (detected % 2 == 0) ScoredAnimationA(teamAScore , teamBScore , 3+(5*detected), 1, 237, 28, 36);
      else ScoredAnimationA(teamAScore , teamBScore , 3+(5*detected), 1, 63, 72, 204);
    }
    else{
      if (detected % 2 == 0) ScoredAnimationA(teamAScore , teamBScore , 5+(5*detected), 1, 237, 28, 36);
      else ScoredAnimationA(teamAScore , teamBScore , 5+(5*detected), 1, 63, 72, 204);
    }
    drawScoreboardA(teamAScore, teamBScore);
    // matrix->show();
    if(ChancesLeft[PlayerCount-1]==0){
      if(teamAScore>teamBScore){
        matrix->fillRect(0,0,34,16,matrix->Color(63, 72, 204));
        matrix->setTextColor(matrix->Color(0, 0, 0));
        matrix->setCursor(4, 8);
        matrix->print("Winner!!");
        matrix->setCursor(15, 15);
        matrix->print(teamAScore);
      }
      else if(teamAScore<teamBScore){
        matrix->fillRect(0,0,34,16,matrix->Color(237, 28, 36));
        matrix->setTextColor(matrix->Color(0, 0, 0));
        matrix->setCursor(4, 8);
        matrix->print("Winner!!");
        matrix->setCursor(15, 15);
        matrix->print(teamBScore);
      }
      else{
        matrix->fillRect(0,0,34,16,matrix->Color(45, 210, 75));
        matrix->setTextColor(matrix->Color(0, 0, 0));
        matrix->setCursor(9, 11);
        matrix->print("TIE !!");
      }
      UIstate = 0;
      gameIsOn = false;
      selectState = 0;
      FastLED.show();
      delay(7000);
      menueSelectionLadder(-1,0);
    }
  }

  FastLED.show();
  delay(200);
}


void gameModeB(int detected, bool isItLongPress){
  FastLED.clear();
  static uint8_t teamAScore = 0;
  static uint8_t teamBScore = 0;
  static uint8_t teamCScore = 0;
  // Serial.println("here i am at the gameModeA");
  // Serial.println(detected);
  // Serial.println(PlayerCount);

  if((detected ==-1)&&!isItLongPress){
    for(uint8_t i = 0; i < 12 ; i++) ChancesLeft[i] = ChancesPerPlayer;
    teamAScore = 0;
    teamBScore = 0;
    teamCScore = 0;
    drawScoreboardB(teamAScore, teamBScore, teamCScore);
  }
  else if(isItLongPress){
    UIstate = 0;
    selectState = 0;
    gameIsOn = false;
    menueSelectionLadder(-1, 0);
    return;
  }
  else{
    if(detected == 0 || detected == 3) teamBScore += 1;
    else if(detected == 1 || detected == 4) teamAScore += 1;
    else teamCScore += 1;
    for (uint8_t i = 0 ; i < PlayerCount ; i++ ){
      if(ChancesLeft[i] != 0){
        ChancesLeft[i] = ChancesLeft[i]-1;
        break;
      }
    }
    

    if (detected == 0) ScoredAnimationB(teamAScore , teamBScore , teamCScore , 3+(5*detected), 1, 237, 28, 36);
    else if (detected == 1) ScoredAnimationB(teamAScore , teamBScore , teamCScore , 3+(5*detected), 1, 63, 72, 204);
    else if (detected == 2) ScoredAnimationB(teamAScore , teamBScore , teamCScore , 3+(5*detected), 1, 255, 242, 50);
    else if (detected == 3) ScoredAnimationB(teamAScore , teamBScore , teamCScore , 3+(5*detected), 1, 237, 28, 36);
    else if (detected == 4) ScoredAnimationB(teamAScore , teamBScore , teamCScore , 3+(5*detected), 1, 63, 72, 204);
    else ScoredAnimationB (teamAScore , teamBScore , teamCScore , 3+(5*detected), 1, 255, 242, 50);
    drawScoreboardB(teamAScore, teamBScore, teamCScore);
    // matrix->show();

    if(ChancesLeft[PlayerCount-1]==0){
      if((teamAScore>teamBScore)&&(teamAScore>teamCScore)){
        matrix->fillRect(0,0,34,16,matrix->Color(63, 72, 204));   // was red, now blue
        matrix->setTextColor(matrix->Color(0, 0, 0));
        matrix->setCursor(4, 8);
        matrix->print("Winner!!");
        matrix->setCursor(15, 15);
        matrix->print(teamAScore);
      }
      else if((teamAScore<teamBScore)&&(teamCScore<teamBScore)){
        matrix->fillRect(0,0,34,16,matrix->Color(237, 28, 36));   // was blue, now red
        matrix->setTextColor(matrix->Color(0, 0, 0));
        matrix->setCursor(4, 8);
        matrix->print("Winner!!");
        matrix->setCursor(15, 15);
        matrix->print(teamBScore);
      }
      else if((teamAScore<teamCScore)&&(teamBScore<teamCScore)){
        matrix->fillRect(0,0,34,16,matrix->Color(255, 242, 50));
        matrix->setTextColor(matrix->Color(0, 0, 0));
        matrix->setCursor(4, 8);
        matrix->print("Winner!!");
        matrix->setCursor(15, 15);
        matrix->print(teamCScore);
      }
      else if((teamAScore==teamCScore)&&(teamBScore<teamCScore)){
        matrix->fillRect(0,0,17,16,matrix->Color(63, 72, 204));   // left was red → blue
        matrix->fillRect(17,0,17,16,matrix->Color(255, 242, 50));
        matrix->setTextColor(matrix->Color(0, 0, 0));
        matrix->setCursor(9, 11);
        matrix->print("TIE !!");
      }
      else if((teamAScore==teamBScore)&&(teamBScore>teamCScore)){
        matrix->fillRect(0,0,17,16,matrix->Color(63, 72, 204));   // left was red → blue
        matrix->fillRect(17,0,17,16,matrix->Color(237, 28, 36));  // right was blue → red
        matrix->setTextColor(matrix->Color(0, 0, 0));
        matrix->setCursor(9, 11);
        matrix->print("TIE !!");
      }
      else if((teamBScore==teamBScore)&&(teamBScore>teamAScore)){
        matrix->fillRect(0,0,17,16,matrix->Color(255, 242, 50));
        matrix->fillRect(17,0,17,16,matrix->Color(237, 28, 36));  // was blue → red
        matrix->setTextColor(matrix->Color(0, 0, 0));
        matrix->setCursor(9, 11);
        matrix->print("TIE !!");
      }
      else{
        matrix->fillRect(0,0,34,16,matrix->Color(45, 210, 75));
        matrix->setTextColor(matrix->Color(0, 0, 0));
        matrix->setCursor(9, 11);
        matrix->print("TIE !!");
      }
      UIstate = 0;
      gameIsOn = false;
      selectState = 0;
      FastLED.show();
      delay(7000);
      menueSelectionLadder(-1,0);
      return;
    }
  }

  FastLED.show();
  delay(200);
}




void fillOutsideCircle(int centerX, int centerY, int radius, uint16_t color) {

    int rSquared = radius * radius;

    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            int dx = x - centerX;
            int dy = y - centerY;
            int distSquared = dx * dx + dy * dy;

            if (distSquared > rSquared) {
              // Serial.println("here at fill circle");
              matrix->drawPixel(x, y, color);
            }
        }
    }
}


void ScoredAnimationA(uint8_t teamAScore, uint8_t teamBScore, uint8_t cx, uint8_t cy, uint8_t rCol, uint8_t gCol, uint8_t bCol) {
    uint16_t color = matrix->Color(rCol, gCol, bCol);

    // --- First expanding circle animation ---
    for (uint8_t r = 1; r <= 17; r++) {
        matrix->fillCircle(cx, cy, (r * 2), color);
        matrix->show();

        if (r < 4) delay(40);
        else if (r < 10) delay(22);
        else if (r < 15) delay(10);
        else delay(5);
    }

    delay(50);

    // --- Draw scoreboard ONCE and store it ---
    drawScoreboardA(teamAScore, teamBScore);

    // Save scoreboard into buffer
    memcpy(scoreboardBuffer, leds, sizeof(leds));

    // --- Expanding "outside circle" animation ---
    for (uint8_t r = 1; r <= 17; r++) {
        // Restore scoreboard from buffer
        memcpy(leds, scoreboardBuffer, sizeof(leds));

        // Draw animated part on top
        fillOutsideCircle(cx, cy, (r * 2), color);

        matrix->show();

        if (r < 4) delay(40);
        else if (r < 10) delay(22);
        else if (r < 15) delay(10);
        else delay(5);
    }
    delay(50);
}

void ScoredAnimationB(uint8_t teamAScore, uint8_t teamBScore, uint8_t teamCScore, uint8_t cx, uint8_t cy, uint8_t rCol, uint8_t gCol, uint8_t bCol) {
    uint16_t color = matrix->Color(rCol, gCol, bCol);

    // --- First expanding circle animation ---
    for (uint8_t r = 1; r <= 17; r++) {
        matrix->fillCircle(cx, cy, (r * 2), color);
        matrix->show();

        if (r < 4) delay(40);
        else if (r < 10) delay(22);
        else if (r < 15) delay(10);
        else delay(5);
    }

    delay(50);

    // --- Draw scoreboard ONCE and store it ---
    drawScoreboardB(teamAScore, teamBScore, teamCScore);

    // Save scoreboard into buffer
    memcpy(scoreboardBuffer, leds, sizeof(leds));

    // --- Expanding "outside circle" animation ---
    for (uint8_t r = 1; r <= 17; r++) {
        // Restore scoreboard from buffer
        memcpy(leds, scoreboardBuffer, sizeof(leds));

        // Draw animated part on top
        fillOutsideCircle(cx, cy, (r * 2), color);

        matrix->show();

        if (r < 4) delay(40);
        else if (r < 10) delay(22);
        else if (r < 15) delay(10);
        else delay(5);
    }
    delay(50);
}




void drawScoreboardA(uint8_t teamAScore, uint8_t teamBScore) {
  // Draw the base scoreboard image
  drawImage(GameModeAScoreboard, 34, 16, 0, 0);
  delay(50);

  // Team A score (blue)
  if(teamAScore < 10){
    matrix->setTextColor(matrix->Color(63, 72, 204));
    matrix->setCursor(10, 11);
    matrix->print(teamAScore);
  }
  else{
    matrix->setTextColor(matrix->Color(63, 72, 204));
    matrix->setCursor(8, 11);
    matrix->print(teamAScore);
  }

  // Team B score (red)
  if(teamBScore < 10){
    matrix->setTextColor(matrix->Color(237, 28, 36));
    matrix->setCursor(21, 11);
    matrix->print(teamBScore);
  }
  else{
    matrix->setTextColor(matrix->Color(237, 28, 36));
    matrix->setCursor(19, 11);
    matrix->print(teamBScore);
  }

  // Small colored squares (decorations)
  matrix->fillRect(3, 1, 2, 2, matrix->Color(237, 28, 36)); // red
  matrix->fillRect(8, 1, 2, 2, matrix->Color(63, 72, 204)); // blue
  matrix->fillRect(13, 1, 2, 2, matrix->Color(237, 28, 36)); // red
  matrix->fillRect(19, 1, 2, 2, matrix->Color(63, 72, 204)); // blue
  matrix->fillRect(24, 1, 2, 2, matrix->Color(237, 28, 36)); // red
  matrix->fillRect(29, 1, 2, 2, matrix->Color(63, 72, 204)); // blue


    //Drawing the chances left bars
    printChances(ChancesLeft, 12);
    for (uint8_t i = 1 ; i <= PlayerCount ; i++ ){
      if(ChancesLeft[i-1] != 0){
        if(i%2 != 0){
          matrix->drawLine(0, (3 + i), ChancesLeft[i-1]-1, (3 + i), matrix->Color(63, 72, 204));
          // matrix->drawLine(0, HEIGHT-1, WIDTH, HEIGHT-1, matrix->Color(63, 72, 204));
        }
        else{
          matrix->drawLine(34-ChancesLeft[i-1], (2 + i), 33, (2 + i), matrix->Color(237, 28, 36));
          // matrix->drawLine(0, HEIGHT-1, WIDTH, HEIGHT-1, matrix->Color(237, 28, 36));
        }
      }
    }
    for (uint8_t i = 1 ; i <= PlayerCount ; i++ ){
      if(ChancesLeft[i-1] != 0){
        if(i%2 != 0){
          // matrix->drawLine(0, (3 + i), ChancesLeft[i-1]-1, (3 + i), matrix->Color(63, 72, 204));
          matrix->drawLine(0, HEIGHT-1, WIDTH, HEIGHT-1, matrix->Color(63, 72, 204));
          break;
        }
        else{
          // matrix->drawLine(34-ChancesLeft[i-1], (2 + i), 33, (2 + i), matrix->Color(237, 28, 36));
          matrix->drawLine(0, HEIGHT-1, WIDTH, HEIGHT-1, matrix->Color(237, 28, 36));
          break;
        }
      }
    }
}

void drawScoreboardB(uint8_t teamAScore, uint8_t teamBScore, uint8_t teamCScore){
  drawImage(GameModeBScoreboard, 34, 16, 0, 0);
  delay(50);
  uint8_t baseX = (WIDTH-(3*PlayerCount))/2;

  if(teamAScore < 10){
    // Team A score (blue)
    matrix->setTextColor(matrix->Color(63, 72, 204));
    matrix->setCursor(7, 8);
    matrix->print(teamAScore);
  }
  else{
    // Team A score (blue)
    matrix->setTextColor(matrix->Color(63, 72, 204));
    matrix->setCursor(5, 8);
    matrix->print(teamAScore);
  }


  if(teamBScore<10){
    // Team B score (red)
    matrix->setTextColor(matrix->Color(237, 28, 36));
    matrix->setCursor(16, 8);
    matrix->print(teamBScore);
  }
  else{
    // Team B score (red)
    matrix->setTextColor(matrix->Color(237, 28, 36));
    matrix->setCursor(14, 8);
    matrix->print(teamBScore);
  }


  if(teamCScore<10){
    // Team C score (yellow)
    matrix->setTextColor(matrix->Color(255,242, 50));
    matrix->setCursor(25, 8);
    matrix->print(teamCScore);
  }
  else{
    // Team C score (yellow)
    matrix->setTextColor(matrix->Color(255,242, 50));
    matrix->setCursor(23, 8);
    matrix->print(teamCScore);
  }


  // Small colored squares (decorations)
  matrix->fillRect(3, 0, 2, 1, matrix->Color(237, 28, 36)); // red
  matrix->fillRect(10, 0, 2, 1, matrix->Color(63, 72, 204)); // blue
  matrix->fillRect(14, 0, 2, 1, matrix->Color(255, 242, 50)); // red
  matrix->fillRect(18, 0, 2, 1, matrix->Color(237, 28, 36)); // blue
  matrix->fillRect(22, 0, 2, 1, matrix->Color(63, 72, 204)); // red
  matrix->fillRect(29, 0, 2, 1, matrix->Color(255, 242, 50)); // blue


  for (uint8_t i = 1 ; i <= PlayerCount ; i++ ){
    if(ChancesLeft[i-1] != 0){
      if(i%3==0){
        matrix->drawLine(baseX+1 + (3*(i-1)), HEIGHT-ChancesLeft[i-1], baseX+1 + (3*(i-1)), HEIGHT+1, matrix->Color(63, 72, 204));
        matrix->drawLine(baseX+1 + 1 + (3*(i-1)), HEIGHT-ChancesLeft[i-1], baseX+1 + 1 + (3*(i-1)), HEIGHT+1, matrix->Color(63, 72, 204));
      }
      else if(i%3==1){
        matrix->drawLine(baseX+1 + (3*(i-1)), HEIGHT-ChancesLeft[i-1], baseX+1 + (3*(i-1)), HEIGHT+1, matrix->Color(237, 28, 36));
        matrix->drawLine(baseX+1 + 1 + (3*(i-1)), HEIGHT-ChancesLeft[i-1], baseX+1 + 1 + (3*(i-1)), HEIGHT+1, matrix->Color(237, 28, 36));
      }
      else{
        matrix->drawLine(baseX+1 + (3*(i-1)), HEIGHT-ChancesLeft[i-1], baseX+1 + (3*(i-1)), HEIGHT+1, matrix->Color(255, 242, 50));
        matrix->drawLine(baseX+1 + 1 + (3*(i-1)), HEIGHT-ChancesLeft[i-1], baseX+1 + 1 + (3*(i-1)), HEIGHT+1, matrix->Color(255, 242, 50));
      }
    }
  }
  for (uint8_t i = 1 ; i <= PlayerCount ; i++ ){
    if(ChancesLeft[i-1] != 0){
      if(i%3==0){
        matrix->drawLine(0, 0, 0, HEIGHT-1, matrix->Color(63, 72, 204));
        matrix->drawLine(WIDTH-1, 0, WIDTH-1, HEIGHT-1, matrix->Color(63, 72, 204));
        break;
      }
      else if(i%3==1){
        matrix->drawLine(0, 0, 0, HEIGHT-1, matrix->Color(237, 28, 36));
        matrix->drawLine(WIDTH-1, 0, WIDTH-1, HEIGHT-1, matrix->Color(237, 28, 36));
        break;
      }
      else{
        matrix->drawLine(0, 0, 0, HEIGHT-1, matrix->Color(255, 242, 50));
        matrix->drawLine(WIDTH-1, 0, WIDTH-1, HEIGHT-1, matrix->Color(255, 242, 50));
        break;
      }
    }
  }
}



void printChances(uint8_t arr[], uint8_t size) {
    for (uint8_t i = 0; i < size; i++) {
        Serial.print(arr[i]);
        if (i < size - 1) { // avoid trailing '-'
            Serial.print("-");
        }
    }
    Serial.println(); // move to new line at the end
}


