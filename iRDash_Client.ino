#include <UTFT.h>
#include <UTouch.h>
#include <UTFT_Buttons.h>

#define ScreenWidth 319 // width of the display in pixels -1

// define car identification numbers
#define NumOfCars 4     // number of car profiles, maximum is 16
#define DefaultCar 0    // car profile to show at startup
#define ID_Skippy 0
#define ID_CTS_V  1
#define ID_MX5_NC 2
#define ID_MX5_ND 3

// define the drawing colors
#define dc_r 100  // default color
#define dc_g 255
#define dc_b 100
#define mc_r 255  // middle color
#define mc_g 240
#define mc_b 0
#define wc_r 255  // warning color
#define wc_g 100
#define wc_b 100

// declare which fonts we will be using
extern uint8_t SmallFont[];        // 8x12 pixel
extern uint8_t BigFont[];          // 16x16 pixel
extern uint8_t SevenSegNumFont[];  // 32x50 pixel

// declare the bitmaps
extern unsigned int fuelpressure_nok[0x400];
extern unsigned int fuelpressure_ok[0x400];
extern unsigned int oilpressure_nok[0x400];
extern unsigned int oilpressure_ok[0x400];
extern unsigned int pitspeedlimiter_off[0x400];
extern unsigned int pitspeedlimiter_on[0x400];
extern unsigned int stall_off[0x400];
extern unsigned int stall_on[0x400];
extern unsigned int water_nok[0x400];
extern unsigned int water_ok[0x400];

// Set the pins to the correct ones for your development shield
// ------------------------------------------------------------
// Arduino Uno / 2009:
// -------------------
// Standard Arduino Uno/2009 shield            : <display model>,A5,A4,A3,A2
// DisplayModule Arduino Uno TFT shield        : <display model>,A5,A4,A3,A2
//
// Arduino Mega:
// -------------------
// Standard Arduino Mega/Due shield            : <display model>,38,39,40,41
// CTE TFT LCD/SD Shield for Arduino Mega      : <display model>,38,39,40,41
//
// Remember to change the model parameter to suit your display module!
UTFT myGLCD(SSD1289, 38, 39, 40, 41);

// Initialize touchscreen
// ----------------------
// Set the pins to the correct ones for your development board
// -----------------------------------------------------------
// Standard Arduino Uno/2009 Shield            : 15,10,14, 9, 8
// Standard Arduino Mega/Due shield            :  6, 5, 4, 3, 2
// CTE TFT LCD/SD Shield for Arduino Due       :  6, 5, 4, 3, 2
// Teensy 3.x TFT Test Board                   : 26,31,27,28,29
// ElecHouse TFT LCD/SD Shield for Arduino Due : 25,26,27,29,30
//
UTouch  myTouch(6, 5, 4, 3, 2);

// Finally we set up UTFT_Buttons :)
UTFT_Buttons  myButtons(&myGLCD, &myTouch);

// bit fields of engine warnings for reference
/*enum irsdk_EngineWarnings 
{
  irsdk_waterTempWarning    = 0x01,
  irsdk_fuelPressureWarning = 0x02,
  irsdk_oilPressureWarning  = 0x04,
  irsdk_engineStalled     = 0x08,
  irsdk_pitSpeedLimiter   = 0x10,
  irsdk_revLimiterActive    = 0x20,
}*/

// structure of the incoming serial data block
struct SIncomingData
{
  byte EngineWarnings;
  float Fuel;
  char Gear;
  bool IsOnTrack;
  float RPM;
  float Speed;
  float WaterTemp;
};

// structure to store the actual and the previous screen data
struct SViewData
{
  byte EngineWarnings;
  int Fuel;
  char Gear;
  int RPMgauge;
  int Speed;
  int SLI;
  int WaterTemp;
};

// structure to store the screen layout of the gauges
struct SScreenLayout
{
  int EngineWarningsPosX;
  int EngineWarningsPosY;
  bool ShowEngineWarnings;
  int FuelPosX;
  int FuelPosY;
  bool ShowFuel;
  int GearPosX;
  int GearPosY;
  bool ShowGear;
  int RPMPosY;
  float RPMscale;
  bool ShowRPM;
  int SLIPosY;
  bool ShowSLI;
  int SpeedPosX;
  int SpeedPosY;
  bool ShowSpeed;
  int WaterTempPosX;
  int WaterTempPosY;
  bool ShowWaterTemp;
  char CarName[10];
};

// limits where different drawing color have to be used
struct SWarnings
{
  int Fuel;           // value in liter * 10
  int RPM;            // value where the redline starts in pixels
  int WaterTemp;      // value in Celsius
  int SLI[10][4];     // RPM values for each SLI light, indexes are 0:reverse, 1:neutral, 2:1st, 3:2nd, ...
};

byte inByte;                  // incoming byte from the serial port
int blockpos;                 // position of the actual incoming byte in the telemetry data block
SIncomingData* InData;        // pointer to access the telemetry data as a structure
byte* pInData;                // pointer to access the telemetry data as a byte array
SViewData Screen[2];          // store the actual and the previous screen data
int buttons[NumOfCars];       // handle for the touch buttons
byte ActiveCar;               // active car profile

// variables to manage screen layout
SScreenLayout ViewLayout[NumOfCars];  // store screen layout for each car
SWarnings ViewWarning[NumOfCars];     // store warning limits for each car

// clear our internal data block
void ResetInternalData()
{
  Screen[0].EngineWarnings = 0;
  Screen[0].Fuel = 0;
  Screen[0].Gear = -1;
  Screen[0].RPMgauge = 0;
  Screen[0].Speed = -1;
  Screen[0].SLI = 0;
  Screen[0].WaterTemp = 0;
  blockpos = 0;
}

