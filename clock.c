#include <stdint.h>

#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/power.h>

#include "uart.h"

struct Time {
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hours;
};

struct Time time = {
    .seconds = 0,
    .minutes = 0,
    .hours = 0,
};

#define ZEHN    0
#define FUENF   1
#define VOR     2
#define NACH    3
#define HALB    4
#define DREI    5
#define VIERTEL 6
#define UHR     7

void main(void){
    power_all_disable();
    power_usart0_enable();
    power_timer1_enable();

    uart_init();

    // Set C0 & C1 (A0 & A1) to input
    DDRC &= ~(_BV(DDC0) | _BV(DDC1));
    // Set pull-ups
    PORTC |= _BV(PORTC0) | _BV(PORTC1);

    // Enable interrupts on PCMSK1
    PCICR |= _BV(PCIE1);
    // Enable interrupts on PCINT8 and PCINT9 (pins C0 and C1)
    PCMSK1 |= _BV(PCINT8) | _BV(PCINT9);

    // Various interrupt times for prescaler 1024
    // 0.08-second interrupt
    //OCR1A = 0x4e1;
    // 1-second interrupt
    OCR1A = 0x3D08;
    // 4-second interrupt
    //OCR1A = 0xF423;

    // Set interrupt on match to OCIE1A
    TIMSK1 |= _BV(OCIE1A);
    // Set CTC-OCR1A mode
    TCCR1B |= _BV(WGM12);
    // Set prescaler to 1024 (starts timer)
    TCCR1B |= _BV(CS12) | _BV(CS10);

    sleep_enable();
    while (1){
        sleep_bod_disable();
        sei();
        sleep_cpu();
    };
};

unsigned int uiround(unsigned int x, unsigned int round){
    // Round `x` to the nearest integer multiple of `round`
    return (x / round + (x % round > round / 2)) * round;
}

void set_output(struct Time time){
    uint8_t minutes = time.minutes + (time.seconds > 30);
    uint8_t hours = time.hours + (minutes > 13);

    uint8_t output_hours = hours % 12;
    uint8_t output_mins;

    switch (uiround(minutes, 5)){
    case 0:
    case 60:
        output_mins = _BV(UHR);
        break;
    case 5:
        output_mins = _BV(FUENF) | _BV(NACH);
        break;
    case 10:
        output_mins = _BV(ZEHN) | _BV(NACH);
        break;
    case 15:
        output_mins = _BV(VIERTEL);
        break;
    case 20:
        output_mins = _BV(ZEHN) | _BV(VOR) | _BV(HALB);
        break;
    case 25:
        output_mins = _BV(FUENF) | _BV(VOR) | _BV(HALB);
        break;
    case 30:
        output_mins = _BV(HALB);
        break;
    case 35:
        output_mins = _BV(FUENF) | _BV(NACH) | _BV(HALB);
        break;
    case 40:
        output_mins = _BV(ZEHN) | _BV(NACH) | _BV(HALB);
        break;
    case 45:
        output_mins = _BV(DREI) | _BV(VIERTEL);
        break;
    case 50:
        output_mins = _BV(ZEHN) | _BV(VOR);
        break;
    case 55:
        output_mins = _BV(FUENF) | _BV(VOR);
        break;
    }

    uart_send(output_hours);
    uart_send(output_mins);
    uart_send(time.seconds);
}

void increment_time(){
    // Increment time to nearest 5 minutes
    cli(); // Disable interrupts
    time.seconds = 0;
    time.minutes = uiround((time.minutes + 5), 5) % 60;
    time.hours = (time.hours + (time.minutes < 5)) % 12;
    sei(); // Enable interrupts
}

void decrement_time(){
    // Decrement time to nearest 5 minutes
    cli(); // Disable interrupts
    if (time.minutes < 5)
        time.minutes = 55 + time.minutes;
    else
        time.minutes = time.minutes - 5;

    if (time.minutes > 55) {
        if (time.hours == 0)
            time.hours = 11;
        else
            time.hours = time.hours - 1;
    }

    time.seconds = 0;
    time.minutes = uiround(time.minutes, 5);
    sei(); // Enable interrupts
}

ISR (PCINT1_vect) {
    // TODO: Better way of checking which one has changed
    if (PINC & _BV(PINC0)) {
        increment_time();
    }

    if (PINC & _BV(PINC1)) {
        decrement_time();
    }
}

ISR (TIMER1_COMPA_vect) {
    time.seconds = (time.seconds + 1) % 60;
    time.minutes = (time.minutes + (time.seconds == 0)) % 60;
    time.hours = (time.hours + ((time.minutes == 0) &&
                                (time.seconds == 0))) % 12;

    set_output(time);
}
