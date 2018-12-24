#include "uart.h"

#include <avr/io.h>
#include <avr/interrupt.h>

#define BAUD 9600
#define UBRR (F_OSC / 16 / BAUD - 1)


void uart_init(){
    // Set baud rate (USART Baud Rate Register Low/High)
    UBRR0H = (uint8_t) (UBRR >> 8);
    UBRR0L = (uint8_t) UBRR;

    // Enable TX/RX (USART Control & Status Register)
    UCSR0B |= _BV(TXEN0) | _BV(RXEN0);
    // Set 8-bit char size
    UCSR0C |= _BV(UCSZ01) | _BV(UCSZ00) & ~_BV(UCSZ02);
    // Set asynchronous mode
    UCSR0C &= ~(_BV(UMSEL01) | _BV(UMSEL00));
    // Set no parity
    UCSR0C &= ~(_BV(UPM01) | _BV(UPM00));
    // Set 1 stop bit
    UCSR0C &= ~_BV(USBS0);
}

void uart_send(char c){
    UDR0 = c;
    while (!(UCSR0A & _BV(UDRE0))); // Wait for bit to send
}

char uart_recv(){
    while (!(UCSR0A & _BV(RXC0))); // Wait for bit to be ready
    return UDR0;
}