// upload the screen layout and warning limits per car
void UploadCarProfiles()
{
  // Skippy
  ViewLayout[ID_Skippy].CarName[0] = 'S';
  ViewLayout[ID_Skippy].CarName[1] = 'k';
  ViewLayout[ID_Skippy].CarName[2] = 'i';
  ViewLayout[ID_Skippy].CarName[3] = 'p';
  ViewLayout[ID_Skippy].CarName[4] = 'p';
  ViewLayout[ID_Skippy].CarName[5] = 'y';
  ViewLayout[ID_Skippy].CarName[6] = 0;
  ViewLayout[ID_Skippy].EngineWarningsPosX = 0;
  ViewLayout[ID_Skippy].EngineWarningsPosY = 207;
  ViewLayout[ID_Skippy].ShowEngineWarnings = true;
  ViewLayout[ID_Skippy].FuelPosX = 0;
  ViewLayout[ID_Skippy].FuelPosY = 150;
  ViewLayout[ID_Skippy].ShowFuel = true;
  ViewLayout[ID_Skippy].GearPosX = 280;
  ViewLayout[ID_Skippy].GearPosY = 65;
  ViewLayout[ID_Skippy].ShowGear = true;
  ViewLayout[ID_Skippy].RPMPosY = 25;
  ViewLayout[ID_Skippy].RPMscale = 20.625;  // 6600 / 320
  ViewLayout[ID_Skippy].ShowRPM = true;
  ViewLayout[ID_Skippy].SLIPosY = 0;
  ViewLayout[ID_Skippy].ShowSLI = true;
  ViewLayout[ID_Skippy].SpeedPosX = 0;
  ViewLayout[ID_Skippy].SpeedPosY = 75;
  ViewLayout[ID_Skippy].ShowSpeed = true;
  ViewLayout[ID_Skippy].WaterTempPosX = 0;
  ViewLayout[ID_Skippy].WaterTempPosY = 182;
  ViewLayout[ID_Skippy].ShowWaterTemp = true;

  ViewWarning[ID_Skippy].Fuel = 25;
  ViewWarning[ID_Skippy].RPM = 291;         // 6000 / RPMscale
  ViewWarning[ID_Skippy].WaterTemp = 90;

  ViewWarning[ID_Skippy].SLI[0][0] = 4900;  // car has 5 forward gears
  ViewWarning[ID_Skippy].SLI[0][1] = 5400;
  ViewWarning[ID_Skippy].SLI[0][2] = 5800;
  ViewWarning[ID_Skippy].SLI[0][3] = 6100;
  ViewWarning[ID_Skippy].SLI[1][0] = 4900;
  ViewWarning[ID_Skippy].SLI[1][1] = 5400;
  ViewWarning[ID_Skippy].SLI[1][2] = 5800;
  ViewWarning[ID_Skippy].SLI[1][3] = 6100;
  ViewWarning[ID_Skippy].SLI[2][0] = 4900;
  ViewWarning[ID_Skippy].SLI[2][1] = 5400;
  ViewWarning[ID_Skippy].SLI[2][2] = 5800;
  ViewWarning[ID_Skippy].SLI[2][3] = 6100;
  ViewWarning[ID_Skippy].SLI[3][0] = 4900;
  ViewWarning[ID_Skippy].SLI[3][1] = 5400;
  ViewWarning[ID_Skippy].SLI[3][2] = 5800;
  ViewWarning[ID_Skippy].SLI[3][3] = 6100;
  ViewWarning[ID_Skippy].SLI[4][0] = 4900;
  ViewWarning[ID_Skippy].SLI[4][1] = 5400;
  ViewWarning[ID_Skippy].SLI[4][2] = 5800;
  ViewWarning[ID_Skippy].SLI[4][3] = 6100;
  ViewWarning[ID_Skippy].SLI[5][0] = 4900;
  ViewWarning[ID_Skippy].SLI[5][1] = 5400;
  ViewWarning[ID_Skippy].SLI[5][2] = 5800;
  ViewWarning[ID_Skippy].SLI[5][3] = 6100;
  ViewWarning[ID_Skippy].SLI[6][0] = 4900;
  ViewWarning[ID_Skippy].SLI[6][1] = 5400;
  ViewWarning[ID_Skippy].SLI[6][2] = 5800;
  ViewWarning[ID_Skippy].SLI[6][3] = 6100;

  // CTS-V
  ViewLayout[ID_CTS_V].CarName[0] = 'C';
  ViewLayout[ID_CTS_V].CarName[1] = 'T';
  ViewLayout[ID_CTS_V].CarName[2] = 'S';
  ViewLayout[ID_CTS_V].CarName[3] = '-';
  ViewLayout[ID_CTS_V].CarName[4] = 'V';
  ViewLayout[ID_CTS_V].CarName[5] = 0;
  ViewLayout[ID_CTS_V].CarName[6] = 0;
  ViewLayout[ID_CTS_V].EngineWarningsPosX = 0;
  ViewLayout[ID_CTS_V].EngineWarningsPosY = 207;
  ViewLayout[ID_CTS_V].ShowEngineWarnings = true;
  ViewLayout[ID_CTS_V].FuelPosX = 0;
  ViewLayout[ID_CTS_V].FuelPosY = 150;
  ViewLayout[ID_CTS_V].ShowFuel = true;
  ViewLayout[ID_CTS_V].GearPosX = 280;
  ViewLayout[ID_CTS_V].GearPosY = 65;
  ViewLayout[ID_CTS_V].ShowGear = true;
  ViewLayout[ID_CTS_V].RPMPosY = 25;
  ViewLayout[ID_CTS_V].RPMscale = 25;  // 8000 / 320
  ViewLayout[ID_CTS_V].ShowRPM = true;
  ViewLayout[ID_CTS_V].SLIPosY = 0;
  ViewLayout[ID_CTS_V].ShowSLI = true;
  ViewLayout[ID_CTS_V].SpeedPosX = 0;
  ViewLayout[ID_CTS_V].SpeedPosY = 75;
  ViewLayout[ID_CTS_V].ShowSpeed = true;
  ViewLayout[ID_CTS_V].WaterTempPosX = 0;
  ViewLayout[ID_CTS_V].WaterTempPosY = 182;
  ViewLayout[ID_CTS_V].ShowWaterTemp = true;

  ViewWarning[ID_CTS_V].Fuel = 80;
  ViewWarning[ID_CTS_V].RPM = 288;         // 7200 / RPMscale
  ViewWarning[ID_CTS_V].WaterTemp = 110;

  ViewWarning[ID_CTS_V].SLI[0][0] = 5600;  // car has 6 forward gears
  ViewWarning[ID_CTS_V].SLI[0][1] = 6200;
  ViewWarning[ID_CTS_V].SLI[0][2] = 6800;
  ViewWarning[ID_CTS_V].SLI[0][3] = 7200;
  ViewWarning[ID_CTS_V].SLI[1][0] = 5600;
  ViewWarning[ID_CTS_V].SLI[1][1] = 6200;
  ViewWarning[ID_CTS_V].SLI[1][2] = 6800;
  ViewWarning[ID_CTS_V].SLI[1][3] = 7200;
  ViewWarning[ID_CTS_V].SLI[2][0] = 5600;
  ViewWarning[ID_CTS_V].SLI[2][1] = 6200;
  ViewWarning[ID_CTS_V].SLI[2][2] = 6800;
  ViewWarning[ID_CTS_V].SLI[2][3] = 7200;
  ViewWarning[ID_CTS_V].SLI[3][0] = 5600;
  ViewWarning[ID_CTS_V].SLI[3][1] = 6200;
  ViewWarning[ID_CTS_V].SLI[3][2] = 6800;
  ViewWarning[ID_CTS_V].SLI[3][3] = 7200;
  ViewWarning[ID_CTS_V].SLI[4][0] = 5600;
  ViewWarning[ID_CTS_V].SLI[4][1] = 6200;
  ViewWarning[ID_CTS_V].SLI[4][2] = 6800;
  ViewWarning[ID_CTS_V].SLI[4][3] = 7200;
  ViewWarning[ID_CTS_V].SLI[5][0] = 5600;
  ViewWarning[ID_CTS_V].SLI[5][1] = 6200;
  ViewWarning[ID_CTS_V].SLI[5][2] = 6800;
  ViewWarning[ID_CTS_V].SLI[5][3] = 7200;
  ViewWarning[ID_CTS_V].SLI[6][0] = 5600;
  ViewWarning[ID_CTS_V].SLI[6][1] = 6200;
  ViewWarning[ID_CTS_V].SLI[6][2] = 6800;
  ViewWarning[ID_CTS_V].SLI[6][3] = 7200;
  ViewWarning[ID_CTS_V].SLI[7][0] = 5600;
  ViewWarning[ID_CTS_V].SLI[7][1] = 6200;
  ViewWarning[ID_CTS_V].SLI[7][2] = 6800;
  ViewWarning[ID_CTS_V].SLI[7][3] = 7200;
  
  // MX-5 NC
  ViewLayout[ID_MX5_NC].CarName[0] = 'M';
  ViewLayout[ID_MX5_NC].CarName[1] = 'X';
  ViewLayout[ID_MX5_NC].CarName[2] = '5';
  ViewLayout[ID_MX5_NC].CarName[3] = ' ';
  ViewLayout[ID_MX5_NC].CarName[4] = 'N';
  ViewLayout[ID_MX5_NC].CarName[5] = 'C';
  ViewLayout[ID_MX5_NC].CarName[6] = 0;
  ViewLayout[ID_MX5_NC].EngineWarningsPosX = 0;
  ViewLayout[ID_MX5_NC].EngineWarningsPosY = 207;
  ViewLayout[ID_MX5_NC].ShowEngineWarnings = true;
  ViewLayout[ID_MX5_NC].FuelPosX = 0;
  ViewLayout[ID_MX5_NC].FuelPosY = 150;
  ViewLayout[ID_MX5_NC].ShowFuel = true;
  ViewLayout[ID_MX5_NC].GearPosX = 280;
  ViewLayout[ID_MX5_NC].GearPosY = 65;
  ViewLayout[ID_MX5_NC].ShowGear = true;
  ViewLayout[ID_MX5_NC].RPMPosY = 25;
  ViewLayout[ID_MX5_NC].RPMscale = 22.8125;  // 7300 / 320
  ViewLayout[ID_MX5_NC].ShowRPM = true;
  ViewLayout[ID_MX5_NC].SLIPosY = 0;
  ViewLayout[ID_MX5_NC].ShowSLI = true;
  ViewLayout[ID_MX5_NC].SpeedPosX = 0;
  ViewLayout[ID_MX5_NC].SpeedPosY = 75;
  ViewLayout[ID_MX5_NC].ShowSpeed = true;
  ViewLayout[ID_MX5_NC].WaterTempPosX = 0;
  ViewLayout[ID_MX5_NC].WaterTempPosY = 182;
  ViewLayout[ID_MX5_NC].ShowWaterTemp = true;

  ViewWarning[ID_MX5_NC].Fuel = 40;
  ViewWarning[ID_MX5_NC].RPM = 296;         // 6750 / RPMscale
  ViewWarning[ID_MX5_NC].WaterTemp = 100;

  ViewWarning[ID_MX5_NC].SLI[0][0] = 6000;  // car has 6 forward gears
  ViewWarning[ID_MX5_NC].SLI[0][1] = 6250;
  ViewWarning[ID_MX5_NC].SLI[0][2] = 6500;
  ViewWarning[ID_MX5_NC].SLI[0][3] = 6750;
  ViewWarning[ID_MX5_NC].SLI[1][0] = 6000;
  ViewWarning[ID_MX5_NC].SLI[1][1] = 6250;
  ViewWarning[ID_MX5_NC].SLI[1][2] = 6500;
  ViewWarning[ID_MX5_NC].SLI[1][3] = 6750;
  ViewWarning[ID_MX5_NC].SLI[2][0] = 6000;
  ViewWarning[ID_MX5_NC].SLI[2][1] = 6250;
  ViewWarning[ID_MX5_NC].SLI[2][2] = 6500;
  ViewWarning[ID_MX5_NC].SLI[2][3] = 6750;
  ViewWarning[ID_MX5_NC].SLI[3][0] = 6000;
  ViewWarning[ID_MX5_NC].SLI[3][1] = 6250;
  ViewWarning[ID_MX5_NC].SLI[3][2] = 6500;
  ViewWarning[ID_MX5_NC].SLI[3][3] = 6750;
  ViewWarning[ID_MX5_NC].SLI[4][0] = 6000;
  ViewWarning[ID_MX5_NC].SLI[4][1] = 6250;
  ViewWarning[ID_MX5_NC].SLI[4][2] = 6500;
  ViewWarning[ID_MX5_NC].SLI[4][3] = 6750;
  ViewWarning[ID_MX5_NC].SLI[5][0] = 6000;
  ViewWarning[ID_MX5_NC].SLI[5][1] = 6250;
  ViewWarning[ID_MX5_NC].SLI[5][2] = 6500;
  ViewWarning[ID_MX5_NC].SLI[5][3] = 6750;
  ViewWarning[ID_MX5_NC].SLI[6][0] = 6000;
  ViewWarning[ID_MX5_NC].SLI[6][1] = 6250;
  ViewWarning[ID_MX5_NC].SLI[6][2] = 6500;
  ViewWarning[ID_MX5_NC].SLI[6][3] = 6750;
  ViewWarning[ID_MX5_NC].SLI[7][0] = 6000;
  ViewWarning[ID_MX5_NC].SLI[7][1] = 6250;
  ViewWarning[ID_MX5_NC].SLI[7][2] = 6500;
  ViewWarning[ID_MX5_NC].SLI[7][3] = 6750;

  // MX-5 ND
  ViewLayout[ID_MX5_ND].CarName[0] = 'M';
  ViewLayout[ID_MX5_ND].CarName[1] = 'X';
  ViewLayout[ID_MX5_ND].CarName[2] = '5';
  ViewLayout[ID_MX5_ND].CarName[3] = ' ';
  ViewLayout[ID_MX5_ND].CarName[4] = 'N';
  ViewLayout[ID_MX5_ND].CarName[5] = 'D';
  ViewLayout[ID_MX5_ND].CarName[6] = 0;
  ViewLayout[ID_MX5_ND].EngineWarningsPosX = 0;
  ViewLayout[ID_MX5_ND].EngineWarningsPosY = 207;
  ViewLayout[ID_MX5_ND].ShowEngineWarnings = true;
  ViewLayout[ID_MX5_ND].FuelPosX = 0;
  ViewLayout[ID_MX5_ND].FuelPosY = 150;
  ViewLayout[ID_MX5_ND].ShowFuel = true;
  ViewLayout[ID_MX5_ND].GearPosX = 280;
  ViewLayout[ID_MX5_ND].GearPosY = 65;
  ViewLayout[ID_MX5_ND].ShowGear = true;
  ViewLayout[ID_MX5_ND].RPMPosY = 25;
  ViewLayout[ID_MX5_ND].RPMscale = 22.8125;  // 7300 / 320
  ViewLayout[ID_MX5_ND].ShowRPM = true;
  ViewLayout[ID_MX5_ND].SLIPosY = 0;
  ViewLayout[ID_MX5_ND].ShowSLI = true;
  ViewLayout[ID_MX5_ND].SpeedPosX = 0;
  ViewLayout[ID_MX5_ND].SpeedPosY = 75;
  ViewLayout[ID_MX5_ND].ShowSpeed = true;
  ViewLayout[ID_MX5_ND].WaterTempPosX = 0;
  ViewLayout[ID_MX5_ND].WaterTempPosY = 182;
  ViewLayout[ID_MX5_ND].ShowWaterTemp = true;

  ViewWarning[ID_MX5_ND].Fuel = 40;
  ViewWarning[ID_MX5_ND].RPM = 281;           // 6400 / RPMscale
  ViewWarning[ID_MX5_ND].WaterTemp = 100;

  ViewWarning[ID_MX5_ND].SLI[0][0] = 5000;    // car has 6 forward gears
  ViewWarning[ID_MX5_ND].SLI[0][1] = 5400;
  ViewWarning[ID_MX5_ND].SLI[0][2] = 5800;
  ViewWarning[ID_MX5_ND].SLI[0][3] = 6400;
  ViewWarning[ID_MX5_ND].SLI[1][0] = 5000;
  ViewWarning[ID_MX5_ND].SLI[1][1] = 5400;
  ViewWarning[ID_MX5_ND].SLI[1][2] = 5800;
  ViewWarning[ID_MX5_ND].SLI[1][3] = 6400;
  ViewWarning[ID_MX5_ND].SLI[2][0] = 5000;
  ViewWarning[ID_MX5_ND].SLI[2][1] = 5400;
  ViewWarning[ID_MX5_ND].SLI[2][2] = 5800;
  ViewWarning[ID_MX5_ND].SLI[2][3] = 6400;
  ViewWarning[ID_MX5_ND].SLI[3][0] = 5000;
  ViewWarning[ID_MX5_ND].SLI[3][1] = 5400;
  ViewWarning[ID_MX5_ND].SLI[3][2] = 5800;
  ViewWarning[ID_MX5_ND].SLI[3][3] = 6400;
  ViewWarning[ID_MX5_ND].SLI[4][0] = 5000;
  ViewWarning[ID_MX5_ND].SLI[4][1] = 5400;
  ViewWarning[ID_MX5_ND].SLI[4][2] = 5800;
  ViewWarning[ID_MX5_ND].SLI[4][3] = 6400;
  ViewWarning[ID_MX5_ND].SLI[5][0] = 5000;
  ViewWarning[ID_MX5_ND].SLI[5][1] = 5400;
  ViewWarning[ID_MX5_ND].SLI[5][2] = 5800;
  ViewWarning[ID_MX5_ND].SLI[5][3] = 6400;
  ViewWarning[ID_MX5_ND].SLI[6][0] = 5000;
  ViewWarning[ID_MX5_ND].SLI[6][1] = 5400;
  ViewWarning[ID_MX5_ND].SLI[6][2] = 5800;
  ViewWarning[ID_MX5_ND].SLI[6][3] = 6400;
  ViewWarning[ID_MX5_ND].SLI[7][0] = 5000;
  ViewWarning[ID_MX5_ND].SLI[7][1] = 5400;
  ViewWarning[ID_MX5_ND].SLI[7][2] = 5800;
  ViewWarning[ID_MX5_ND].SLI[7][3] = 6400;

  // upload the button layout of car selection menu
  // use the formula to determine button outline: (x*80)+10, (y*54)+30, 60, 36)
  // x and y is the position in the 4x4 matrix
  for (int i=0; i<NumOfCars; i++)
    buttons[i] = myButtons.addButton( ((i%4)*80)+10, ((i/4)*54)+30, 60,  36, ViewLayout[i].CarName);
}

