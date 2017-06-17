#include <UTFT.h>
#include <URTouch.h>
#include <UTFT_Buttons.h>

// define car identification numbers
#define NumOfCars 5     // number of car profiles, maximum is 16
#define DefaultCar 4    // car profile to show at startup
#define ID_Skippy 0
#define ID_CTS_V  1
#define ID_MX5_NC 2
#define ID_MX5_ND 3
#define ID_FR20   4

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
#define bc_r 70   // background color
#define bc_g 180
#define bc_b 70

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
URTouch  myTouch(6, 5, 4, 3, 2);

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
  bool ShowRPM;
  int SLIPosY;
  bool ShowSLI;
  int SpeedPosX;
  int SpeedPosY;
  bool ShowSpeed;
  int WaterTempPosX;
  int WaterTempPosY;
  bool ShowWaterTemp;
};

// limits where different drawing color have to be used
struct SCarProfile
{
  char CarName[10];
  int Fuel;                   // value in liter * 10
  int RPM;                    // value where the redline starts in pixels
  float RPMscale;             // actual RPM value is divided by this number to scale it to display coordinate
  int WaterTemp;              // value in Celsius
  int SLI[8];                 // RPM values for each SLI light
};

byte inByte;                  // incoming byte from the serial port
int blockpos;                 // position of the actual incoming byte in the telemetry data block
SIncomingData* InData;        // pointer to access the telemetry data as a structure
byte* pInData;                // pointer to access the telemetry data as a byte array
SViewData Screen[2];          // store the actual and the previous screen data
int buttons[NumOfCars];       // handle for the touch buttons
byte ActiveCar;               // active car profile

