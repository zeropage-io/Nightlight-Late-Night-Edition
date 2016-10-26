//
// Stormtrooper Nightlight v1.2
// Copyright (C) 2016 Axel Dietrich <foobar@zeropage.io>
// http://zeropage.io
//
// An Arduino controlled Stormtrooper nightlight with WS2812b RGB LEDs,
// touch sensor switches, a potpourri of nice lighting effects and a
// quite ugly piezo Star Wars(R) intro.
//
// This program is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation, either version 3 of the License, or (at your option)
// any later version. This program is distributed in the hope that it will be
// useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
// Public License for more details. You should have received a copy of the GNU
// General Public License along with this program (file LICENSE). If not, see
// <http://www.gnu.org/licenses/>.
//
// History
// 1.2, 13.09.2016
//   - eSTROBE: Using EVERY_N_MILLISECONDS macro did not work. Rolled my own.
//   - Very long touching mode button did not work. Used wrong variable names. Fixed.
//   - eBREATH mode looks ugly on lower brightness settings. Changed amplitude.
// 1.1, 12.09.2016
//   - Usability: increase sensor 2nd function waiting time. one second is too short and might induce unwanted triggers.
//   - Usability: add 4 sec long touch (3rd function) for mode sensor to quickly jump to first mode.
//   - Removed unnecessary code from eBREATHE mode.
// 1.0, 11.09.2016
//   - Initial release.
//
#include <FastLED.h>            // http://fastled.io/
#include <Streaming.h>          // http://arduiniana.org/libraries/streaming/
#include <ADCTouch.h>           // http://playground.arduino.cc/Code/ADCTouch
#include <EEPROM.h>

#define LED_DATA_PIN                                    10
#define LED_NUM_LEDS                                     6
#define LED_BRIGHTNESS_STEP_SIZE                        15
#define LED_COLOR_STEP_SIZE                              5
#define PIEZO_PIN                                       12
#define TSENSOR_BRIGHTNESS_PIN                          A0
#define TSENSOR_MODE_PIN                                A2
#define TSENSOR_BRIGHTNESS_2ND_FUNCTION_WAITING_TIME  1200 // in milliseconds
#define TSENSOR_BRIGHTNESS_3RD_FUNCTION_WAITING_TIME  3000 // in milliseconds
#define TSENSOR_MODE_2ND_FUNCTION_WAITING_TIME        1200 // in milliseconds
#define TSENSOR_MODE_3RD_FUNCTION_WAITING_TIME        3000 // in milliseconds
#define EEPROM_ADR_PLAY_IMPERIAL_MARCH                   0
#define EEPROM_ADR_MASTER_BRIGHTNESS                     1
#define EEPROM_ADR_MODE                                  2
#define EEPROM_ADR_FIXED_COLOR_INDEX                     3
#define EEPROM_ADR_TWINKLES_PALETTE                      4
#define EEPROM_ADR_STROBE_SPEED                          5
#define EEPROM_ADR_RAINBOW_DELTA_HUE                     6
#define EEPROM_ADR_RAINBOW_GLITTER_DELTA_HUE             7
#define EEPROM_ADR_CONFETTI_CHANCE                       8
#define EEPROM_ADR_BEATS_PER_MINUTE                      9
#define EEPROM_ADR_BREATHE_COLOR_INDEX                  10
#define EEPROM_ADR_CYCLONE_COLOR_INDEX                  11
#define IMPERIAL_MARCH_SPEED                           2.5 // higher value = play faster
#define ARRAY_SIZE(A)         (sizeof(A) / sizeof((A)[0]))
#define DEBUG                                            0
CRGB gLED[ LED_NUM_LEDS ];
CRGBPalette16 myPal = RainbowColors_p;
enum          gModes            { eSTART_MODE, eFIXED_COLOR, eCOLOR_TWINKLES, eSTROBE, eRAINBOW, eRAINBOW_WITH_GLITTER, eCONFETTI, eBPM, eBREATHE, eCYCLONE, eEND_MODE };
enum          gPiezoModes       { eBRIGHTNESS_PIEZO, eMODE_PIEZO, e3RD_FUNC_PIEZO };
uint16_t      gTouchBrightnessRef;
uint16_t      gTouchModeRef;
bool          gPlayImperialMarch       = true;
uint8_t       gMasterBrightness        = 120;
uint8_t       gCurrentMode             = eFIXED_COLOR;
uint8_t       gFixedColorIndex         = 0;
uint8_t       gTwinklesPalette         = 0;
uint8_t       gStrobeSpeed             = 100;
uint8_t       gRainbowDeltaHue         = 7;
uint8_t       gRainbowGlitterDeltaHue  = 7;
uint8_t       gConfettiChance          = 80;
uint8_t       gBeatsPerMinute          = 60;
uint8_t       gBreatheColorIndex       = 0;
uint8_t       gCycloneColorIndex       = 0;
uint8_t       gHue                     = 0;
bool          gPowerUp                 = true;