// draw the background for the selected car, draw only the active instruments
void DrawBackground(byte ID)
{
  myGLCD.clrScr();
  myGLCD.setFont(BigFont);
  myGLCD.setColor(dc_r, dc_g, dc_b);

  if(ViewLayout[ID].ShowEngineWarnings == true)
  {
    // draw the off state warning lights
    myGLCD.drawBitmap(ViewLayout[ID].EngineWarningsPosX +0, ViewLayout[ID].EngineWarningsPosY, 32, 32, fuelpressure_ok);
    myGLCD.drawBitmap(ViewLayout[ID].EngineWarningsPosX +32, ViewLayout[ID].EngineWarningsPosY, 32, 32, oilpressure_ok);
    myGLCD.drawBitmap(ViewLayout[ID].EngineWarningsPosX +64, ViewLayout[ID].EngineWarningsPosY, 32, 32, water_ok);
    myGLCD.drawBitmap(ViewLayout[ID].EngineWarningsPosX +96, ViewLayout[ID].EngineWarningsPosY, 32, 32, pitspeedlimiter_off);
    myGLCD.drawBitmap(ViewLayout[ID].EngineWarningsPosX +128, ViewLayout[ID].EngineWarningsPosY, 32, 32, stall_off);
    
  }
  
  if(ViewLayout[ID].ShowFuel == true)
  {
    myGLCD.print("Fuel:", ViewLayout[ID].FuelPosX, ViewLayout[ID].FuelPosY);
    myGLCD.print("L", ViewLayout[ID].FuelPosX+160, ViewLayout[ID].FuelPosY);
  }

  if(ViewLayout[ID].ShowSpeed == true)
  {
    myGLCD.print("Speed:", ViewLayout[ID].SpeedPosX, ViewLayout[ID].SpeedPosY);
    myGLCD.print("km/h", ViewLayout[ID].SpeedPosX+160, ViewLayout[ID].SpeedPosY);
  }

  if(ViewLayout[ID].ShowWaterTemp == true)
  {
    myGLCD.print("Water:", ViewLayout[ID].WaterTempPosX, ViewLayout[ID].WaterTempPosY);
    myGLCD.print("C", ViewLayout[ID].WaterTempPosX+160, ViewLayout[ID].WaterTempPosY);
  }
  
  if(ViewLayout[ID].ShowRPM == true)
  {
    // use hard coded instructions to draw the RPM gauge layout per car
    myGLCD.setFont(SmallFont);  // 8x12 pixel
    switch (ID)
    {
      case ID_Skippy:
        myGLCD.drawLine(0, ViewLayout[ID].RPMPosY+35, ViewWarning[ID].RPM-1, ViewLayout[ID].RPMPosY+35);  // horizontal green line
        myGLCD.drawLine(0, ViewLayout[ID].RPMPosY+24, 0, ViewLayout[ID].RPMPosY+34);      // 0 rmp mark
        myGLCD.drawLine(97, ViewLayout[ID].RPMPosY+28, 97, ViewLayout[ID].RPMPosY+34);    // 2000 rpm mark
        myGLCD.drawLine(194, ViewLayout[ID].RPMPosY+28, 194, ViewLayout[ID].RPMPosY+34);  // 4000 rpm mark
        myGLCD.print("10", 40, ViewLayout[ID].RPMPosY+22);         // 1000 rpm mark
        myGLCD.print("30", 137, ViewLayout[ID].RPMPosY+22);        // 3000 rpm mark
        myGLCD.print("50", 234, ViewLayout[ID].RPMPosY+22);        // 5000 rpm mark
        myGLCD.setColor(wc_r, wc_g, wc_b);
        myGLCD.drawLine(291, ViewLayout[ID].RPMPosY+28, 291, ViewLayout[ID].RPMPosY+34);  // 6000 rpm mark
        myGLCD.drawLine(ViewWarning[ID].RPM, ViewLayout[ID].RPMPosY+35, 319, ViewLayout[ID].RPMPosY+35);  // horizontal red line
        break;

      case ID_CTS_V:
        myGLCD.drawLine(0, ViewLayout[ID].RPMPosY+35, ViewWarning[ID].RPM-1, ViewLayout[ID].RPMPosY+35);  // horizontal green line
        myGLCD.drawLine(0, ViewLayout[ID].RPMPosY+24, 0, ViewLayout[ID].RPMPosY+34);      // 0 rmp mark
        myGLCD.drawLine(80, ViewLayout[ID].RPMPosY+28, 80, ViewLayout[ID].RPMPosY+34);    // 2000 rpm mark
        myGLCD.drawLine(160, ViewLayout[ID].RPMPosY+28, 160, ViewLayout[ID].RPMPosY+34);  // 4000 rpm mark
        myGLCD.drawLine(240, ViewLayout[ID].RPMPosY+28, 240, ViewLayout[ID].RPMPosY+34);  // 6000 rpm mark
        myGLCD.print("10", 32, ViewLayout[ID].RPMPosY+22);         // 1000 rpm mark -8 pixel
        myGLCD.print("30", 112, ViewLayout[ID].RPMPosY+22);        // 3000 rpm mark -8 pixel
        myGLCD.print("50", 192, ViewLayout[ID].RPMPosY+22);        // 5000 rpm mark -8 pixel
        myGLCD.print("70", 272, ViewLayout[ID].RPMPosY+22);        // 7000 rpm mark -8 pixel
        myGLCD.setColor(wc_r, wc_g, wc_b);
        myGLCD.drawLine(319, ViewLayout[ID].RPMPosY+28, 319, ViewLayout[ID].RPMPosY+34);  // 8000 rpm mark
        myGLCD.drawLine(ViewWarning[ID].RPM, ViewLayout[ID].RPMPosY+35, 319, ViewLayout[ID].RPMPosY+35);  // horizontal red line
        break;

      case ID_MX5_NC:
        myGLCD.drawLine(0, ViewLayout[ID].RPMPosY+35, ViewWarning[ID].RPM-1, ViewLayout[ID].RPMPosY+35);  // horizontal green line
        myGLCD.drawLine(0, ViewLayout[ID].RPMPosY+24, 0, ViewLayout[ID].RPMPosY+34);      // 0 rmp mark
        myGLCD.drawLine(88, ViewLayout[ID].RPMPosY+28, 88, ViewLayout[ID].RPMPosY+34);    // 2000 rpm mark
        myGLCD.drawLine(175, ViewLayout[ID].RPMPosY+28, 175, ViewLayout[ID].RPMPosY+34);  // 4000 rpm mark
        myGLCD.drawLine(263, ViewLayout[ID].RPMPosY+28, 263, ViewLayout[ID].RPMPosY+34);  // 6000 rpm mark
        myGLCD.print("10", 36, ViewLayout[ID].RPMPosY+22);         // 1000 rpm mark -8 pixel
        myGLCD.print("30", 124, ViewLayout[ID].RPMPosY+22);        // 3000 rpm mark -8 pixel
        myGLCD.print("50", 211, ViewLayout[ID].RPMPosY+22);        // 5000 rpm mark -8 pixel
        myGLCD.setColor(wc_r, wc_g, wc_b);
        myGLCD.print("70", 299, ViewLayout[ID].RPMPosY+22);        // 7000 rpm mark -8 pixel
        myGLCD.drawLine(ViewWarning[ID].RPM, ViewLayout[ID].RPMPosY+35, 319, ViewLayout[ID].RPMPosY+35);  // horizontal red line
        break;

      case ID_MX5_ND:
        myGLCD.drawLine(0, ViewLayout[ID].RPMPosY+35, ViewWarning[ID].RPM-1, ViewLayout[ID].RPMPosY+35);  // horizontal green line
        myGLCD.drawLine(0, ViewLayout[ID].RPMPosY+24, 0, ViewLayout[ID].RPMPosY+34);      // 0 rmp mark
        myGLCD.drawLine(88, ViewLayout[ID].RPMPosY+28, 88, ViewLayout[ID].RPMPosY+34);    // 2000 rpm mark
        myGLCD.drawLine(175, ViewLayout[ID].RPMPosY+28, 175, ViewLayout[ID].RPMPosY+34);  // 4000 rpm mark
        myGLCD.drawLine(263, ViewLayout[ID].RPMPosY+28, 263, ViewLayout[ID].RPMPosY+34);  // 6000 rpm mark
        myGLCD.print("10", 36, ViewLayout[ID].RPMPosY+22);         // 1000 rpm mark -8 pixel
        myGLCD.print("30", 124, ViewLayout[ID].RPMPosY+22);        // 3000 rpm mark -8 pixel
        myGLCD.print("50", 211, ViewLayout[ID].RPMPosY+22);        // 5000 rpm mark -8 pixel
        myGLCD.setColor(wc_r, wc_g, wc_b);
        myGLCD.print("70", 299, ViewLayout[ID].RPMPosY+22);        // 7000 rpm mark -8 pixel
        myGLCD.drawLine(ViewWarning[ID].RPM, ViewLayout[ID].RPMPosY+35, 319, ViewLayout[ID].RPMPosY+35);  // horizontal red line
        break;        
    }
  }

  // draw car name
  myGLCD.setFont(SmallFont);
  myGLCD.setColor(dc_r, dc_g, dc_b);
  myGLCD.print(ViewLayout[ID].CarName, 271, 227);
}