// variables to manage screen layout
SScreenLayout ScreenLayout;            // store screen layout
SCarProfile CarProfile[NumOfCars];     // store warning limits for each car

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
void UploadProfiles()
{
  // upload gauge positions
  ScreenLayout.EngineWarningsPosX = 0;
  ScreenLayout.EngineWarningsPosY = 207;
  ScreenLayout.ShowEngineWarnings = true;
  ScreenLayout.FuelPosX = 0;
  ScreenLayout.FuelPosY = 150;
  ScreenLayout.ShowFuel = true;
  ScreenLayout.GearPosX = 280;
  ScreenLayout.GearPosY = 78;
  ScreenLayout.ShowGear = true;
  ScreenLayout.RPMPosY = 30;
  ScreenLayout.ShowRPM = true;
  ScreenLayout.SLIPosY = 0;
  ScreenLayout.ShowSLI = true;
  ScreenLayout.SpeedPosX = 0;
  ScreenLayout.SpeedPosY = 75;
  ScreenLayout.ShowSpeed = true;
  ScreenLayout.WaterTempPosX = 0;
  ScreenLayout.WaterTempPosY = 182;
  ScreenLayout.ShowWaterTemp = true;

  // Skippy
  CarProfile[ID_Skippy].CarName[0] = 'S';
  CarProfile[ID_Skippy].CarName[1] = 'k';
  CarProfile[ID_Skippy].CarName[2] = 'i';
  CarProfile[ID_Skippy].CarName[3] = 'p';
  CarProfile[ID_Skippy].CarName[4] = 'p';
  CarProfile[ID_Skippy].CarName[5] = 'y';
  CarProfile[ID_Skippy].CarName[6] = 0;

  CarProfile[ID_Skippy].Fuel = 25;
  CarProfile[ID_Skippy].RPM = 291;          // 6000 / RPMscale
  CarProfile[ID_Skippy].RPMscale = 20.625;  // 6600 / 320
  CarProfile[ID_Skippy].WaterTemp = 90;

  CarProfile[ID_Skippy].SLI[0] = 4700;
  CarProfile[ID_Skippy].SLI[1] = 4900;
  CarProfile[ID_Skippy].SLI[2] = 5100;
  CarProfile[ID_Skippy].SLI[3] = 5300;
  CarProfile[ID_Skippy].SLI[4] = 5500;
  CarProfile[ID_Skippy].SLI[5] = 5700;
  CarProfile[ID_Skippy].SLI[6] = 5900;
  CarProfile[ID_Skippy].SLI[7] = 6100;

  // CTS-V
  CarProfile[ID_CTS_V].CarName[0] = 'C';
  CarProfile[ID_CTS_V].CarName[1] = 'T';
  CarProfile[ID_CTS_V].CarName[2] = 'S';
  CarProfile[ID_CTS_V].CarName[3] = '-';
  CarProfile[ID_CTS_V].CarName[4] = 'V';
  CarProfile[ID_CTS_V].CarName[5] = 0;

  CarProfile[ID_CTS_V].Fuel = 80;
  CarProfile[ID_CTS_V].RPM = 288;         // 7200 / RPMscale
  CarProfile[ID_CTS_V].RPMscale = 25;     // 8000 / 320
  CarProfile[ID_CTS_V].WaterTemp = 110;

  CarProfile[ID_CTS_V].SLI[0] = 6600;
  CarProfile[ID_CTS_V].SLI[1] = 6700;
  CarProfile[ID_CTS_V].SLI[2] = 6800;
  CarProfile[ID_CTS_V].SLI[3] = 6900;
  CarProfile[ID_CTS_V].SLI[4] = 7000;
  CarProfile[ID_CTS_V].SLI[5] = 7100;
  CarProfile[ID_CTS_V].SLI[6] = 7200;
  CarProfile[ID_CTS_V].SLI[7] = 7300;
  
  // MX-5 NC
  CarProfile[ID_MX5_NC].CarName[0] = 'M';
  CarProfile[ID_MX5_NC].CarName[1] = 'X';
  CarProfile[ID_MX5_NC].CarName[2] = '5';
  CarProfile[ID_MX5_NC].CarName[3] = ' ';
  CarProfile[ID_MX5_NC].CarName[4] = 'N';
  CarProfile[ID_MX5_NC].CarName[5] = 'C';
  CarProfile[ID_MX5_NC].CarName[6] = 0;

  CarProfile[ID_MX5_NC].Fuel = 40;
  CarProfile[ID_MX5_NC].RPM = 296;           // 6750 / RPMscale
  CarProfile[ID_MX5_NC].RPMscale = 22.8125;  // 7300 / 320
  CarProfile[ID_MX5_NC].WaterTemp = 100;

  CarProfile[ID_MX5_NC].SLI[0] = 6000;
  CarProfile[ID_MX5_NC].SLI[1] = 6125;
  CarProfile[ID_MX5_NC].SLI[2] = 6250;
  CarProfile[ID_MX5_NC].SLI[3] = 6375;
  CarProfile[ID_MX5_NC].SLI[4] = 6500;
  CarProfile[ID_MX5_NC].SLI[5] = 6625;
  CarProfile[ID_MX5_NC].SLI[6] = 6750;
  CarProfile[ID_MX5_NC].SLI[7] = 6850;

  // MX-5 ND
  CarProfile[ID_MX5_ND].CarName[0] = 'M';
  CarProfile[ID_MX5_ND].CarName[1] = 'X';
  CarProfile[ID_MX5_ND].CarName[2] = '5';
  CarProfile[ID_MX5_ND].CarName[3] = ' ';
  CarProfile[ID_MX5_ND].CarName[4] = 'N';
  CarProfile[ID_MX5_ND].CarName[5] = 'D';
  CarProfile[ID_MX5_ND].CarName[6] = 0;

  CarProfile[ID_MX5_ND].Fuel = 40;
  CarProfile[ID_MX5_ND].RPM = 281;           // 6400 / RPMscale
  CarProfile[ID_MX5_ND].RPMscale = 22.8125;  // 7300 / 320
  CarProfile[ID_MX5_ND].WaterTemp = 100;

  CarProfile[ID_MX5_ND].SLI[0] = 5000;
  CarProfile[ID_MX5_ND].SLI[1] = 5200;
  CarProfile[ID_MX5_ND].SLI[2] = 5400;
  CarProfile[ID_MX5_ND].SLI[3] = 5600;
  CarProfile[ID_MX5_ND].SLI[4] = 5800;
  CarProfile[ID_MX5_ND].SLI[5] = 6200;
  CarProfile[ID_MX5_ND].SLI[6] = 6400;
  CarProfile[ID_MX5_ND].SLI[7] = 6600;

  // Formula Renault 2.0
  CarProfile[ID_FR20].CarName[0] = 'F';
  CarProfile[ID_FR20].CarName[1] = 'R';
  CarProfile[ID_FR20].CarName[2] = ' ';
  CarProfile[ID_FR20].CarName[3] = '2';
  CarProfile[ID_FR20].CarName[4] = '.';
  CarProfile[ID_FR20].CarName[5] = '0';
  CarProfile[ID_FR20].CarName[6] = 0;

  CarProfile[ID_FR20].Fuel = 40;
  CarProfile[ID_FR20].RPM = 307;           // 7300 / RPMscale
  CarProfile[ID_FR20].RPMscale = 23.75;    // 7600 / 320
  CarProfile[ID_FR20].WaterTemp = 100;

  CarProfile[ID_FR20].SLI[0] = 6600;
  CarProfile[ID_FR20].SLI[1] = 6700;
  CarProfile[ID_FR20].SLI[2] = 6800;
  CarProfile[ID_FR20].SLI[3] = 6900;
  CarProfile[ID_FR20].SLI[4] = 7000;
  CarProfile[ID_FR20].SLI[5] = 7100;
  CarProfile[ID_FR20].SLI[6] = 7200;
  CarProfile[ID_FR20].SLI[7] = 7300;

  // upload the button layout of car selection menu
  // use the formula to determine button outline: (x*80)+10, (y*54)+30, 60, 36)
  // x and y is the position in the 4x4 matrix
  for (int i=0; i<NumOfCars; i++)
    buttons[i] = myButtons.addButton( ((i%4)*80)+10, ((i/4)*54)+30, 60,  36, CarProfile[i].CarName);
}