//
// forward declarations
void fPiezoShortTouch( byte mode );
void fPiezoLongTouch( byte mode );

void setup( ) {
  if ( DEBUG ) {
    delay( 500 );
    Serial.begin( 9600 );
    Serial.setTimeout( 200 ); // 200ms should be enough
  }
  randomSeed( analogRead( 1 ) );
  pinMode( PIEZO_PIN, OUTPUT );

  // Fetch data from EEPROM
  gPlayImperialMarch       = EEPROM.read( EEPROM_ADR_PLAY_IMPERIAL_MARCH );
  if(gPlayImperialMarch<0||gPlayImperialMarch>1)
    gPlayImperialMarch = true;
  gMasterBrightness        = EEPROM.read( EEPROM_ADR_MASTER_BRIGHTNESS );
  gCurrentMode             = EEPROM.read( EEPROM_ADR_MODE              );
  if ( gCurrentMode<(eSTART_MODE+1) || gCurrentMode>(eEND_MODE-1) )
    gCurrentMode = eSTART_MODE + 1;
  gFixedColorIndex         = EEPROM.read( EEPROM_ADR_FIXED_COLOR_INDEX         ); // for eFIXED_COLOR
  gTwinklesPalette         = EEPROM.read( EEPROM_ADR_TWINKLES_PALETTE          ); // for eCOLOR_TWINKLES
  gStrobeSpeed             = EEPROM.read( EEPROM_ADR_STROBE_SPEED              ); // for eSTROBE
  gRainbowDeltaHue         = EEPROM.read( EEPROM_ADR_RAINBOW_DELTA_HUE         ); // for eRAINBOW
  gRainbowGlitterDeltaHue  = EEPROM.read( EEPROM_ADR_RAINBOW_GLITTER_DELTA_HUE ); // for eRAINBOW_WITH_GLITTER
  gConfettiChance          = EEPROM.read( EEPROM_ADR_CONFETTI_CHANCE           ); // for eCONFETTI
  gBeatsPerMinute          = EEPROM.read( EEPROM_ADR_BEATS_PER_MINUTE          ); // for eBPM
  gBreatheColorIndex       = EEPROM.read( EEPROM_ADR_BREATHE_COLOR_INDEX       ); // for eBREATHE
  gCycloneColorIndex       = EEPROM.read( EEPROM_ADR_CYCLONE_COLOR_INDEX       ); // for eCYCLONE
  fChoosePalette( gTwinklesPalette );

  // FastLED setup
  FastLED.addLeds< WS2812B, LED_DATA_PIN, GRB >( gLED, LED_NUM_LEDS );
  FastLED.setCorrection( TypicalLEDStrip );
  FastLED.setBrightness( 0 );
  fill_solid( gLED, LED_NUM_LEDS, CRGB::Black );
  FastLED.show( );

  if ( gPlayImperialMarch )
    fPlayImperialMarch( );

  // Touch wire setup. Create reference values to account for the capacitance of control wires.
  gTouchBrightnessRef = ADCTouch.read( TSENSOR_BRIGHTNESS_PIN, 500 );
  gTouchModeRef       = ADCTouch.read( TSENSOR_MODE_PIN,       500 );

  // Welcome to the pleasure dome!
  if ( DEBUG ) {
    Serial << endl << endl << F( "Welcome to Stormtrooper Nightlight v1.0" ) << endl;
    Serial << F( "(C) 2016 Axel Dietrich <foobar@zeropage.io>" ) << endl;
    Serial << F( "Type h for a list of commands." ) << endl;
  }
}