// draw the engine warning icons
void DrawEngineWarnings(byte ID, byte Warning, byte WarningPrev)
{
  byte Filtered, FilteredPrev;
  
  // draw fuel pressure light
  // check only the bit which contains this warning light
  Filtered = Warning & 0x02;
  FilteredPrev = WarningPrev & 0x02;
  if (Filtered != FilteredPrev)
  {
    if (Filtered != 0) myGLCD.drawBitmap(ViewLayout[ID].EngineWarningsPosX +0, ViewLayout[ID].EngineWarningsPosY, 32, 32, fuelpressure_nok);
    else myGLCD.drawBitmap(ViewLayout[ID].EngineWarningsPosX +0, ViewLayout[ID].EngineWarningsPosY, 32, 32, fuelpressure_ok);
  }

  // draw oil pressure light
  // check only the bit which contains this warning light
  Filtered = Warning & 0x04;
  FilteredPrev = WarningPrev & 0x04;
  if (Filtered != FilteredPrev)
  {
    if (Filtered != 0) myGLCD.drawBitmap(ViewLayout[ID].EngineWarningsPosX +32, ViewLayout[ID].EngineWarningsPosY, 32, 32, oilpressure_nok);
    else myGLCD.drawBitmap(ViewLayout[ID].EngineWarningsPosX +32, ViewLayout[ID].EngineWarningsPosY, 32, 32, oilpressure_ok);
  }

  // draw water temp light
  // check only the bit which contains this warning light
  Filtered = Warning & 0x01;
  FilteredPrev = WarningPrev & 0x01;
  if (Filtered != FilteredPrev)
  {
    if (Filtered != 0) myGLCD.drawBitmap(ViewLayout[ID].EngineWarningsPosX +64, ViewLayout[ID].EngineWarningsPosY, 32, 32, water_nok);
    else myGLCD.drawBitmap(ViewLayout[ID].EngineWarningsPosX +64, ViewLayout[ID].EngineWarningsPosY, 32, 32, water_ok);
  }

  // draw pit speed limiter light
  // check only the bit which contains this warning light
  Filtered = Warning & 0x10;
  FilteredPrev = WarningPrev & 0x10;
  if (Filtered != FilteredPrev)
  {
    if (Filtered != 0) myGLCD.drawBitmap(ViewLayout[ID].EngineWarningsPosX +96, ViewLayout[ID].EngineWarningsPosY, 32, 32, pitspeedlimiter_on);
    else myGLCD.drawBitmap(ViewLayout[ID].EngineWarningsPosX +96, ViewLayout[ID].EngineWarningsPosY, 32, 32, pitspeedlimiter_off);
  }

  // draw stall sign light
  // check only the bit which contains this warning light
  Filtered = Warning & 0x08;
  FilteredPrev = WarningPrev & 0x08;
  if (Filtered != FilteredPrev)
  {
    if (Filtered != 0) myGLCD.drawBitmap(ViewLayout[ID].EngineWarningsPosX +128, ViewLayout[ID].EngineWarningsPosY, 32, 32, stall_on);
    else myGLCD.drawBitmap(ViewLayout[ID].EngineWarningsPosX +128, ViewLayout[ID].EngineWarningsPosY, 32, 32, stall_off);
  }
}