// draw the background and the active instruments for the car
void DrawBackground(byte ID)
{
  myGLCD.clrScr();

  if(ScreenLayout.ShowEngineWarnings == true)
  {
    // draw the off state warning lights
    myGLCD.drawBitmap(ScreenLayout.EngineWarningsPosX +0, ScreenLayout.EngineWarningsPosY, 32, 32, fuelpressure_ok);
    myGLCD.drawBitmap(ScreenLayout.EngineWarningsPosX +32, ScreenLayout.EngineWarningsPosY, 32, 32, oilpressure_ok);
    myGLCD.drawBitmap(ScreenLayout.EngineWarningsPosX +64, ScreenLayout.EngineWarningsPosY, 32, 32, water_ok);
    myGLCD.drawBitmap(ScreenLayout.EngineWarningsPosX +112, ScreenLayout.EngineWarningsPosY, 32, 32, pitspeedlimiter_off);
    myGLCD.drawBitmap(ScreenLayout.EngineWarningsPosX +144, ScreenLayout.EngineWarningsPosY, 32, 32, stall_off);
    
  }
  
  if(ScreenLayout.ShowFuel == true)
  {
    myGLCD.setFont(BigFont);
    myGLCD.setColor(dc_r, dc_g, dc_b);
    
    myGLCD.print("Fuel:", ScreenLayout.FuelPosX, ScreenLayout.FuelPosY);
    myGLCD.print("L", ScreenLayout.FuelPosX+160, ScreenLayout.FuelPosY);
  }

  if(ScreenLayout.ShowSpeed == true)
  {
    myGLCD.setFont(BigFont);
    myGLCD.setColor(dc_r, dc_g, dc_b);
    
    myGLCD.print("Speed:", ScreenLayout.SpeedPosX, ScreenLayout.SpeedPosY);
    myGLCD.print("km/h", ScreenLayout.SpeedPosX+160, ScreenLayout.SpeedPosY);
  }

  if(ScreenLayout.ShowWaterTemp == true)
  {
    myGLCD.setFont(BigFont);
    myGLCD.setColor(dc_r, dc_g, dc_b);
    
    myGLCD.print("Water:", ScreenLayout.WaterTempPosX, ScreenLayout.WaterTempPosY);
    myGLCD.print("C", ScreenLayout.WaterTempPosX+160, ScreenLayout.WaterTempPosY);
  }
  
  if(ScreenLayout.ShowRPM == true)
  {
    // use hard coded instructions to draw the RPM gauge layout per car
    myGLCD.setFont(SmallFont);  // 8x12 pixel
    myGLCD.setColor(bc_r, bc_g, bc_b);

    switch (ID)
    {
      case ID_Skippy:
        myGLCD.drawLine(0, ScreenLayout.RPMPosY+35, CarProfile[ID_Skippy].RPM-1, ScreenLayout.RPMPosY+35);  // horizontal green line
        myGLCD.drawLine(0, ScreenLayout.RPMPosY+24, 0, ScreenLayout.RPMPosY+34);      // 0 rmp mark
        myGLCD.drawLine(97, ScreenLayout.RPMPosY+28, 97, ScreenLayout.RPMPosY+34);    // 2000 rpm mark
        myGLCD.drawLine(194, ScreenLayout.RPMPosY+28, 194, ScreenLayout.RPMPosY+34);  // 4000 rpm mark
        myGLCD.print("10", 40, ScreenLayout.RPMPosY+22);         // 1000 rpm mark
        myGLCD.print("30", 137, ScreenLayout.RPMPosY+22);        // 3000 rpm mark
        myGLCD.print("50", 234, ScreenLayout.RPMPosY+22);        // 5000 rpm mark
        myGLCD.setColor(wc_r, wc_g, wc_b);
        myGLCD.drawLine(291, ScreenLayout.RPMPosY+28, 291, ScreenLayout.RPMPosY+34);  // 6000 rpm mark
        myGLCD.drawLine(CarProfile[ID_Skippy].RPM, ScreenLayout.RPMPosY+35, 319, ScreenLayout.RPMPosY+35);  // horizontal red line
        break;

      case ID_CTS_V:
        myGLCD.drawLine(0, ScreenLayout.RPMPosY+35, CarProfile[ID_CTS_V].RPM-1, ScreenLayout.RPMPosY+35);  // horizontal green line
        myGLCD.drawLine(0, ScreenLayout.RPMPosY+24, 0, ScreenLayout.RPMPosY+34);      // 0 rmp mark
        myGLCD.drawLine(80, ScreenLayout.RPMPosY+28, 80, ScreenLayout.RPMPosY+34);    // 2000 rpm mark
        myGLCD.drawLine(160, ScreenLayout.RPMPosY+28, 160, ScreenLayout.RPMPosY+34);  // 4000 rpm mark
        myGLCD.drawLine(240, ScreenLayout.RPMPosY+28, 240, ScreenLayout.RPMPosY+34);  // 6000 rpm mark
        myGLCD.print("10", 32, ScreenLayout.RPMPosY+22);         // 1000 rpm mark -8 pixel
        myGLCD.print("30", 112, ScreenLayout.RPMPosY+22);        // 3000 rpm mark -8 pixel
        myGLCD.print("50", 192, ScreenLayout.RPMPosY+22);        // 5000 rpm mark -8 pixel
        myGLCD.print("70", 272, ScreenLayout.RPMPosY+22);        // 7000 rpm mark -8 pixel
        myGLCD.setColor(wc_r, wc_g, wc_b);
        myGLCD.drawLine(319, ScreenLayout.RPMPosY+28, 319, ScreenLayout.RPMPosY+34);  // 8000 rpm mark
        myGLCD.drawLine(CarProfile[ID_CTS_V].RPM, ScreenLayout.RPMPosY+35, 319, ScreenLayout.RPMPosY+35);  // horizontal red line
        break;

      case ID_MX5_NC:
        myGLCD.drawLine(0, ScreenLayout.RPMPosY+35, CarProfile[ID_MX5_NC].RPM-1, ScreenLayout.RPMPosY+35);  // horizontal green line
        myGLCD.drawLine(0, ScreenLayout.RPMPosY+24, 0, ScreenLayout.RPMPosY+34);      // 0 rmp mark
        myGLCD.drawLine(88, ScreenLayout.RPMPosY+28, 88, ScreenLayout.RPMPosY+34);    // 2000 rpm mark
        myGLCD.drawLine(175, ScreenLayout.RPMPosY+28, 175, ScreenLayout.RPMPosY+34);  // 4000 rpm mark
        myGLCD.drawLine(263, ScreenLayout.RPMPosY+28, 263, ScreenLayout.RPMPosY+34);  // 6000 rpm mark
        myGLCD.print("10", 36, ScreenLayout.RPMPosY+22);         // 1000 rpm mark -8 pixel
        myGLCD.print("30", 124, ScreenLayout.RPMPosY+22);        // 3000 rpm mark -8 pixel
        myGLCD.print("50", 211, ScreenLayout.RPMPosY+22);        // 5000 rpm mark -8 pixel
        myGLCD.setColor(wc_r, wc_g, wc_b);
        myGLCD.print("70", 299, ScreenLayout.RPMPosY+22);        // 7000 rpm mark -8 pixel
        myGLCD.drawLine(CarProfile[ID_MX5_NC].RPM, ScreenLayout.RPMPosY+35, 319, ScreenLayout.RPMPosY+35);  // horizontal red line
        break;

      case ID_MX5_ND:
        myGLCD.drawLine(0, ScreenLayout.RPMPosY+35, CarProfile[ID_MX5_ND].RPM-1, ScreenLayout.RPMPosY+35);  // horizontal green line
        myGLCD.drawLine(0, ScreenLayout.RPMPosY+24, 0, ScreenLayout.RPMPosY+34);      // 0 rmp mark
        myGLCD.drawLine(88, ScreenLayout.RPMPosY+28, 88, ScreenLayout.RPMPosY+34);    // 2000 rpm mark
        myGLCD.drawLine(175, ScreenLayout.RPMPosY+28, 175, ScreenLayout.RPMPosY+34);  // 4000 rpm mark
        myGLCD.drawLine(263, ScreenLayout.RPMPosY+28, 263, ScreenLayout.RPMPosY+34);  // 6000 rpm mark
        myGLCD.print("10", 36, ScreenLayout.RPMPosY+22);         // 1000 rpm mark -8 pixel
        myGLCD.print("30", 124, ScreenLayout.RPMPosY+22);        // 3000 rpm mark -8 pixel
        myGLCD.print("50", 211, ScreenLayout.RPMPosY+22);        // 5000 rpm mark -8 pixel
        myGLCD.setColor(wc_r, wc_g, wc_b);
        myGLCD.print("70", 299, ScreenLayout.RPMPosY+22);        // 7000 rpm mark -8 pixel
        myGLCD.drawLine(CarProfile[ID_MX5_ND].RPM, ScreenLayout.RPMPosY+35, 319, ScreenLayout.RPMPosY+35);  // horizontal red line
        break;

      case ID_FR20:
        myGLCD.drawLine(0, ScreenLayout.RPMPosY+35, CarProfile[ID_FR20].RPM-1, ScreenLayout.RPMPosY+35);  // horizontal green line
        myGLCD.drawLine(0, ScreenLayout.RPMPosY+24, 0, ScreenLayout.RPMPosY+34);      // 0 rmp mark
        myGLCD.drawLine(84, ScreenLayout.RPMPosY+28, 84, ScreenLayout.RPMPosY+34);    // 2000 rpm mark
        myGLCD.drawLine(168, ScreenLayout.RPMPosY+28, 168, ScreenLayout.RPMPosY+34);  // 4000 rpm mark
        myGLCD.drawLine(253, ScreenLayout.RPMPosY+28, 253, ScreenLayout.RPMPosY+34);  // 6000 rpm mark
        myGLCD.print("10", 34, ScreenLayout.RPMPosY+22);         // 1000 rpm mark -8 pixel
        myGLCD.print("30", 118, ScreenLayout.RPMPosY+22);        // 3000 rpm mark -8 pixel
        myGLCD.print("50", 202, ScreenLayout.RPMPosY+22);        // 5000 rpm mark -8 pixel
        myGLCD.print("70", 286, ScreenLayout.RPMPosY+22);        // 7000 rpm mark -8 pixel
        myGLCD.setColor(wc_r, wc_g, wc_b);
        myGLCD.drawLine(CarProfile[ID_FR20].RPM, ScreenLayout.RPMPosY+35, 319, ScreenLayout.RPMPosY+35);  // horizontal red line
    }
  }

  // draw background lines
  myGLCD.setColor(bc_r, bc_g, bc_b);
  myGLCD.drawLine(0, 143, 319, 143);
  myGLCD.drawLine(0, 203, 319, 203);
  myGLCD.drawRoundRect(270, 74, 319, 130);

  // draw car name
  myGLCD.setFont(SmallFont);
  myGLCD.setColor(dc_r, dc_g, dc_b);
  myGLCD.print(CarProfile[ID].CarName, 271, 227);
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
    if (Filtered != 0) myGLCD.drawBitmap(ScreenLayout.EngineWarningsPosX +0, ScreenLayout.EngineWarningsPosY, 32, 32, fuelpressure_nok);
    else myGLCD.drawBitmap(ScreenLayout.EngineWarningsPosX +0, ScreenLayout.EngineWarningsPosY, 32, 32, fuelpressure_ok);
  }

  // draw oil pressure light
  // check only the bit which contains this warning light
  Filtered = Warning & 0x04;
  FilteredPrev = WarningPrev & 0x04;
  if (Filtered != FilteredPrev)
  {
    if (Filtered != 0) myGLCD.drawBitmap(ScreenLayout.EngineWarningsPosX +32, ScreenLayout.EngineWarningsPosY, 32, 32, oilpressure_nok);
    else myGLCD.drawBitmap(ScreenLayout.EngineWarningsPosX +32, ScreenLayout.EngineWarningsPosY, 32, 32, oilpressure_ok);
  }

  // draw water temp light
  // check only the bit which contains this warning light
  Filtered = Warning & 0x01;
  FilteredPrev = WarningPrev & 0x01;
  if (Filtered != FilteredPrev)
  {
    if (Filtered != 0) myGLCD.drawBitmap(ScreenLayout.EngineWarningsPosX +64, ScreenLayout.EngineWarningsPosY, 32, 32, water_nok);
    else myGLCD.drawBitmap(ScreenLayout.EngineWarningsPosX +64, ScreenLayout.EngineWarningsPosY, 32, 32, water_ok);
  }

  // draw pit speed limiter light
  // check only the bit which contains this warning light
  Filtered = Warning & 0x10;
  FilteredPrev = WarningPrev & 0x10;
  if (Filtered != FilteredPrev)
  {
    if (Filtered != 0) myGLCD.drawBitmap(ScreenLayout.EngineWarningsPosX +112, ScreenLayout.EngineWarningsPosY, 32, 32, pitspeedlimiter_on);
    else myGLCD.drawBitmap(ScreenLayout.EngineWarningsPosX +112, ScreenLayout.EngineWarningsPosY, 32, 32, pitspeedlimiter_off);
  }

  // draw stall sign light
  // check only the bit which contains this warning light
  Filtered = Warning & 0x08;
  FilteredPrev = WarningPrev & 0x08;
  if (Filtered != FilteredPrev)
  {
    if (Filtered != 0) myGLCD.drawBitmap(ScreenLayout.EngineWarningsPosX +144, ScreenLayout.EngineWarningsPosY, 32, 32, stall_on);
    else myGLCD.drawBitmap(ScreenLayout.EngineWarningsPosX +144, ScreenLayout.EngineWarningsPosY, 32, 32, stall_off);
  }
}