void loop( ) {
  static uint16_t lastMillisBrightness        = millis( );
  static uint16_t lastMillisMode              = millis( );
  static bool     touchedBrightness           = false;
  static bool     touchedMode                 = false;
  static bool     mustReleaseBrightnessSensor = false;
  static bool     mustReleaseModeSensor       = false;
         bool     shortModeTouch              = false;

  // Check if touch sensors have been activated.
  // Get Touch values.
  int touchBrightnessVal = ADCTouch.read( TSENSOR_BRIGHTNESS_PIN );
  int touchModeVal       = ADCTouch.read( TSENSOR_MODE_PIN       );

  // Remove offset computed in setup()
  touchBrightnessVal -= gTouchBrightnessRef;
  touchModeVal       -= gTouchModeRef;

  // Touch too much?
  // Check the BRIGHTNESS touch sensor.
  // short touch     => increase brightness by LED_BRIGHTNESS_STEP_SIZE
  // long touch      => set brightness to lowest value (i.e. night mode)
  // very long touch => toggle playing imperial march on boot
  if ( touchBrightnessVal > 40 ) {
    if ( !touchedBrightness )
      touchedBrightness = true;
    // Check if touch sensor touched for at least TSENSOR_BRIGHTNESS_2ND_FUNCTION_WAITING_TIME milliseconds.
    uint16_t m = millis( );
    if ( !mustReleaseBrightnessSensor ) {
      if ( ( m - lastMillisBrightness ) > TSENSOR_BRIGHTNESS_2ND_FUNCTION_WAITING_TIME ) {
        // a long touch => set brightnes to lowest value
        lastMillisBrightness = m;
        gMasterBrightness = 10;
        FastLED.setBrightness( gMasterBrightness );
        FastLED.show( );
        EEPROM.write( EEPROM_ADR_MASTER_BRIGHTNESS, gMasterBrightness );
        fPiezoLongTouch( eBRIGHTNESS_PIEZO );
        mustReleaseBrightnessSensor = true;
      }
    } else if ( ( m - lastMillisBrightness ) > TSENSOR_BRIGHTNESS_3RD_FUNCTION_WAITING_TIME ) {
      // a very long touch => toggle playing imperial march
      lastMillisBrightness = m;
      EEPROM.write( EEPROM_ADR_PLAY_IMPERIAL_MARCH, ( gPlayImperialMarch ? 0 : 1 ) );
      fPiezoLongTouch( e3RD_FUNC_PIEZO );
    }
  } else {
    if ( touchedBrightness ) {
      // We get here when brightness touch sensor was touched and released.
      touchedBrightness = false;
      if ( mustReleaseBrightnessSensor ) {
        // ah, brightness touch sensor released after a long touch
        mustReleaseBrightnessSensor = false;
      } else {
        // a short touch => gradually increase brightness
        gMasterBrightness = ( (gMasterBrightness + LED_BRIGHTNESS_STEP_SIZE) > 255 ? 0 : (gMasterBrightness + LED_BRIGHTNESS_STEP_SIZE) );
        FastLED.setBrightness( gMasterBrightness );
        FastLED.show( );
        EEPROM.write( EEPROM_ADR_MASTER_BRIGHTNESS, gMasterBrightness );
        if ( gMasterBrightness == (255-LED_BRIGHTNESS_STEP_SIZE) )
          fPiezoShortTouch( eBRIGHTNESS_PIEZO );
        if ( gMasterBrightness == 255 )
          fPiezoShortTouch( eBRIGHTNESS_PIEZO ), fPiezoShortTouch( eBRIGHTNESS_PIEZO );
        fPiezoShortTouch( eBRIGHTNESS_PIEZO );
      }
    }
    lastMillisBrightness = millis( );
  }

  // Touch too much?
  // Check the MODE touch sensor.
  // short touch     => function depends on mode
  // long touch      => switch to next mode
  // very long touch => jump to the first mode
  if ( touchModeVal > 40 ) {
    if ( !touchedMode )
      touchedMode = true;
    // Check if touch sensor touched for at least TSENSOR_MODE_2ND_FUNCTION_WAITING_TIME milliseconds.
    uint16_t m = millis( );
    if ( !mustReleaseModeSensor  ) {
      if ( ( m - lastMillisMode ) > TSENSOR_MODE_2ND_FUNCTION_WAITING_TIME ) {
        // a long touch => switch to the next mode
        byte noOfModes = eEND_MODE - eSTART_MODE - 1;
        gCurrentMode = ( gCurrentMode<noOfModes ? gCurrentMode+1 : eSTART_MODE+1 );
        lastMillisMode = m;
        mustReleaseModeSensor = true;
        fPiezoLongTouch( eMODE_PIEZO );
      }
    } else if ( ( m - lastMillisMode ) > TSENSOR_MODE_3RD_FUNCTION_WAITING_TIME ) {
      // a very long touch => jump to the first mode
      lastMillisMode = m;
      gCurrentMode = eSTART_MODE + 1;
      fPiezoLongTouch( e3RD_FUNC_PIEZO );
    }
  } else {
    if ( touchedMode ) {
      // We get here when mode touch sensor was touched and released.
      touchedMode = false;
      if ( mustReleaseModeSensor  ) {
        // ah, mode touch sensor released after a long touch
        mustReleaseModeSensor = false;
        EEPROM.write( EEPROM_ADR_MODE, gCurrentMode );
      } else {
        // a short touch => depends on mode what happens
        fPiezoShortTouch( eMODE_PIEZO );
        shortModeTouch = true;
      }
    }
    lastMillisMode = millis( );
  }

  switch ( gCurrentMode ) {
    case eFIXED_COLOR: {
        static bool firstTimeAfterBoot = true;
        if ( shortModeTouch ) {
          gFixedColorIndex = ( (gFixedColorIndex + LED_COLOR_STEP_SIZE) > 255 ? 0 : (gFixedColorIndex + LED_COLOR_STEP_SIZE) );
          fill_solid( gLED, LED_NUM_LEDS, ColorFromPalette( myPal, gFixedColorIndex ) );
          FastLED.show( );
          EEPROM.write( EEPROM_ADR_FIXED_COLOR_INDEX, gFixedColorIndex );
        }
        if (firstTimeAfterBoot||mustReleaseModeSensor) {
          fill_solid( gLED, LED_NUM_LEDS, ColorFromPalette( myPal, gFixedColorIndex ) );
          FastLED.show( );
          firstTimeAfterBoot = false;
        }
      }
      break;
    case eCOLOR_TWINKLES:
      if ( shortModeTouch ) {
        fNextPalette( );
        EEPROM.write( EEPROM_ADR_TWINKLES_PALETTE, gTwinklesPalette );
      }
      fTwinkle( );
      FastLED.show( );
      break;
    case eSTROBE: {
        static bool strobesSet = false;
        static uint16_t lastMillis = millis( );
        byte maxLEDsOn = ((int)(LED_NUM_LEDS/2)) + 1;
        gStrobeSpeed   = max( gStrobeSpeed, 10  );
        gStrobeSpeed   = min( gStrobeSpeed, 100 );
        if ( shortModeTouch ) {
          gStrobeSpeed = ( gStrobeSpeed>10 ? gStrobeSpeed-10 : 100 );
          EEPROM.write( EEPROM_ADR_STROBE_SPEED, gStrobeSpeed );
          Serial << "gStrobeSpeed " << gStrobeSpeed << endl;
        }
        if ( !strobesSet ) {
          for( byte i = 0; i < maxLEDsOn; i++ ) {
            gLED[ random( LED_NUM_LEDS ) ] = CRGB::White;
            FastLED.show( );
          }
          strobesSet = true;
        }
       // hm. EVERY_N_MILLISECONDS did not work here so have to roll my own.
        uint16_t m = millis( );
        if ( ( m - lastMillis ) > gStrobeSpeed ) {
          if ( strobesSet ) {
            fill_solid( gLED, LED_NUM_LEDS, CRGB::Black );
            strobesSet = false;
          }
          lastMillis = m;
        }
        FastLED.show( );
      }
      break;
    case eRAINBOW: {
        byte start    = 7;
        byte stepSize = 3;
        gRainbowDeltaHue = max( gRainbowDeltaHue, start );
        if ( shortModeTouch ) {
          gRainbowDeltaHue = ( gRainbowDeltaHue<(start+10*stepSize) ? gRainbowDeltaHue+stepSize : start );
          EEPROM.write( EEPROM_ADR_RAINBOW_DELTA_HUE, gRainbowDeltaHue );
        }
        fill_rainbow( gLED, LED_NUM_LEDS, gHue, gRainbowDeltaHue );
        FastLED.show( );
      }
      break;
    case eRAINBOW_WITH_GLITTER: {
        byte start    = 7;
        byte stepSize = 3;
        gRainbowGlitterDeltaHue = max( gRainbowGlitterDeltaHue, start );
        if ( shortModeTouch ) {
          gRainbowGlitterDeltaHue = ( gRainbowGlitterDeltaHue<(start+10*stepSize) ? gRainbowGlitterDeltaHue+stepSize : start );
          EEPROM.write( EEPROM_ADR_RAINBOW_GLITTER_DELTA_HUE, gRainbowGlitterDeltaHue );
        }
        fill_rainbow( gLED, LED_NUM_LEDS, gHue, gRainbowGlitterDeltaHue );
        if ( random8( ) < 80 ) // 30% chance of glitter
          gLED[ random16( LED_NUM_LEDS ) ] += CRGB::White;
        FastLED.show( );
      }
      break;
    case eCONFETTI: {
        byte start      = 80;
        byte stepSize   = 10;
        gConfettiChance = max( gConfettiChance, start );
        if ( shortModeTouch ) {
          gConfettiChance = ( gConfettiChance<(start+10*stepSize) ? gConfettiChance+stepSize : start );
          EEPROM.write( EEPROM_ADR_CONFETTI_CHANCE, gConfettiChance );
        }
        fadeToBlackBy( gLED, LED_NUM_LEDS, 10 );
        if ( random8( ) < gConfettiChance )
          gLED[ random16( LED_NUM_LEDS ) ] += CHSV( gHue + random8( 64 ), 200, 255 );
        FastLED.show( );
      }
      break;
    case eBPM: {
        byte startBeats = 60;
        byte stepSize   = 5;
        gBeatsPerMinute = max( gBeatsPerMinute, startBeats );
        gBeatsPerMinute = min( gBeatsPerMinute, startBeats+15*stepSize );
        if ( shortModeTouch ) {
          gBeatsPerMinute = ( gBeatsPerMinute<(startBeats+15*stepSize) ? gBeatsPerMinute+stepSize : startBeats );
          EEPROM.write( EEPROM_ADR_BEATS_PER_MINUTE, gBeatsPerMinute );
        }
        uint8_t beat = beatsin8( gBeatsPerMinute, 64, 255);
        for ( int i = 0; i < LED_NUM_LEDS; i++ )
          gLED[ i ] = ColorFromPalette( PartyColors_p, gHue+(i*2), beat-gHue+(i*10) );
        FastLED.show( );
      }
      break;
    case eBREATHE: {
        static byte counter = 0;
        if ( shortModeTouch ) {
          gBreatheColorIndex = ( (gBreatheColorIndex + LED_COLOR_STEP_SIZE) > 255 ? 0 : (gBreatheColorIndex + LED_COLOR_STEP_SIZE) );
          fill_solid( gLED, LED_NUM_LEDS, ColorFromPalette( myPal, gBreatheColorIndex ) );
          FastLED.show( );
          EEPROM.write( EEPROM_ADR_BREATHE_COLOR_INDEX, gBreatheColorIndex );
        }
        byte brightness = quadwave8( 3*(counter++) ); // quadwave8() is nearly a sine wave but 2/3 faster than sin8()
        fill_solid( gLED, LED_NUM_LEDS, ColorFromPalette( myPal, gBreatheColorIndex, brightness ) );
        FastLED.show( );
        if ( brightness > 254 )
          FastLED.delay( 50 );
      }
      break;
    case eCYCLONE: {
        static byte index = 0;
        if ( shortModeTouch ) {
          gCycloneColorIndex = ( (gCycloneColorIndex + LED_COLOR_STEP_SIZE) > 255 ? 0 : (gCycloneColorIndex + LED_COLOR_STEP_SIZE) );
          fill_solid( gLED, LED_NUM_LEDS, ColorFromPalette( myPal, gCycloneColorIndex ) );
          FastLED.show( );
          EEPROM.write( EEPROM_ADR_CYCLONE_COLOR_INDEX, gCycloneColorIndex );
        }
        EVERY_N_MILLISECONDS( 80 ) {
          gLED[ index ] = ColorFromPalette( myPal, gCycloneColorIndex );
          FastLED.show( );
          fadeToBlackBy( gLED, LED_NUM_LEDS, 50 );
          index = ( index < (LED_NUM_LEDS-1) ? index+1 : 0 );
        }
      }
      break;
    default:
      break;
  }

  // slowly cycle the "base color" through the rainbow
  EVERY_N_MILLISECONDS( 20 ) { gHue++; }

  // fade in to master brightness when nightlight is plugged into socket.
  static byte fadeInBrightness = 0;
  if ( gPowerUp ) {
    if ( fadeInBrightness<gMasterBrightness ) {
      fadeInBrightness += 5;
      EVERY_N_MILLISECONDS( 10 ) {
        FastLED.setBrightness( fadeInBrightness );
        FastLED.show( );
      }
    } else
      gPowerUp = false;
  }

  // Check for serial command.
  if ( DEBUG ) {
    byte ch;
    if ( Serial.available( ) ) {
      switch ( ch = Serial.read( ) ) {
        case 'b': {
          // b{0..255} set brightness
          while ( !Serial.available( ) ) {}
          byte brightness = Serial.parseInt( );
          brightness = ( brightness > 255 ? 255 : brightness );
          FastLED.setBrightness( brightness );
          FastLED.show( );
          break;
        }
        case 'h':
          fShowHelp( );
          break;
        case 'p': {
          // p{0..255} set LEDs to color from palette myPal
          while ( !Serial.available( ) ) {}
          byte index = Serial.parseInt( );
          index = ( index > 255 ? 255 : index );
          fill_solid( gLED, LED_NUM_LEDS, ColorFromPalette( myPal, index ) );
          FastLED.show( );
          break;
        }
        case 'R':
          // soft reset, does not reset peripherals and registers
          asm volatile ("  jmp 0");
          break;
        default:
          // none
          break;
      }
    }
  } // EOF check for serial command
}

