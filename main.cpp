/*
 * avr_blink.cpp
 *
 * Created: 27/6/2017 6:26:37 PM
 * Author : ngj
 */ 

#define F_CPU 1000000UL

#define BIT_TST(REG, bit, val)			( (REG & (1UL << (bit) ) ) == ((val) << (bit)) )

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay.h>
#include "Rotary.h"

void init();
void LED_on_hours(uint8_t h);
static volatile uint8_t second, minute, hour;
static volatile uint8_t second_ones, second_tens, minute_ones, minute_tens;
static volatile bool clockSet = false;
Rotary rotaryStep = Rotary();

int main(void)
{	
	init();

    while (1) 
    {
		// Enter sleep mode.
		// Will wake from timer overflow interrupt
		sleep_mode();

		/*The MCU should always be awake for a minimum of one
		 * TOSC cycle, before re-entering sleep mode. Otherwise,
		 * the interrupt flag will not be cleared, and the MCU
		 * will stay in sleep mode until it is reset. We solve
		 * this by doing a dummy write to the TCCR2B register
		 * and waiting until the ASSR register reports that TC2
		 * has been updated, before going to sleep*/
			
		/* Dummy write the desired pre-scaler */
		TCCR2 |= ((1 << CS20) | (0 << CS21) | (1 << CS22));

		// Wait until TC2 is updated
		while (ASSR & ((1 << TCN2UB) | (1 << OCR2UB) | (1 << TCR2UB))) {}
		
		// Encoder position
		// Disable TOV2 for now

		// Grab state of input pins.
		// &PINC

		while (clockSet) {
			unsigned char rotaryResult = rotaryStep.process(&PINC);
			if (rotaryResult) {
				if ((rotaryResult) == DIR_CW) {
					if (minute == 00) {
						minute = 59;
						if (hour == 0) {
							hour = 23;
						} else	hour--;
					} else minute--;
				} else {
					minute++;
					if (minute == 60) {
						minute = 0;
						hour++;
						if (hour == 24) hour = 0;
					}				
				}
			}
		
			minute_ones = minute % 10;
			minute_tens = minute / 10;
			PORTB = ((minute_ones << 0) | (minute_tens << 4));

			LED_on_hours((hour));
		}
    }
}

void init(void) {
	
	second = 0;
	minute = 0;
	hour = 0;

	DDRA = 0b11111111;
	DDRB = 0b00111111;
	DDRC = 0x00;
	DDRD = 0b11110000;

	// Activate pull up resistors
	PORTC = 0x00;
	PORTD = 0b00001111;

	// Wait for external clock crystal to stabilize
	for (uint8_t i = 0; i < 0x40; i++) {
		for (uint32_t j = 0; j < 0xFF; j++) {
			// Do a no-op instruction to keep the empty
			// loop from being optimized away
			asm volatile("nop");
		}
	}

	// Make sure all TC0 interrupts are disabled
	TIMSK = 0x00;

	// set Timer/counter2 to be async from the CPU clock
	// This will clock TC2 from the external 32,768 kHz crystal
	ASSR |= (1 << AS2);
	
	// Reset Counter
	TCNT2 = 0;

	// Pre-scale the timer to be clock source/128 to make
	// TC2 overflow precisely once every second
	TCCR2 = ((1 << CS20) | (0 << CS21) | (1 << CS22));

	// Wait until TC2 is updated
	while (ASSR & ((1 << TCN2UB) | (1 << OCR2UB) | ( 1 << TCR2UB))) {}

	// Enable Timer/Counter2 overflow interrupts
	TIMSK |= (1 << TOIE2);

	// Enable External Interrupt Flag
	GICR |= (1 << INT0);

	// Low Level of INT0 generates interrupt request
	MCUCSR = ((1 << ISC01) | (0 << ISC00));

	// Set the Global interrupt enable bit
	sei();

	// Setting the sleep mode to be used to power save mode
	set_sleep_mode(SLEEP_MODE_PWR_SAVE);

	// Enable sleep mode
	sleep_enable();
}

void LED_on_hours(uint8_t h) {

		if (h == 0) PORTD = ((12 << 4) | (15 << 0));
		else if (h > 12) PORTD = (((h-12) << 4) | (15 << 0));
		else PORTD = ((h << 4) | (15 << 0));

}

ISR(TIMER2_OVF_vect) {

	second++;
	if (second == 60) {
		second = 0;
		minute++;
		if (minute == 60) {
			minute = 0;
			hour++;
			if (hour == 24) {
				hour = 0;
			}
		}
	}
	
	second_ones = second % 10; //1s
	second_tens = second / 10; //10s

	minute_ones = minute % 10;
	minute_tens = minute / 10;
	
	// Bit-shift magic :)
	PORTA = ((second_ones << 0) | (second_tens << 4));
	PORTB = ((minute_ones << 0) | (minute_tens << 4));
	
	LED_on_hours(hour);
	
}

ISR(INT0_vect) {
	// Toggle boolean
	GICR = (0 << INT0); 
	clockSet = !clockSet;	
	_delay_ms(500); //Wait for button to reset to neutral before enabling INT0, can change to while loop
	GICR |= (1 << INT0); //Turn interrupt flag on - counters debouncing
}