// draw fuel gauge
void DrawFuel(byte ID, int Fuel, int FuelPrev)
{
  myGLCD.setFont(BigFont);
  if (Fuel <= CarProfile[ID].Fuel) myGLCD.setColor(wc_r, wc_g, wc_b);
  else myGLCD.setColor(dc_r, dc_g, dc_b);
  
  myGLCD.print(".", ScreenLayout.FuelPosX+128, ScreenLayout.FuelPosY);
  myGLCD.printNumI(Fuel % 10, ScreenLayout.FuelPosX+144, ScreenLayout.FuelPosY);
                  
  // draw the integer value right justified
  if (Fuel < 100)
  {
    myGLCD.printNumI(Fuel / 10, ScreenLayout.FuelPosX+112, ScreenLayout.FuelPosY);
    if (FuelPrev >= 100)  // previous value was greater than 9, so we have to clear the above 10 value from the screen
    {
      myGLCD.setColor(0, 0, 0);
      myGLCD.fillRect(ScreenLayout.FuelPosX+80, ScreenLayout.FuelPosY, ScreenLayout.FuelPosX+112, ScreenLayout.FuelPosY+16);
    }
  }
  else if (Fuel < 1000)
  {
    myGLCD.printNumI(Fuel / 10, ScreenLayout.FuelPosX+96, ScreenLayout.FuelPosY);
    if (FuelPrev >= 1000)  // previous value was greater than 99, so we have to clear the above 100 value from the screen
    {
      myGLCD.setColor(0, 0, 0);
      myGLCD.fillRect(ScreenLayout.FuelPosX+80, ScreenLayout.FuelPosY, ScreenLayout.FuelPosX+96, ScreenLayout.FuelPosY+16);
    }
  }
  else if (Fuel < 10000) myGLCD.printNumI(Fuel / 10, ScreenLayout.FuelPosX+80, ScreenLayout.FuelPosY);
}

