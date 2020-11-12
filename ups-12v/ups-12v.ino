/*
 * Fuses
 *   CKDIV8 = 1      (make sure unchecked) turn off the clock div
 *   BODLEVEL2 = 1   bod at 2.7V (could be as low as 2.5V; low-voltage tiny85 recommended)
 *   BODLEVEL1 = 0
 *   BODLEVEL0 = 1
 * 
 *   HIGH=DA, LOW=E2, EXT=FF
 */

#include <Arduino.h>
#include <TinyWireS.h>
#include <util/crc16.h>

#define R1 10000
#define R2 2200
// Note: Make sure to use a float when specifying the voltages.
//       Some rounding may occur, as these are converted to adc values.
#define ON_THRESH_DESIRED 11.5
#define POWERUP_THRESH_DESIRED 8.5
#define OFF_THRESH_DESIRED 8.0
#define SHUTDOWN_THRESH_DESIRED 9.0

#define SLOW_START_MOSFET

#include "../ups-common/ups.ino"
