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

volatile struct Time global_time = {
    .seconds = 0,
    .minutes = 0,
    .hours = 0,
};

// Number instead of bool to allow debounce time > 12.5ms
volatile uint8_t switch_pressed = 0;

#define ZEHN    0
#define FUENF   1
#define VOR     2
#define NACH    3
#define HALB    4
#define DREI    5
#define VIERTEL 6
#define UHR     7

// DDR? = Data direction (true for outputs)
// PIN? = Value (if input)
// PORT? = Value (if output) or pull-up activation (if input)
void main(void){
    power_all_disable();
    power_usart0_enable();
    power_timer1_enable();
    power_timer0_enable();

    uart_init();

    // Set D2 & D3 to input
    DDRD = ~(_BV(PIND2) | _BV(PIND3));
    // Set pull-ups
    PORTD = _BV(PIND2) | _BV(PIND3);

    // Other pins are all outputs, starting zero'd
    DDRC = DDRB = ~0;
    PORTC = PORTB = 0;

    // Enable interrupts on PCINT18 and PCINT19 (pins D2 and D3)
    PCMSK2 |= _BV(PCINT18) | _BV(PCINT19);
    // Enable interrupts on PCMSK2
    PCICR |= _BV(PCIE2);

    // Clock speed is 16_000_000 Hz
    // OCRnA value = (clk / prescaler * desired) - 1
    // Debounce timer
    OCR0A = 0xc2; // 12.5ms interrupt with prescaler == 1024 (0xFF max)
    TIMSK0 |= _BV(OCIE0A); // Set interrupt on match to TIMER0_COMPA
    TCCR0A |= _BV(WGM01); // Set CTC-OCR0A mode

    // Clock timer
    OCR1A = 0x3D08; // 1s interrupt with prescaler == 1024
    TIMSK1 |= _BV(OCIE1A); // Set interrupt on match to TIMER1_COMPA
    TCCR1B |= _BV(WGM12); // Set CTC-OCR1A mode
    TCCR1B |= _BV(CS12) | _BV(CS10); // Set prescaler to 1024 (starts timer)

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

    PORTB = output_hours;

    // Don't mess with pull-up settings
    PORTD &= ~(_BV(PIND4) | _BV(PIND5));
    PORTD |= (((output_mins & _BV(DREI)) ? _BV(PIND4) : 0)
              | ((output_mins & _BV(FUENF)) ? _BV(PIND5) : 0));

    PORTC = (((output_mins & _BV(VOR)) ? _BV(PINC1) : 0)
             | ((output_mins & _BV(NACH)) ? _BV(PINC2) : 0)
             | ((output_mins & _BV(ZEHN)) ? _BV(PINC3) : 0)
             | ((output_mins & _BV(HALB)) ? _BV(PINC4) : 0)
             | ((output_mins & _BV(VIERTEL)) ? _BV(PINC5) : 0));
}

struct Time increment_time(struct Time time){
    // Increment time to nearest 5 minutes
    time.seconds = 0;
    time.minutes = uiround((time.minutes + 5), 5) % 60;
    time.hours = (time.hours + (time.minutes < 5)) % 12;

    return time;
}

struct Time decrement_time(struct Time time){
    // Decrement time to nearest 5 minutes
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

    return time;
}

ISR (PCINT2_vect) {
    if (switch_pressed)
        return;

    switch_pressed = 1;
    TCCR0B |= _BV(CS02) | _BV(CS00); // Set prescaler to 1024 (starts timer)
}

ISR (TIMER1_COMPA_vect) {
    global_time.seconds = (global_time.seconds + 1) % 60;
    global_time.minutes = (global_time.minutes + (global_time.seconds == 0)) % 60;
    global_time.hours = (global_time.hours +
                         ((global_time.minutes == 0) && (global_time.seconds == 0))) % 12;

    set_output(global_time);
}

ISR (TIMER0_COMPA_vect) {
    if (switch_pressed == 0) {
        TCCR0B &= ~(_BV(CS02) | _BV(CS00)); // Set prescaler to 0 (stops timer)

        if ((PIND & _BV(PIND2)) == 0) {
            global_time = increment_time(global_time);
            set_output(global_time);
        }
        if ((PIND & _BV(PIND3)) == 0) {
            global_time = decrement_time(global_time);
            set_output(global_time);
        }
    } else {
        switch_pressed -= 1;
    }
}