// draw gear number
void DrawGear(byte ID, char Gear)
{
  myGLCD.setFont(SevenSegNumFont);  // 32x50
  if (InData->Gear <= -1)  // reverse gear
  {
    myGLCD.setColor(wc_r, wc_g, wc_b);
    myGLCD.print("1", ScreenLayout.GearPosX, ScreenLayout.GearPosY);
  }
  else
  {
    myGLCD.setColor(dc_r, dc_g, dc_b);
    if (Gear>=0 and Gear<10) myGLCD.printNumI(Gear, ScreenLayout.GearPosX, ScreenLayout.GearPosY);
  }
}

// draw water temperature gauge
void DrawWaterTemp(byte ID, int WaterTemp, int WaterTempPrev)
{
  myGLCD.setFont(BigFont);
  if (WaterTemp >= CarProfile[ID].WaterTemp) myGLCD.setColor(wc_r, wc_g, wc_b);
  else myGLCD.setColor(dc_r, dc_g, dc_b);
                  
  if (WaterTemp < 10 && WaterTemp > 0)
  {
    myGLCD.printNumI(WaterTemp, ScreenLayout.WaterTempPosX+144, ScreenLayout.WaterTempPosY);
    if (WaterTempPrev >= 10)  // previous value was greater than 9, so we have to clear the above 10 value from the screen
    {
      myGLCD.setColor(0, 0, 0);
      myGLCD.fillRect(ScreenLayout.WaterTempPosX+112, ScreenLayout.WaterTempPosY, ScreenLayout.WaterTempPosX+144, ScreenLayout.WaterTempPosY+16);
    }
  }
  else if (WaterTemp < 100 && WaterTemp > 0)
       {
         myGLCD.printNumI(WaterTemp, ScreenLayout.WaterTempPosX+128, ScreenLayout.WaterTempPosY);
         if (WaterTempPrev >= 100)  // previous value was greater than 99, so we have to clear the above 100 value from the screen
         {
           myGLCD.setColor(0, 0, 0);
           myGLCD.fillRect(ScreenLayout.WaterTempPosX+112, ScreenLayout.WaterTempPosY, ScreenLayout.WaterTempPosX+128, ScreenLayout.WaterTempPosY+16);
         }                    
       }
       else if (WaterTemp<1000 && WaterTemp>0) myGLCD.printNumI(WaterTemp, ScreenLayout.WaterTempPosX+112, ScreenLayout.WaterTempPosY);
}

