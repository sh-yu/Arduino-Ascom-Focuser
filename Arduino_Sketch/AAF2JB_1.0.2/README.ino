
//------------------------------------------------------------------
// Ascom-Arduino Focuser
// Dave Wells
// Thanks for code snippets & inspiration:
//  o  Gina (Stargazers Lounge) for the stepper control basics
//  o  russellhq  (Stargazers Lounge) for the 1-wire code and info.
//------------------------------------------------------------------

//------ Change Log ----------------------------------------------------------------------
//  Version    Date        Change
//  0.0.2      26/05/2014  Initial - copied from Windows version
//  2.0.0      22/06/2014  Renumbered to match Ascom Driver Numbering
//  2.0.1      09/08/2014  Initial position set to 1000
//  2.0.2      16/08/2014  Halt function implemented with H# command
//                         New I# command to set an initial position
//
//  version numbering brought into line with driver numbering
//  2.2.0      03/10/2014  Implemented Temperature Sensing C# command
//
//  2.3.0      07/10/2014  Now stores position in EEPROM
//                         Changed some int variables to unsigned int
//                         Fixed minor bug in motor hi/lo speed detection
//                         Temperature now in 100ths of degree for internationlisation
//                         Block temperature requests during focuser movement
//
// 2.4.0       18/04/2015  Amended to step in single half-steps not 8 per unit
//                         Moved user-configurable parameters to a single block at top of code
//                         Reduced number of read/write operations to/from EEPROM
//                         Reduced no of calls to clear the output pins in loop()
//                         Some general code and structure tidying
//
// 2.5.0       24/06/2015  Changes to integrate Bluetooth Comms using HC05.
//                         Some refactoring, renaming, rearranging of code
//
// 1.0.0JB     20/10/2016  Changed name and version number to avoid confusion with original AAF2.
//                         Name will be AAF2JB and version starts at 1.0.0JB
//                         Removed the #define BLUETOOTH_COMMS method to activate BT since makes 
//                         imposible to activate and deactivate the BT radio without editing the code.
//                         Two buttons added (Pins 11 & 12). One to allow turning BT on/off and the 
//                         other to perform BT configuration.
//                         Functions were moved to different tabs depending on their purpose.
//                         Multiple code modifications.
//
// 1.0.1       15/04/2018  Switched from multiple board system (arduino Nano+HC05+ULN2003)to 
//                         a JD_IOBoard V1.0 which include an Atmega+ULN2003 in the size of a 
//                         Arduino pro mini droping BT functionality and its buttons. 
//
// 1.0.2       16/04/2018  Replaced all #define with const
//                         
//                          
//----------------------------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
//  Description: 
//
//  jD-IOBoard V1.0 pinouts
//
//             D D D G       R T R
//         5 5 1 1 1 N D D D X X S
//         V V 3 1 2 D 7 6 5 1 1 T
//         | | | | | | | | | | | |
//      +----------------------------+
//      |O O O O O O O O O O O O O   |
// O1 - |O O   S M M                O| _ DTS 
// O2 - |O O   C O I                O| - RX  F
// O3 - |O O   K S S       S S      O| - TX  T
// O4 - |O O     I O       D C      O| - 5V  D
// O5 - |O O               A L      O| _ CTS I
// O6 - |O O O O O O O O   O O O O  O| - GND
//      +----------------------------+
//       |   | | | | | |   | | | |
//       C   G 5 A A A A   A A 5 G
//       O   N V 0 1 2 3   4 5 V N
//       M   D                   D
//
//
/******************************************************************************/

//------------------------------------------------------------------
// Functions moved
//------------------------------------------------------------------
// ****void HC05Config()                    | Bluetooth

// **** void serialEvent()                  | Serial
// **** void BTSerialEvent()                |

// **** void doMovement()                   |
// **** void anticlockwise()                |
// **** void clockwise()                    |
// **** void setOutput(int out)             | Motor
// **** void clearOutput()                  |
// **** void savePosition(unsigned int p)   |
// **** unsigned int restorePosition(void)  |
// **** boolean storedPositionValid(void)   |

// **** double getTemperature()             | Temperature