//
// Functions

//
// =====================================================================================
// Color twinkle lights by Mark Kriegsman.
//   https://gist.github.com/kriegsman/5408ecd397744ba0393e
#define STARTING_BRIGHTNESS 64
#define FADE_IN_SPEED       28
#define FADE_OUT_SPEED      20
#define DENSITY             128

CRGBPalette16 mkG_Palette;
CRGB r(CRGB::Red), R(0xA80016), w(85,85,85), W(CRGB::White), l(0xE1A024);
CRGBPalette16 mkG_allPalettes[] = {
  CRGBPalette16( r,r,r,r, R,R,R,R, r,r,r,r, R,R,R,R ),      // Red, Burgundy
  CRGBPalette16( l,l,l,l, l,l,l,l, l,l,l,l, l,l,l,l ),      // Incandescent "fairy lights"
  //LavaColors_p,
  ForestColors_p,
  CRGBPalette16( 0x000000, 0x250000, 0x4A0000, 0x6F0000,    // Variation of HeatColors_p with no white and less yellow
                 0x940000, 0xB90000, 0xDE0000, 0xFF0000,
                 0xFF2500, 0xFF4A00, 0xFF6F00, 0xFF9400,
                 0xFFB900, 0xFFDE00, 0xFFFF00, 0x250000 ),
  CRGBPalette16( W,W,W,W, w,w,w,w, w,w,w,w, w,w,w,w )       // Snow
};
void fChoosePalette( byte paletteNumber )
{
  paletteNumber = min( paletteNumber, ARRAY_SIZE(mkG_allPalettes)-1 ); // sanity check
  mkG_Palette = mkG_allPalettes[ paletteNumber ];
}
void fNextPalette( )
{
  gTwinklesPalette = ( gTwinklesPalette < (ARRAY_SIZE(mkG_allPalettes)-1) ? gTwinklesPalette+1 : 0 );
  fChoosePalette( gTwinklesPalette );
}