// draw RPM gauge
void DrawRPM(byte ID, int RPM, int RPMPrev)
{
  // RPM is bigger than the previous
  if (RPMPrev < RPM)
  {
    if (RPM >= CarProfile[ID].RPM)  // RPM is bigger than the warning limit
    {
      if (RPMPrev < CarProfile[ID].RPM)
      {
        // we have to draw both color on the RPM gauge
        myGLCD.setColor(dc_r, dc_g, dc_b);
        myGLCD.fillRect(RPMPrev, ScreenLayout.RPMPosY, CarProfile[ID].RPM-1, ScreenLayout.RPMPosY+20);
        myGLCD.setColor(wc_r, wc_g, wc_b);
        myGLCD.fillRect(CarProfile[ID].RPM, ScreenLayout.RPMPosY, RPM, ScreenLayout.RPMPosY+20);
      }
      else
      {
        // only the warning color have to be used
        myGLCD.setColor(wc_r, wc_g, wc_b);
        myGLCD.fillRect(RPMPrev, ScreenLayout.RPMPosY, RPM, ScreenLayout.RPMPosY+20);
      }
    }
    else
    {
      // RPM is not bigger than the warning limit, only the default color have to be used
      myGLCD.setColor(dc_r, dc_g, dc_b);
      myGLCD.fillRect(RPMPrev, ScreenLayout.RPMPosY, RPM, ScreenLayout.RPMPosY+20);
    }
  }

  // RPM is lower than the previous
  if (RPMPrev > RPM)
  {
    myGLCD.setColor(0, 0, 0);
    myGLCD.fillRect(RPMPrev, ScreenLayout.RPMPosY, RPM, ScreenLayout.RPMPosY+20);
  }
}

