/**
 * Blink an LED every second.
 * This program does not use any of the Arduino SDK. It implements functionality equivalent to
 * pinMode, digitalWrite, and delay in terms of low-level AVR features.
 */

#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdbool.h>
#include <stdint.h>

#ifndef LED
#define LED 12
#endif

#define clockCyclesPerMicrosecond() (F_CPU / 1000000L)
#define sbi(port,bit) (port) |= (1 << (bit))    // set bit
#define cbi(port,bit) (port) &= ~(1 << (bit))   // clear bit
#define tbi(port,bit) ((port) ^= _BV(bit))      // toggle bit

volatile unsigned long timer0_overflow_count = 0;

ISR(TIMER0_OVF_vect)
{
    timer0_overflow_count++;
}

typedef enum {
    INPUT,
    OUTPUT
} mode;

// Get a pointer to the mode register for the port containing the given pin.
volatile uint8_t *pin_to_mode(uint8_t pin)
{
    if (pin < 8) return &DDRD;
    else if (pin < 14) return &DDRB;
    else return &DDRC;
}

// Get a pointer to the output register for the port containing the given pin.
volatile uint8_t *pin_to_port(uint8_t pin)
{
    if (pin < 8) return &PORTD;
    else if (pin < 14) return &PORTB;
    else return &PORTC;
}

// Get the bit offset of the given pin within its port.
uint8_t pin_to_bit(uint8_t pin)
{
    volatile uint8_t *port = pin_to_port(pin);
    if (port == &PORTD) return pin;
    else if (port == &PORTB) return pin - 8;
    else return pin - 14;
}

// Set the mode of the given pin.
void pin_mode(uint8_t pin, mode m)
{
    volatile uint8_t *port = pin_to_port(pin);
    uint8_t bit = pin_to_bit(pin);

    switch (m) {
    case INPUT:
        cbi(*port, bit);
        break;
    case OUTPUT:
        sbi(*port, bit);
        break;
    }
}

// Toggle the output value of the given pin.
void toggle_pin(uint8_t pin)
{
    volatile uint8_t *port = pin_to_port(pin);
    uint8_t bit = pin_to_bit(pin);
    tbi(*port, bit);
}

// Current time in microseconds.
unsigned long micros()
{
    unsigned long m;
    uint32_t oldSREG = SREG, t;

    cli();
    m = timer0_overflow_count;
    t = TCNT0;

    if ((TIFR0 & _BV(TOV0)) && (t < 255))
        m++;

    SREG = oldSREG;

    return ((m << 8) + t) * (64 / clockCyclesPerMicrosecond());
}

// Spin for the given number of milliseconds.
void delay(unsigned long ms)
{
    uint16_t start = (uint16_t)micros();

    while (ms > 0) {
        if (((uint16_t)micros() - start) >= 1000) {
            ms--;
            start += 1000;
        }
    }
}

// Set up timer
void init()
{
    // Enable interrupts so we can count clock overflows
    sei();

    // set timer 0 prescale factor to 64
    // this combination is for the standard 168/328/1280/2560
    sbi(TCCR0B, CS01);
    sbi(TCCR0B, CS00);

    // enable timer 0 overflow interrupt
    sbi(TIMSK0, TOIE0);
}

int main()
{
    init();
    pin_mode(LED, OUTPUT);

    while (true) {
        toggle_pin(LED);
        delay(500);
    }
}
