#include <UTFT.h>
#include <UTouch.h>
#include <UTFT_Buttons.h>

#define DefaultCar 0    // default car profile (numbering starts with 0)
#define NumOfCars 3     // number of car profiles
#define ScreenWidth 319 // width of the display in pixels -1

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

// structure of the incoming serial data block
struct SIncomingData
{
  //char EngineWarnings;
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
  //char EngineWarnings;
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
  int Fuel;       // value in liter * 10
  int RPM;        // value where the redline starts in pixels
  int WaterTemp;  // value in Celsius
  int SLI[4];     // RPM values for each SLI light (in RPM)
};

char inByte;                  // incoming byte from the serial port
int blockpos;                 // position of the actual incoming byte in the telemetry data block
SIncomingData* InData;        // pointer to access the telemetry data as a structure
char* pInData;                // pointer to access the telemetry data as a byte array
SViewData Screen[2];          // store the actual and the previous screen data
int buttons[NumOfCars];       // handle for the touch buttons
char ActiveCar;               // active car profile

// variables to manage screen layout
SScreenLayout ViewLayout[NumOfCars];  // store screen layout for each car
SWarnings ViewWarning[NumOfCars];     // store warning limits for each car

// clear our internal data block
void ResetInternalData()
{
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
  // 0: Skippy
  ViewLayout[0].CarName[0] = 'S';
  ViewLayout[0].CarName[1] = 'k';
  ViewLayout[0].CarName[2] = 'i';
  ViewLayout[0].CarName[3] = 'p';
  ViewLayout[0].CarName[4] = 'p';
  ViewLayout[0].CarName[5] = 'y';
  ViewLayout[0].CarName[6] = 0;
  ViewLayout[0].FuelPosX = 0;
  ViewLayout[0].FuelPosY = 150;
  ViewLayout[0].ShowFuel = true;
  ViewLayout[0].GearPosX = 280;
  ViewLayout[0].GearPosY = 65;
  ViewLayout[0].ShowGear = true;
  ViewLayout[0].RPMPosY = 25;
  ViewLayout[0].RPMscale = 20.625;  // 6600 / 320
  ViewLayout[0].ShowRPM = true;
  ViewLayout[0].SLIPosY = 0;
  ViewLayout[0].ShowSLI = true;
  ViewLayout[0].SpeedPosX = 0;
  ViewLayout[0].SpeedPosY = 75;
  ViewLayout[0].ShowSpeed = true;
  ViewLayout[0].WaterTempPosX = 0;
  ViewLayout[0].WaterTempPosY = 190;
  ViewLayout[0].ShowWaterTemp = true;

  ViewWarning[0].Fuel = 25;
  ViewWarning[0].RPM = 291;     // 6000 / RPMscale
  ViewWarning[0].SLI[0] = 4900;
  ViewWarning[0].SLI[1] = 5400;
  ViewWarning[0].SLI[2] = 5800;
  ViewWarning[0].SLI[3] = 6100;
  ViewWarning[0].WaterTemp = 90;

  // use the formula to determine button outline: (x*80)+10, (y*54)+30, 60, 36)
  // x and y is the position in the 4x4 matrix
  buttons[0] = myButtons.addButton( 10,  30, 60,  36, ViewLayout[0].CarName); // x = 0; y = 0

  // 1: CTS-V
  ViewLayout[1].CarName[0] = 'C';
  ViewLayout[1].CarName[1] = 'T';
  ViewLayout[1].CarName[2] = 'S';
  ViewLayout[1].CarName[3] = '-';
  ViewLayout[1].CarName[4] = 'V';
  ViewLayout[1].CarName[5] = 0;
  ViewLayout[1].CarName[6] = 0;
  ViewLayout[1].FuelPosX = 0;
  ViewLayout[1].FuelPosY = 150;
  ViewLayout[1].ShowFuel = true;
  ViewLayout[1].GearPosX = 280;
  ViewLayout[1].GearPosY = 65;
  ViewLayout[1].ShowGear = true;
  ViewLayout[1].RPMPosY = 25;
  ViewLayout[1].RPMscale = 25;  // 8000 / 320
  ViewLayout[1].ShowRPM = true;
  ViewLayout[1].SLIPosY = 0;
  ViewLayout[1].ShowSLI = true;
  ViewLayout[1].SpeedPosX = 0;
  ViewLayout[1].SpeedPosY = 75;
  ViewLayout[1].ShowSpeed = true;
  ViewLayout[1].WaterTempPosX = 0;
  ViewLayout[1].WaterTempPosY = 190;
  ViewLayout[1].ShowWaterTemp = true;

  ViewWarning[1].Fuel = 80;
  ViewWarning[1].RPM = 288;     // 7200 / RPMscale
  ViewWarning[1].SLI[0] = 5600;
  ViewWarning[1].SLI[1] = 6200;
  ViewWarning[1].SLI[2] = 6800;
  ViewWarning[1].SLI[3] = 7200;
  ViewWarning[1].WaterTemp = 110;

  // use the formula to determine button outline: (x*80)+10, (y*54)+30, 60, 36)
  // x and y is the position in the 4x4 matrix
  buttons[1] = myButtons.addButton( 90,  30, 60,  36, ViewLayout[1].CarName); // x = 1; y = 0

  // 2: MX-5
  ViewLayout[2].CarName[0] = 'M';
  ViewLayout[2].CarName[1] = 'X';
  ViewLayout[2].CarName[2] = '-';
  ViewLayout[2].CarName[3] = '5';
  ViewLayout[2].CarName[4] = 0;
  ViewLayout[2].CarName[5] = 0;
  ViewLayout[2].CarName[6] = 0;
  ViewLayout[2].FuelPosX = 0;
  ViewLayout[2].FuelPosY = 150;
  ViewLayout[2].ShowFuel = true;
  ViewLayout[2].GearPosX = 280;
  ViewLayout[2].GearPosY = 65;
  ViewLayout[2].ShowGear = true;
  ViewLayout[2].RPMPosY = 25;
  ViewLayout[2].RPMscale = 22.8125;  // 7300 / 320
  ViewLayout[2].ShowRPM = true;
  ViewLayout[2].SLIPosY = 0;
  ViewLayout[2].ShowSLI = true;
  ViewLayout[2].SpeedPosX = 0;
  ViewLayout[2].SpeedPosY = 75;
  ViewLayout[2].ShowSpeed = true;
  ViewLayout[2].WaterTempPosX = 0;
  ViewLayout[2].WaterTempPosY = 190;
  ViewLayout[2].ShowWaterTemp = true;

  ViewWarning[2].Fuel = 40;
  ViewWarning[2].RPM = 296;     // 6750 / RPMscale
  ViewWarning[2].SLI[0] = 6000;
  ViewWarning[2].SLI[1] = 6250;
  ViewWarning[2].SLI[2] = 6500;
  ViewWarning[2].SLI[3] = 6750;
  ViewWarning[2].WaterTemp = 100;

  // use the formula to determine button outline: (x*80)+10, (y*54)+30, 60, 36)
  // x and y is the position in the 4x4 matrix
  buttons[2] = myButtons.addButton( 170,  30, 60,  36, ViewLayout[2].CarName);  // x = 2; y = 0
}