// draw speed gauge
void DrawSpeed(byte ID, int Speed, int SpeedPrev)
{
  myGLCD.setFont(BigFont);
  myGLCD.setColor(dc_r, dc_g, dc_b);
  if (Speed<10)
  {
    myGLCD.printNumI(Speed, ScreenLayout.SpeedPosX+144, ScreenLayout.SpeedPosY);
    if (SpeedPrev>=10)
    {
      myGLCD.setColor(0, 0, 0);
      myGLCD.fillRect(ScreenLayout.SpeedPosX+112, ScreenLayout.SpeedPosY, ScreenLayout.SpeedPosX+144, ScreenLayout.SpeedPosY+16);
    }
  }
  else if (Speed<100)
       {
         myGLCD.printNumI(Speed, ScreenLayout.SpeedPosX+128, ScreenLayout.SpeedPosY);
         if (SpeedPrev>=100)
         {
           myGLCD.setColor(0, 0, 0);
           myGLCD.fillRect(ScreenLayout.SpeedPosX+112, ScreenLayout.SpeedPosY, ScreenLayout.SpeedPosX+128, ScreenLayout.SpeedPosY+16);
         }
       }
  else if (Speed<1000) myGLCD.printNumI(Speed, ScreenLayout.SpeedPosX+112, ScreenLayout.SpeedPosY);
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
        case 0: myGLCD.fillRect(10, ScreenLayout.SLIPosY, 35, ScreenLayout.SLIPosY+16);
                break;
        case 1: myGLCD.fillRect(50, ScreenLayout.SLIPosY, 75, ScreenLayout.SLIPosY+16);
                break;
        case 2: myGLCD.fillRect(90, ScreenLayout.SLIPosY, 115, ScreenLayout.SLIPosY+16);
                break;
        case 3: myGLCD.fillRect(130, ScreenLayout.SLIPosY, 155, ScreenLayout.SLIPosY+16);
                break;
        case 4: myGLCD.fillRect(170, ScreenLayout.SLIPosY, 195, ScreenLayout.SLIPosY+16);
                break;
        case 5: myGLCD.fillRect(210, ScreenLayout.SLIPosY, 235, ScreenLayout.SLIPosY+16);
                break;
        case 6: myGLCD.fillRect(250, ScreenLayout.SLIPosY, 275, ScreenLayout.SLIPosY+16);
                break;
        case 7: myGLCD.fillRect(290, ScreenLayout.SLIPosY, 315, ScreenLayout.SLIPosY+16);
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
                myGLCD.fillRect(10, ScreenLayout.SLIPosY, 35, ScreenLayout.SLIPosY+16);
                break;
        case 2: myGLCD.setColor(dc_r, dc_g, dc_b);
                myGLCD.fillRect(50, ScreenLayout.SLIPosY, 75, ScreenLayout.SLIPosY+16);
                break;
        case 3: myGLCD.setColor(dc_r, dc_g, dc_b);
                myGLCD.fillRect(90, ScreenLayout.SLIPosY, 115, ScreenLayout.SLIPosY+16);
                break;
        case 4: myGLCD.setColor(dc_r, dc_g, dc_b);
                myGLCD.fillRect(130, ScreenLayout.SLIPosY, 155, ScreenLayout.SLIPosY+16);
                break;
        case 5: myGLCD.setColor(mc_r, mc_g, mc_b);
                myGLCD.fillRect(170, ScreenLayout.SLIPosY, 195, ScreenLayout.SLIPosY+16);
                break;
        case 6: myGLCD.setColor(mc_r, mc_g, mc_b);
                myGLCD.fillRect(210, ScreenLayout.SLIPosY, 235, ScreenLayout.SLIPosY+16);
                break;
        case 7: myGLCD.setColor(wc_r, wc_g, wc_b);
                myGLCD.fillRect(250, ScreenLayout.SLIPosY, 275, ScreenLayout.SLIPosY+16);
                break;
        case 8: myGLCD.setColor(wc_r, wc_g, wc_b);
                myGLCD.fillRect(290, ScreenLayout.SLIPosY, 315, ScreenLayout.SLIPosY+16);
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

  UploadProfiles();
  DrawBackground(DefaultCar);  // draw the default screen layout
  ActiveCar = DefaultCar;
}