enum { eGETTING_DARKER = 0, eGETTING_BRIGHTER = 1 };
void fTwinkle( )
{
  // Make each pixel brighter or darker, depending on
  // its 'direction' flag.
  fBrightenOrDarkenEachPixel( FADE_IN_SPEED, FADE_OUT_SPEED) ;

  // Now consider adding a new random twinkle
  if( random8() < DENSITY ) {
    int pos = random16( LED_NUM_LEDS );
    if( !gLED[ pos ]) {
      gLED[pos] = ColorFromPalette( mkG_Palette, random8( ), STARTING_BRIGHTNESS, NOBLEND );
      fsetPixelDirection( pos, eGETTING_BRIGHTER );
    }
  }
}

void fBrightenOrDarkenEachPixel( fract8 fadeUpAmount, fract8 fadeDownAmount )
{
 for( uint16_t i = 0; i < LED_NUM_LEDS; i++) {
    if( fGetPixelDirection( i ) == eGETTING_DARKER ) {
      // This pixel is getting darker
      gLED[i] = fMakeDarker( gLED[i], fadeDownAmount );
    } else {
      // This pixel is getting brighter
      gLED[i] = fMakeBrighter( gLED[i], fadeUpAmount );
      // now check to see if we've maxxed out the brightness
      if( gLED[i].r == 255 || gLED[i].g == 255 || gLED[i].b == 255) {
        // if so, turn around and start getting darker
        fsetPixelDirection( i, eGETTING_DARKER );
      }
    }
  }
}

