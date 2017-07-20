/* Rotary encoder handler
 * Adapted from Rotary.cpp by Ben Buxton, bb@cactii.net
 * https://github.com/brianlow/Rotary/blob/master/Rotary.cpp
 */

#include "Rotary.h"
#include <avr/io.h>

#define BIT_TST(REG, bit, val)			( (REG & (1UL << (bit) ) ) == ((val) << (bit)) )

/*
 * The below state table has, for each state (row), the new state
 * to set based on the next encoder output. From left to right in,
 * the table, the encoder outputs are 00, 01, 10, 11, and the value
 * in that position is the new state to set.
 */

 #define R_START 0x0
 
 // not interested in half-steps
 // full-step table

 #define R_CW_FINAL 0x1
 #define R_CW_BEGIN 0x2
 #define R_CW_NEXT 0x3
 #define R_CCW_BEGIN 0x4
 #define R_CCW_FINAL 0x5
 #define R_CCW_NEXT 0x6

 const unsigned char ttable[7][4] = {
	 // R_START
	 {R_START,    R_CW_BEGIN,  R_CCW_BEGIN, R_START},
	 // R_CW_FINAL
	 {R_CW_NEXT,  R_START,     R_CW_FINAL,  R_START | DIR_CW},
	 // R_CW_BEGIN
	 {R_CW_NEXT,  R_CW_BEGIN,  R_START,     R_START},
	 // R_CW_NEXT
	 {R_CW_NEXT,  R_CW_BEGIN,  R_CW_FINAL,  R_START},
	 // R_CCW_BEGIN
	 {R_CCW_NEXT, R_START,     R_CCW_BEGIN, R_START},
	 // R_CCW_FINAL
	 {R_CCW_NEXT, R_CCW_FINAL, R_START,     R_START | DIR_CCW},
	 // R_CCW_NEXT
	 {R_CCW_NEXT, R_CCW_FINAL, R_CCW_BEGIN, R_START},
 };

 Rotary::Rotary() {
	state = R_START;
 }

 unsigned char Rotary::process(volatile uint8_t *pin) {
	// Grab state of input pins	
	unsigned char pinstate = (!(BIT_TST(*pin, 1, 1)) << 1) | !(BIT_TST(*pin, 0, 1));
	// Determine new state from the pins and state table.
	state = ttable[state & 0xf][pinstate];
	return state & 0x30;
 }