void loop()
{
  int rpm_int, pressed_button;
  int x, y; // position of the screen touch

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
                if (Screen[0].EngineWarnings != Screen[1].EngineWarnings && ScreenLayout.ShowEngineWarnings == true) DrawEngineWarnings(ActiveCar, Screen[1].EngineWarnings, Screen[0].EngineWarnings);

                // draw RPM gauge
                Screen[1].RPMgauge = (int)(InData->RPM / CarProfile[ActiveCar].RPMscale);
                if (Screen[1].RPMgauge > 319) Screen[1].RPMgauge = 319;  // limit RPM gauge to maximum display width
                if (Screen[0].RPMgauge != Screen[1].RPMgauge && ScreenLayout.ShowRPM == true) DrawRPM(ActiveCar, Screen[1].RPMgauge, Screen[0].RPMgauge);

                // draw gear number
                Screen[1].Gear = InData->Gear;
                if (Screen[0].Gear != Screen[1].Gear && ScreenLayout.ShowGear == true) DrawGear(ActiveCar, Screen[1].Gear);

                // draw fuel level gauge
                Screen[1].Fuel = (int)(InData->Fuel*10); // convert float data to int and keep the first digit of the fractional part also
                if (Screen[0].Fuel != Screen[1].Fuel && ScreenLayout.ShowFuel == true) DrawFuel(ActiveCar, Screen[1].Fuel, Screen[0].Fuel);

                // draw speed gauge
                Screen[1].Speed = (int)(InData->Speed*3.6);  // convert m/s to km/h
                if (Screen[0].Speed != Screen[1].Speed && ScreenLayout.ShowSpeed == true) DrawSpeed(ActiveCar, Screen[1].Speed, Screen[0].Speed);

                // draw water temperature gauge
                Screen[1].WaterTemp = (int)InData->WaterTemp;
                if (Screen[0].WaterTemp != Screen[1].WaterTemp && ScreenLayout.ShowWaterTemp == true) DrawWaterTemp(ActiveCar, Screen[1].WaterTemp, Screen[0].WaterTemp);
                
                // draw shift light indicator
                rpm_int = (int)InData->RPM;
                if (rpm_int <= CarProfile[ActiveCar].SLI[0]) Screen[1].SLI = 0; // determine how many light to be activated for the current gear
                else
                {
                  if (rpm_int > CarProfile[ActiveCar].SLI[7]) Screen[1].SLI = 8;
                  else if (rpm_int > CarProfile[ActiveCar].SLI[6]) Screen[1].SLI = 7;
                       else if (rpm_int > CarProfile[ActiveCar].SLI[5]) Screen[1].SLI = 6;
                            else if (rpm_int > CarProfile[ActiveCar].SLI[4]) Screen[1].SLI = 5;
                                 else if (rpm_int > CarProfile[ActiveCar].SLI[3]) Screen[1].SLI = 4;
                                      else if (rpm_int > CarProfile[ActiveCar].SLI[2]) Screen[1].SLI = 3;
                                           else if (rpm_int > CarProfile[ActiveCar].SLI[1]) Screen[1].SLI = 2;
                                                else if (rpm_int > CarProfile[ActiveCar].SLI[0]) Screen[1].SLI = 1;
                }
                if (Screen[0].SLI != Screen[1].SLI && ScreenLayout.ShowSLI == true) DrawSLI(ActiveCar, Screen[1].SLI, Screen[0].SLI);

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