CRGB fMakeBrighter( const CRGB& color, fract8 howMuchBrighter )
{
  CRGB incrementalColor = color;
  incrementalColor.nscale8( howMuchBrighter );
  return color + incrementalColor;
}

CRGB fMakeDarker( const CRGB& color, fract8 howMuchDarker)
{
  CRGB newcolor = color;
  newcolor.nscale8( 255 - howMuchDarker );
  return newcolor;
}

uint8_t gDirectionFlags[ (LED_NUM_LEDS+7) / 8];
bool fGetPixelDirection( uint16_t i)
{
  uint16_t index = i / 8;
  uint8_t  bitNum = i & 0x07;
  return bitRead( gDirectionFlags[index], bitNum );
}

void fsetPixelDirection( uint16_t i, bool dir )
{
  uint16_t index = i / 8;
  uint8_t  bitNum = i & 0x07;
  bitWrite( gDirectionFlags[index], bitNum, dir);
}

//
// show help in serial monitor
void fShowHelp( ) {
  Serial << F( "=====================================================" ) << endl;
  Serial << F( "COMMANDS" ) << endl;
  Serial << F( "h                This help." ) << endl;
  Serial << F( "b{0..255}        Set LED master brightness." ) << endl;
  Serial << F( "p{0..255}        Set LEDs to color from (green,yellow,red) palette." ) << endl;
  Serial << F( "R                Reset Arduino. Does not reset peripherals and registers." ) << endl;
  Serial << F( "=====================================================" ) << endl;
}