// draw fuel gauge
void DrawFuel(byte ID, int Fuel, int FuelPrev)
{
  myGLCD.setFont(BigFont);
  if (Fuel <= ViewWarning[ID].Fuel) myGLCD.setColor(wc_r, wc_g, wc_b);
  else myGLCD.setColor(dc_r, dc_g, dc_b);
  
  myGLCD.print(".", ViewLayout[ID].FuelPosX+128, ViewLayout[ID].FuelPosY);
  myGLCD.printNumI(Fuel % 10, ViewLayout[ID].FuelPosX+144, ViewLayout[ID].FuelPosY);
                  
  // draw the integer value right justified
  if (Fuel < 100)
  {
    myGLCD.printNumI(Fuel / 10, ViewLayout[ID].FuelPosX+112, ViewLayout[ID].FuelPosY);
    if (FuelPrev >= 100)  // previous value was greater than 9, so we have to clear the above 10 value from the screen
    {
      myGLCD.setColor(0, 0, 0);
      myGLCD.fillRect(ViewLayout[ID].FuelPosX+80, ViewLayout[ID].FuelPosY, ViewLayout[ID].FuelPosX+112, ViewLayout[ID].FuelPosY+16);
    }
  }
  else if (Fuel < 1000)
  {
    myGLCD.printNumI(Fuel / 10, ViewLayout[ID].FuelPosX+96, ViewLayout[ID].FuelPosY);
    if (FuelPrev >= 1000)  // previous value was greater than 99, so we have to clear the above 100 value from the screen
    {
      myGLCD.setColor(0, 0, 0);
      myGLCD.fillRect(ViewLayout[ID].FuelPosX+80, ViewLayout[ID].FuelPosY, ViewLayout[ID].FuelPosX+96, ViewLayout[ID].FuelPosY+16);
    }
  }
  else if (Fuel < 10000) myGLCD.printNumI(Fuel / 10, ViewLayout[ID].FuelPosX+80, ViewLayout[ID].FuelPosY);
}

