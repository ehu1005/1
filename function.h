#include <avr/pgmspace.h>
#include <math.h>
#include <stdlib.h>

typedef unsigned char		U08;		// data type definition
typedef   signed char		S08;
typedef unsigned int		U16;
typedef   signed int		S16;
typedef unsigned long int	U32;
typedef   signed long int	S32;

#define BV(bit)		(1<<(bit))		// bit processing
#define cbi(reg,bit)	reg &= ~(BV(bit))
#define sbi(reg,bit)	reg |= (BV(bit))


void MCU_initialize(void);			// initialize ATmega128A MCU
void Delay_us(U08 time_us);			// time delay for us in 16MHz (1~255 us)
void Delay_ms(U16 time_ms);			// time delay for ms in 16MHz (1~65535 ms)
void Delay_s(U16 time_s);			// time delay for sec in 16MHz (1~65535 s)
unsigned char Key_input(void);			// input key KEY1 - KEY4 


void MCU_initialize(void)			/* initialize ATmega128A MCU */
{
	DDRA  = 0x00;					
	PORTA = 0x00;

	DDRB  = 0x82;				// PB7 = PWM output, PB1 = output
	PORTB = 0x00;					

	DDRC  = 0x00;					
	PORTC = 0x00;					 

	DDRD  = 0x00;					
	PORTD = 0x00;					

	DDRE  = 0x02;				// PE1 = output, PE0 = input
	PORTE = 0x00;

	DDRF  = 0x00;				// PF1 = input
	PORTF = 0x00;

	DDRG  = 0x00;				// PG3~0 = input
	PORTG = 0x00;					
}

void Delay_us(U08 time_us)			/* time delay for us in 16MHz */
{
	register unsigned char i;

	for(i = 0; i < time_us; i++)			// 5 cycle +
	{ 
		asm volatile(" PUSH  R0 ");		// 2 cycle +
		asm volatile(" POP   R0 ");		// 2 cycle +
		asm volatile(" NOP      ");		// 1 cycle +
		asm volatile(" NOP      ");		// 1 cycle +
		asm volatile(" NOP      ");		// 1 cycle +
		asm volatile(" PUSH  R0 ");		// 2 cycle +
		asm volatile(" POP   R0 ");		// 2 cycle = 16 cycle = 1 us for 16MHz
	}
}

void Delay_ms(U16 time_ms)			/* time delay for ms in 16MHz */
{
	register unsigned int i;

	for(i = 0; i < time_ms; i++)
	{	Delay_us(248);
		Delay_us(250);
		Delay_us(250);
		Delay_us(250);
	}
}

void Delay_s(U16 time_s)
{
      register unsigned int i;
      
      for(i=0; i< time_s; i++)
      {
	      Delay_ms(1000);
      }
}

#define KEY1		0x0E			// KEY1 value
#define KEY2		0x0D			// KEY2 value
#define KEY3		0x0B			// KEY3 value
#define KEY4		0x07			// KEY4 value
#define no_key		0x0F			// no key input

unsigned char key_flag = 0;

unsigned char Key_input(void)		/* input key KEY1 - KEY4 */
{
	unsigned char key;

	key = PING & 0x0F;				// any key pressed ?
	if(key == 0x0F)				// if no key, check key off
	{ if(key_flag == 0)
		return key;
		else
		{ Delay_ms(20);
			key_flag = 0;
			return key;
		}
	}
	else						// if key input, check continuous key
	{ if(key_flag != 0)				// if continuous key, treat as no key input
		return 0x0F;
		else					// if new key, delay for debounce
		{ Delay_ms(20);
			key_flag = 1;
			return key;
		}
	}
}