//
// Bells'n'whistles! Play first notes from Star Wars Imperial March when the nightlight is plugged in.
// c/o https://gist.github.com/tagliati/1804108
void fPlayImperialMarch( )
{
  fBeep( 440, (long)(500/IMPERIAL_MARCH_SPEED) );
  fBeep( 440, (long)(500/IMPERIAL_MARCH_SPEED) );
  fBeep( 440, (long)(500/IMPERIAL_MARCH_SPEED) );
  fBeep( 349, (long)(350/IMPERIAL_MARCH_SPEED) );
  fBeep( 523, (long)(150/IMPERIAL_MARCH_SPEED) );
  fBeep( 440, (long)(500/IMPERIAL_MARCH_SPEED) );
  fBeep( 349, (long)(350/IMPERIAL_MARCH_SPEED) );
  fBeep( 523, (long)(150/IMPERIAL_MARCH_SPEED) );
  fBeep( 440, (long)(800/IMPERIAL_MARCH_SPEED) );
  delay( 100 );
  fBeep( 659, (long)(500/IMPERIAL_MARCH_SPEED) );
  fBeep( 659, (long)(500/IMPERIAL_MARCH_SPEED) );
  fBeep( 659, (long)(500/IMPERIAL_MARCH_SPEED) );
  fBeep( 698, (long)(350/IMPERIAL_MARCH_SPEED) );
  fBeep( 523, (long)(150/IMPERIAL_MARCH_SPEED) );
  fBeep( 415, (long)(500/IMPERIAL_MARCH_SPEED) );
  fBeep( 349, (long)(350/IMPERIAL_MARCH_SPEED) );
  fBeep( 523, (long)(150/IMPERIAL_MARCH_SPEED) );
  fBeep( 440, (long)(1000/IMPERIAL_MARCH_SPEED) );
}