// draw gear number
void DrawGear(byte ID, char Gear)
{
  myGLCD.setFont(SevenSegNumFont);  // 32x50
  if (InData->Gear == -1)  // reverse gear
  {
    myGLCD.setColor(wc_r, wc_g, wc_b);
    myGLCD.print("1", ViewLayout[ID].GearPosX, ViewLayout[ID].GearPosY);
  }
  else
  {
    myGLCD.setColor(dc_r, dc_g, dc_b);
    if (Gear>=0 and Gear<10) myGLCD.printNumI(Gear, ViewLayout[ID].GearPosX, ViewLayout[ID].GearPosY);
  }
}

// draw water temperature gauge
void DrawWaterTemp(byte ID, int WaterTemp, int WaterTempPrev)
{
  myGLCD.setFont(BigFont);
  if (WaterTemp >= ViewWarning[ID].WaterTemp) myGLCD.setColor(wc_r, wc_g, wc_b);
  else myGLCD.setColor(dc_r, dc_g, dc_b);
                  
  if (WaterTemp < 10 && WaterTemp > 0)
  {
    myGLCD.printNumI(WaterTemp, ViewLayout[ID].WaterTempPosX+144, ViewLayout[ID].WaterTempPosY);
    if (WaterTempPrev >= 10)  // previous value was greater than 9, so we have to clear the above 10 value from the screen
    {
      myGLCD.setColor(0, 0, 0);
      myGLCD.fillRect(ViewLayout[ID].WaterTempPosX+112, ViewLayout[ID].WaterTempPosY, ViewLayout[ID].WaterTempPosX+144, ViewLayout[ID].WaterTempPosY+16);
    }
  }
  else if (WaterTemp < 100 && WaterTemp > 0)
       {
         myGLCD.printNumI(WaterTemp, ViewLayout[ID].WaterTempPosX+128, ViewLayout[ID].WaterTempPosY);
         if (WaterTempPrev >= 100)  // previous value was greater than 99, so we have to clear the above 100 value from the screen
         {
           myGLCD.setColor(0, 0, 0);
           myGLCD.fillRect(ViewLayout[ID].WaterTempPosX+112, ViewLayout[ID].WaterTempPosY, ViewLayout[ID].WaterTempPosX+128, ViewLayout[ID].WaterTempPosY+16);
         }                    
       }
       else if (WaterTemp<1000 && WaterTemp>0) myGLCD.printNumI(WaterTemp, ViewLayout[ID].WaterTempPosX+112, ViewLayout[ID].WaterTempPosY);
}