// draw the background for the selected car, draw only the active instruments
void DrawBackground(char ID)
{
  myGLCD.clrScr();
  myGLCD.setFont(BigFont);
  myGLCD.setColor(dc_r, dc_g, dc_b);

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
      case 0:
        // Skippy
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

      case 1:
        // CTS-V
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

      case 2:
        // MX-5
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

// draw fuel gauge
void DrawFuel(char ID, int Fuel, int FuelPrev)
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
void DrawGear(char ID, char Gear)
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
void DrawWaterTemp(char ID, int WaterTemp, int WaterTempPrev)
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
void DrawRPM(char ID, int RPM, int RPMPrev)
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
void DrawSpeed(char ID, int Speed, int SpeedPrev)
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
void DrawSLI(char ID, int SLI, int SLIPrev)
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
  pInData = (char*)InData;     // set the byte array pointer to the telemetry data
  ResetInternalData();

  UploadCarProfiles();
  DrawBackground(DefaultCar);  // draw the default screen layout
  ActiveCar = DefaultCar;
}

void loop()
{
  int temp_int, pressed_button;
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

                // draw RPM gauge
                Screen[1].RPMgauge = (int)(InData->RPM / ViewLayout[ActiveCar].RPMscale);
                if (Screen[1].RPMgauge > ScreenWidth) Screen[1].RPMgauge = ScreenWidth;  // limit RPM gauge to maximum display width
                if (Screen[0].RPMgauge != Screen[1].RPMgauge && ViewLayout[ActiveCar].ShowRPM == true) DrawRPM(ActiveCar, Screen[1].RPMgauge, Screen[0].RPMgauge);

                // draw gear number
                if (Screen[0].Gear != InData->Gear && ViewLayout[ActiveCar].ShowGear == true) DrawGear(ActiveCar, InData->Gear);

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
                temp_int = (int)InData->RPM;
                if (temp_int <= ViewWarning[ActiveCar].SLI[0]) Screen[1].SLI = 0; // determine how many light is activated
                else
                {
                  if (temp_int > ViewWarning[ActiveCar].SLI[3]) Screen[1].SLI = 4;
                  else if (temp_int > ViewWarning[ActiveCar].SLI[2]) Screen[1].SLI = 3;
                       else if (temp_int > ViewWarning[ActiveCar].SLI[1]) Screen[1].SLI = 2;
                            else if (temp_int > ViewWarning[ActiveCar].SLI[0]) Screen[1].SLI = 1;
                }
                if (Screen[0].SLI != Screen[1].SLI && ViewLayout[ActiveCar].ShowSLI == true) DrawSLI(ActiveCar, Screen[1].SLI, Screen[0].SLI);

                // update old screen data
                Screen[0].Fuel      = Screen[1].Fuel;
                Screen[0].Gear      = InData->Gear;
                Screen[0].RPMgauge  = Screen[1].RPMgauge;
                Screen[0].Speed     = Screen[1].Speed;
                Screen[0].WaterTemp = Screen[1].WaterTemp;
                Screen[0].SLI       = Screen[1].SLI;
  
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