//
// play a tone on piezo speaker with specific frequency and duration
void fBeep( int frequencyInHertz, long timeInMilliseconds )
{
  long delayAmount = (long)( 1000000 / frequencyInHertz );
  long loopTime    = (long)( ( timeInMilliseconds*1000 ) / ( delayAmount*2 ) );
  for ( int x = 0; x < loopTime; x++ ) {
    digitalWrite( PIEZO_PIN, HIGH );
    delayMicroseconds( delayAmount );
    digitalWrite( PIEZO_PIN, LOW );
    delayMicroseconds( delayAmount );
  }
  delay( 20 ); // a little delay to make all notes sound separate
}

//
// play this when one of the touch wire buttons is briefly touched
void fPiezoShortTouch( byte mode )
{
  int note = ( mode == eBRIGHTNESS_PIEZO ? 3000 : 1000 );
  fBeep( note, 2 );
}

//
// play this when one of the touch wire buttons is touched for a longer time (triggering its 2nd function)
void fPiezoLongTouch( byte mode )
{
  switch ( mode ) {
    case eBRIGHTNESS_PIEZO:
      fBeep( 240, 2 );
      fBeep( 880, 5 );
      fBeep( 1600, 9 );
      break;
    case eMODE_PIEZO:
      fBeep( 120, 2 );
      fBeep( 440, 5 );
      fBeep( 800, 9 );
      break;
    default:  /* e3RD_FUNC_PIEZO */
      fBeep( 5000, 15 );
      delay( 20 );
      fBeep( 5000, 15 );
      delay( 20 );
      fBeep( 5000, 15 );
      delay( 20 );
      fBeep( 5000, 15 );
      break;
  }
}