// draw RPM gauge
void DrawRPM(byte ID, int RPM, int RPMPrev)
{
  // RPM is bigger than the previous
  if (RPMPrev < RPM)
  {
    if (RPM >= ViewWarning[ID].RPM)  // RPM is bigger than warning limit
    {
      if (RPMPrev < ViewWarning[ID].RPM)
      {
        // we have to draw both color on the RPM gauge
        myGLCD.setColor(dc_r, dc_g, dc_b);
        myGLCD.fillRect(RPMPrev, ViewLayout[ID].RPMPosY, ViewWarning[ID].RPM-1, ViewLayout[ID].RPMPosY+20);
        myGLCD.setColor(wc_r, wc_g, wc_b);
        myGLCD.fillRect(ViewWarning[ID].RPM, ViewLayout[ID].RPMPosY, RPM, ViewLayout[ID].RPMPosY+20);
      }
      else
      {
        // only the warning color have to be used
        myGLCD.setColor(wc_r, wc_g, wc_b);
        myGLCD.fillRect(RPMPrev, ViewLayout[ID].RPMPosY, RPM, ViewLayout[ID].RPMPosY+20);
      }
    }
    else
    {
      // only the default color have to be used
      myGLCD.setColor(dc_r, dc_g, dc_b);
      myGLCD.fillRect(RPMPrev, ViewLayout[ID].RPMPosY, RPM, ViewLayout[ID].RPMPosY+20);
    }
  }

  // RPM is lower than the previous
  if (RPMPrev > RPM)
  {
    myGLCD.setColor(0, 0, 0);
    myGLCD.fillRect(RPMPrev, ViewLayout[ID].RPMPosY, RPM, ViewLayout[ID].RPMPosY+20);
  }
}

// draw speed gauge
void DrawSpeed(byte ID, int Speed, int SpeedPrev)
{
  myGLCD.setFont(BigFont);
  myGLCD.setColor(dc_r, dc_g, dc_b);
  if (Speed<10)
  {
    myGLCD.printNumI(Speed, ViewLayout[ID].SpeedPosX+144, ViewLayout[ID].SpeedPosY);
    if (SpeedPrev>=10)
    {
      myGLCD.setColor(0, 0, 0);
      myGLCD.fillRect(ViewLayout[ID].SpeedPosX+112, ViewLayout[ID].SpeedPosY, ViewLayout[ID].SpeedPosX+144, ViewLayout[ID].SpeedPosY+16);
    }
  }
  else if (Speed<100)
       {
         myGLCD.printNumI(Speed, ViewLayout[ID].SpeedPosX+128, ViewLayout[ID].SpeedPosY);
         if (SpeedPrev>=100)
         {
           myGLCD.setColor(0, 0, 0);
           myGLCD.fillRect(ViewLayout[ID].SpeedPosX+112, ViewLayout[ID].SpeedPosY, ViewLayout[ID].SpeedPosX+128, ViewLayout[ID].SpeedPosY+16);
         }
       }
  else if (Speed<1000) myGLCD.printNumI(Speed, ViewLayout[ID].SpeedPosX+112, ViewLayout[ID].SpeedPosY);
}

// draw shift light indicator
void DrawSLI(byte ID, int SLI, int SLIPrev)
{
  if (SLI < SLIPrev)
  {
    // clear only the disappeared indicators
    myGLCD.setColor(0, 0, 0);
    for (int i=SLI; i<=SLIPrev-1; i++)
    {
      switch (i)
      {
        case 0: myGLCD.fillRect(10, ViewLayout[ID].SLIPosY, 70, ViewLayout[ID].SLIPosY+16);
                break;
        case 1: myGLCD.fillRect(90, ViewLayout[ID].SLIPosY, 150, ViewLayout[ID].SLIPosY+16);
                break;
        case 2: myGLCD.fillRect(170, ViewLayout[ID].SLIPosY, 230, ViewLayout[ID].SLIPosY+16);
                break;
        case 3: myGLCD.fillRect(250, ViewLayout[ID].SLIPosY, 310, ViewLayout[ID].SLIPosY+16);
                break;
      }
    }
  }
  else
  {
    // draw only the appeared indicators
    for (int i=SLIPrev+1; i<= SLI; i++)
    {
      switch (i)
      {
        case 1: myGLCD.setColor(dc_r, dc_g, dc_b);
                myGLCD.fillRect(10, ViewLayout[ID].SLIPosY, 70, ViewLayout[ID].SLIPosY+16);
                break;
        case 2: myGLCD.setColor(dc_r, dc_g, dc_b);
                myGLCD.fillRect(90, ViewLayout[ID].SLIPosY, 150, ViewLayout[ID].SLIPosY+16);
                break;
        case 3: myGLCD.setColor(mc_r, mc_g, mc_b);
                myGLCD.fillRect(170, ViewLayout[ID].SLIPosY, 230, ViewLayout[ID].SLIPosY+16);
                break;
        case 4: myGLCD.setColor(wc_r, wc_g, wc_b);
                myGLCD.fillRect(250, ViewLayout[ID].SLIPosY, 310, ViewLayout[ID].SLIPosY+16);
                break;
     }
   }
  }
}

void DrawCarSelectionMenu()
{
  myGLCD.clrScr();
  myGLCD.setFont(BigFont);    // 16x16
  myGLCD.setColor(dc_r, dc_g, dc_b);
  myGLCD.print("Car Selection Menu", 14, 0);

  myButtons.drawButtons();
}

