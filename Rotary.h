/*
 * Rotary encoder library
 */

 #include <avr/io.h>

#ifndef Rotary_h
#define Rotary_h

// Values returned by 'process'
// No complete step
#define DIR_NONE 0x0
// Clockwise step
#define DIR_CW 0x10
// Counter-clockwise step
#define DIR_CCW 0x20

class Rotary {

	public:
		Rotary();
		unsigned char process(volatile uint8_t *pin);
	private:
		unsigned char state;
};

#endif
