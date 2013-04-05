/*
 * Microprocessor.c
 *
 * Created: 10/15/2012 3:15:21 PM
 *  Author: Alexander Tracy
 */ 

#define F_CPU 8000000UL
#define LCD_IO_MODE 1
#define CTC_MATCH_OVERFLOW ((F_CPU / 1000) / 8)
#define aref 5
#define thresholdWeightLoc 0x00
#define thresholdSpeedLoc 0x10
#define thresholdTempLoc 0x20
#define BAUD 9600

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include <avr/eeprom.h>
#include <util/delay.h>
#include <stdint.h>
#include <stdlib.h>
#include <avr/pgmspace.h>
#include "lcd.h"
#include <math.h>
#include <stdio.h>

#include <util/setbaud.h>

void uart_init(void) {
	UBRR0H = UBRRH_VALUE;
	UBRR0L = UBRRL_VALUE;
	
	#if USE_2X
		UCSR0A |= _BV(U2X0);
	#else
		UCSR0A &= ~(_BV(U2X0));
	#endif
	
	UCSR0C = _BV(UCSZ01) | _BV(UCSZ00); /* 8-bit data */
	UCSR0B = _BV(RXEN0) | _BV(TXEN0); /* Enable RX and TX */
}

void uart_putchar(char c, FILE *stream) {
    if (c == '\n') {
        uart_putchar('\r', stream);
    }
    loop_until_bit_is_set(UCSR0A, UDRE0);
    UDR0 = c;
}

char uart_getchar(FILE *stream) {
	loop_until_bit_is_set(UCSR0A, RXC0); /* Wait until data exists. */
	return UDR0;
}

volatile unsigned long timer1_millis;
long milliseconds_since;

ISR (TIMER1_COMPA_vect)
{
	timer1_millis++;
}

unsigned long millis ()
{
	unsigned long millis_return;
	
	ATOMIC_BLOCK(ATOMIC_FORCEON)
	{
		millis_return = timer1_millis;
	}
	
	return millis_return;
}