void setup()
{
  randomSeed(analogRead(0));
  
  // setup the LCD
  myGLCD.InitLCD();

  // setup the touch sensing
  myTouch.InitTouch();
  myButtons.setTextFont(SmallFont);
  
  // start serial port at speed of 115200 bps
  Serial.begin(115200, SERIAL_8N1);

  // initialize internal variables
  InData = new SIncomingData;  // allocate the data structure of the telemetry data
  pInData = (byte*)InData;     // set the byte array pointer to the telemetry data
  ResetInternalData();

  UploadCarProfiles();
  DrawBackground(DefaultCar);  // draw the default screen layout
  ActiveCar = DefaultCar;
}

void loop()
{
  int rpm_int, pressed_button;
  char gear;
  int x, y;

  // read serial port
  if (Serial.available() > 0)
  {
    inByte = Serial.read();

    // check the incoming byte, store the byte in the correct telemetry data block position
    switch (blockpos)
    {
      // first identify if the three byte long header is received in the correct order
      case 0: if (inByte == 3) blockpos = 1;
              break;
      case 1: if (inByte == 12) blockpos = 2;
              else blockpos = 0;
              break;
      case 2: if (inByte == 48) blockpos = 3;
              else blockpos = 0;
              break;
      default:
              if (blockpos >=3) // the three byte long identification header was found, now we store the next incoming bytes in our telemetry data block.
              {
                *(pInData +blockpos-3) = inByte;  // we don't store the identification header
                blockpos++;
              }
              if (blockpos == sizeof(SIncomingData)+4)  // last byte of the incoming telemetry data was received, now the screen can be drawn
              {
                // draw screen and draw only activated gauges
                blockpos = 0; // reset block position

                // draw Engine Warning lights
                Screen[1].EngineWarnings = InData->EngineWarnings;
                if (Screen[0].EngineWarnings != Screen[1].EngineWarnings && ViewLayout[ActiveCar].ShowEngineWarnings == true) DrawEngineWarnings(ActiveCar, Screen[1].EngineWarnings, Screen[0].EngineWarnings);

                // draw RPM gauge
                Screen[1].RPMgauge = (int)(InData->RPM / ViewLayout[ActiveCar].RPMscale);
                if (Screen[1].RPMgauge > ScreenWidth) Screen[1].RPMgauge = ScreenWidth;  // limit RPM gauge to maximum display width
                if (Screen[0].RPMgauge != Screen[1].RPMgauge && ViewLayout[ActiveCar].ShowRPM == true) DrawRPM(ActiveCar, Screen[1].RPMgauge, Screen[0].RPMgauge);

                // draw gear number
                Screen[1].Gear = InData->Gear;
                if (Screen[0].Gear != Screen[1].Gear && ViewLayout[ActiveCar].ShowGear == true) DrawGear(ActiveCar, Screen[1].Gear);

                // draw fuel level gauge
                Screen[1].Fuel = (int)(InData->Fuel*10); // convert float data to int and keep the first digit of the fractional part also
                if (Screen[0].Fuel != Screen[1].Fuel && ViewLayout[ActiveCar].ShowFuel == true) DrawFuel(ActiveCar, Screen[1].Fuel, Screen[0].Fuel);

                // draw speed gauge
                Screen[1].Speed = (int)(InData->Speed*3.6);  // convert m/s to km/h
                if (Screen[0].Speed != Screen[1].Speed && ViewLayout[ActiveCar].ShowSpeed == true) DrawSpeed(ActiveCar, Screen[1].Speed, Screen[0].Speed);

                // draw water temperature gauge
                Screen[1].WaterTemp = (int)InData->WaterTemp;
                if (Screen[0].WaterTemp != Screen[1].WaterTemp && ViewLayout[ActiveCar].ShowWaterTemp == true) DrawWaterTemp(ActiveCar, Screen[1].WaterTemp, Screen[0].WaterTemp);
                
                // draw shift light indicator
                gear = InData->Gear+1;  // "-1" corresponds to reverse but index should start with "0"
                rpm_int = (int)InData->RPM;
                if (rpm_int <= ViewWarning[ActiveCar].SLI[gear][0]) Screen[1].SLI = 0; // determine how many light to be activated for the current gear
                else
                {
                  if (rpm_int > ViewWarning[ActiveCar].SLI[gear][3]) Screen[1].SLI = 4;
                  else if (rpm_int > ViewWarning[ActiveCar].SLI[gear][2]) Screen[1].SLI = 3;
                       else if (rpm_int > ViewWarning[ActiveCar].SLI[gear][1]) Screen[1].SLI = 2;
                            else if (rpm_int > ViewWarning[ActiveCar].SLI[gear][0]) Screen[1].SLI = 1;
                }
                if (Screen[0].SLI != Screen[1].SLI && ViewLayout[ActiveCar].ShowSLI == true) DrawSLI(ActiveCar, Screen[1].SLI, Screen[0].SLI);

                // update old screen data
                Screen[0].EngineWarnings = Screen[1].EngineWarnings;
                Screen[0].Fuel           = Screen[1].Fuel;
                Screen[0].Gear           = Screen[1].Gear;
                Screen[0].RPMgauge       = Screen[1].RPMgauge;
                Screen[0].Speed          = Screen[1].Speed;
                Screen[0].WaterTemp      = Screen[1].WaterTemp;
                Screen[0].SLI            = Screen[1].SLI;
  
                // clear the incoming serial buffer which was filled up during the drawing, if it is not done then data misalignment can happen when we read the next telemetry block
                inByte = Serial.available();
                for (int i=0; i<inByte; i++) Serial.read();
              }
              break;
    }
  }

  // check if the screen was touched, if yes then draw the car selection menu
  if (myTouch.dataAvailable())
  {
    myTouch.setPrecision(PREC_LOW); // we don't need too much precision now so we can spare the CPU cycles
    myTouch.read();
    x = myTouch.getX();
    y = myTouch.getY();

    // check if the screen was pressed in the middle 1/3rd area and draw the car selection menu only if it was
    if (x>107 && x<213 && y>80 && y<160)
    {
      while (myTouch.dataAvailable()) myTouch.read(); // wait until the screen is not touched

      DrawCarSelectionMenu();
      myTouch.setPrecision(PREC_MEDIUM);          // set medium precision for the buttons

      do
      {
        while (!myTouch.dataAvailable());           // wait until the screen is touched
        pressed_button = myButtons.checkButtons();
      }
      while (pressed_button == -1);

      for (int i=0; i<NumOfCars; i++) if (pressed_button == buttons[i]) ActiveCar = i;  // set the active car profile based on the pressed button
    
      myGLCD.clrScr();
      DrawBackground(ActiveCar);
      ResetInternalData(); // clear the old screen data
    
      // clear the incoming serial buffer which was filled up during the drawing, if it is not done then data misalignment can happen when we read the next telemetry block
      inByte = Serial.available();
      for (int i=0; i<inByte; i++) Serial.read();
    }
  }
}

