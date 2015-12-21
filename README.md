# iRDash_Client
Displays live data of iRacing telemetry using an Arduino board plus a TFT display.

The purpose of this program is to display iRacing's live telemetry data on an Arduino TFT display. As of now it can show RPM, a four segment Shift Light Indicator, Gear, Fuel quantity, Water temperature and Speed.
It supports multiple profiles to suit the different cars available in the sim. You can choose between them if you touch the middle 1/3 of the display. Currently it has profile for the Skippy, Cadillac CTS-V and MX-5.

The program is developed on an Arduino Mega 2560 and a 320x240 resolution touch sensitive TFT display. If you have different Arduino board and / or a different display then you need to adjust the screen initialization in the UTFT library accordingly.
The screen layout is optimized for the 320x240 resolution but it is relatively easy to adjust it for other resolutions.

To display the live data of iRacing it needs the "iRDash Server" program running on Windows host and connected to the Arduino board via USB.

To compile the program you need the below libraries from http://www.rinkydinkelectronics.com/: UTFT, UTouch, UTFT_Buttons