int getADC(uint8_t adcport)
{
	int ADCval;
	
	ADMUX = adcport;
	ADMUX |= (1 << REFS0);
	ADMUX &= ~(1 << ADLAR);
	
	ADCSRA |= (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
	ADCSRA |= (1 << ADEN);
	
	ADCSRA |= (1 << ADSC);
	
	while (ADCSRA & (1 << ADSC));
	
	ADCval = ADCL;
	ADCval = (ADCH << 8) + ADCval;
	
	return ADCval;	
}

int getButtonPress()
{
	
	//return bit_is_clear(PIND, 6);
	
	//if (PIND && 0b10000000)
	if (bit_is_clear(PIND, 6))
	{
		return 3;
	}
	else if (bit_is_clear(PIND, 5))
	{
		return 2;
	}
	else if (bit_is_clear(PIND, 4))
	{
		return 1;
	}
	else
		return 0;
	
	
}

void displayWeight(char buffer[8], int weight, int thresholdWeight)
{
	lcd_clrscr();
	
	lcd_puts("Weight: ");
	itoa( weight, buffer, 10);
	lcd_puts(buffer);
	lcd_puts(" g\n");
	
	lcd_puts("Threshold: ");
	itoa( thresholdWeight, buffer, 10);
	lcd_puts(buffer);
	lcd_puts(" g\n");
}

void displaySpeed(char buffer[8], int speed, int thresholdSpeed)
{
	lcd_clrscr();
	
	lcd_puts("Speed: ");
	itoa( speed, buffer, 10);
	lcd_puts(buffer);
	lcd_puts(" mph\n");
	
	lcd_puts("Max: ");
	itoa( thresholdSpeed, buffer, 10);
	lcd_puts(buffer);
	lcd_puts(" mph\n");

}

void displayTemp(char buffer[8], int temp, int thresholdTemp)
{
	lcd_clrscr();
	
	lcd_puts("Temp: ");
	itoa( temp, buffer, 10);
	lcd_puts(buffer);
	lcd_puts(" C\n");
	
	lcd_puts("Threshold: ");
	itoa( thresholdTemp, buffer, 10);
	lcd_puts(buffer);
	lcd_puts(" C\n");

}

void sendSerial(char buffer[8], int weight, int thresholdWeight, int speed, int thresholdSpeed, int temp, int thresholdTemp)
{
	// Send data over serial port
	
	printf("{\n");
	
	// Send weight
	printf("\"weight\": \"");
	itoa( weight, buffer, 10);
	printf(buffer);
	printf(" g\",\n");
	
	// Send threshold weight
	printf("\"thresholdWeight\": \"");
	itoa( thresholdWeight, buffer, 10);
	printf(buffer);
	printf(" g\",\n");
	
	// Send speed
	printf("\"speed\": \"");
	itoa( speed, buffer, 10);
	printf(buffer);
	printf(" mph\",\n");
	
	// Send threshold speed
	printf("\"thresholdSpeed\": \"");
	itoa( thresholdSpeed, buffer, 10);
	printf(buffer);
	printf(" mph\",\n");
	
	// Send temperature
	printf("\"temperature\": \"");
	itoa( temp, buffer, 10);
	printf(buffer);
	printf(" C\",\n");
	
	// Send threshold temperature
	printf("\"thresholdTemperature\": \"");
	itoa( thresholdTemp, buffer, 10);
	printf(buffer);
	printf(" C\"\n");
	
	printf("}\n");
}

int getWeight()
{
	int measuredV;
	int actualV;
	int weight;
	float ratio = (float)5000 / (float)1023;
	
	measuredV = getADC(7);
	
	actualV = (int) round( (float)measuredV * ratio);
	weight = (int) round( ( ((float)actualV / (float)1000) * (float)12.4533 ) + (float)2.9589 );
	weight = (int) round( (float)125 * ((float)actualV / (float)1000 ) + (float)19.150 );
	
	if(weight == 19)
	{
		weight -= 19;
	}
	
	return weight;
}

int getTemp()
{
	int measuredV;
	int actualV;
	int temperature;
	float ratio = (float)5000 / (float)1023;
	
	/*
	Sample Input
	
	temperature coefficient:	22.5 mV/C
	
	Vout = (Vcc / 5 V) * [1.375 V + 22.5 mV/C * Ta]
	==>
	T = (Vout - 1.375)/(.0225 V)
	
	
	*/
	
	measuredV = getADC(6);
	actualV = (int) round( (float)measuredV * ratio);
	
	temperature = (int) round( ((float)actualV - (float)1375) / ( (float)22.5 ) );
	
	return temperature;
	
	
}

int getWindSpeed()
{
	
	/*
	Sample Input
	
	mph = 2.25 * Hz
	
	*/
	unsigned long t1;
	unsigned long t2;
	int count;
	int delta_t;
	int measuredV;
	float freq;
	int mph;
	float ratio = (float)5000 / (float)1023;
	
	int actualV;
	int previousV;
	int deltaV[1000];
	
	measuredV = getADC(5);
	previousV = (int) round( (float)measuredV * ratio );
	deltaV[0] = 0;
	
	t1 = millis();
	for (int i = 1; i < 1000; i++)
	{
		measuredV = getADC(5);
		actualV = (int) round( (float)measuredV * ratio);
		deltaV[i] = actualV - previousV;
		previousV = actualV;
	}
	t2 = millis();
	delta_t = (int) round( t2 - t1 );
	
	count = 0;
	
	for (int i = 1; i < 1000; i++ )
	{
		if (deltaV[i] > 75 || deltaV[i] < -75)
		{
			count++;
		}
	}
	
	if (delta_t == 0)
	{
		delta_t = 1;
	}
	
	
	freq = (float)count * (float)1000 / (float)delta_t;
	
	mph = (int) round( (float)freq * (float)2.25 );
	
	//mph = 5;
	return mph;
}

void writeThresholdWeight(int thresholdWeight)
{
	eeprom_write_byte((uint8_t*)thresholdWeightLoc, thresholdWeight);
}

void writeThresholdSpeed(int thresholdSpeed)
{
	eeprom_write_byte((uint8_t*)thresholdSpeedLoc, thresholdSpeed);
}

void writeThresholdTemp(int thresholdTemp)
{
	eeprom_write_byte((uint8_t*)thresholdTempLoc, thresholdTemp);
}

int main(void)
{
	FILE uart_output = FDEV_SETUP_STREAM(uart_putchar, NULL, _FDEV_SETUP_WRITE);
	FILE uart_input = FDEV_SETUP_STREAM(NULL, uart_getchar, _FDEV_SETUP_READ);
	uart_init();
	stdout = &uart_output;
	stdin = &uart_input;
	
    // for millisecond timer
	TCCR1B |= (1 << WGM12) | (1 << CS11);
	OCR1AH = (CTC_MATCH_OVERFLOW >> 8);
	OCR1AL = CTC_MATCH_OVERFLOW;
	TIMSK1 |= (1 << OCIE1A);
	sei();
	
	char buffer[8];
	int thresholdWeight;
	int thresholdSpeed;
	int thresholdTemp;
	int weight;
	int speed;
	int temp;
	
	PORTB = 0x00;
	PORTD = 0x00;
	DDRB = 0xFF;
	DDRD = 0x00;
	
	//DDRD &=~ (1 << PD2);
	//PORTD |= (1 << PD2);
	
	lcd_init(LCD_DISP_ON);
	
	thresholdWeight = eeprom_read_byte((uint8_t*)thresholdWeightLoc);
	thresholdSpeed = eeprom_read_byte((uint8_t*)thresholdSpeedLoc);
	thresholdTemp = eeprom_read_byte((uint8_t*)thresholdTempLoc);
	
	while(1)
    {
		_delay_ms(1000);
		
		lcd_clrscr();
		
		// Get and display weight
		weight = getWeight();
		
		
		displayWeight(buffer, weight, thresholdWeight);
		
		_delay_ms(1000);
		//
		
		// Get and display temperature
		temp = getTemp();
		
		displayTemp(buffer, temp, thresholdTemp);
		
		_delay_ms(1000);
		//
		
		// Get and display wind speed
		speed = getWindSpeed();
		
		displaySpeed(buffer, speed, thresholdSpeed);
		
		_delay_ms(1000);
		//
		
		
		sendSerial(buffer, weight, thresholdWeight, speed, thresholdSpeed, temp, thresholdTemp);
		
		int x = getButtonPress();
		if(x == 1)
		{
			thresholdWeight += 5;
			if(thresholdWeight >= 700)
			{
				thresholdWeight -= 700;
			}
		}
		if(x == 2)
		{
			thresholdWeight -= 5;
			if(thresholdWeight <= 0)
			{
				thresholdWeight += 700;
			}
		}
		if(x == 3)
		{
			writeThresholdWeight(thresholdWeight);
			lcd_clrscr();
			_delay_ms(500);
			displayWeight(buffer, weight, thresholdWeight);
			_delay_ms(500);
			lcd_clrscr();
			_delay_ms(500);
			displayWeight(buffer, weight, thresholdWeight);
			
		}
		
		if(weight >= thresholdWeight)
		{
			PORTB = 0xFF;
		}
		else
		{
			PORTB = 0x00;
		}
		
		
    }
	
	return 0;
}