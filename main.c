#include <avr/io.h>
#include <avr/interrupt.h>
#include "function.h"

#define CDS_20 409 //CDS_lux = k, k = (V_ADC/5)*1023, V_ADC = (10k/(10k+R_CDS))*5
#define CDS_30 441
#define CDS_40 457
#define CDS_50 474
#define CDS_60 487
#define CDS_70 506

unsigned int Read_light(void);
void Restart_reset(void);
void LED_off(void);
void Blink(U08 num);

volatile unsigned int light;
volatile unsigned int ms_count, sec_count, min_count = 0;
volatile unsigned char auto_light_mode = 0;
volatile unsigned char light_level = 2;
volatile unsigned char timer_mode = 0;
volatile unsigned char LED_mode = 1;
volatile unsigned char fadeout_flag = 0;
volatile unsigned char now = 0;
volatile unsigned char hys_flag = 0;

ISR(TIMER0_COMP_vect)	// interrupt per 1ms
{
  ms_count++;
}


int main(void)
{
  MCU_initialize();
  Delay_ms(50);

  ADCSRA = 0x87;	// ADC enable, 125kHz
  TCCR0 = 0X0C;		// Clear Timer on Compare match mode & prescaler = 64 
  OCR0 = 249;		// define Top value
  TCNT0 = 0;		// clear initial value
  TIMSK |= (1<<OCIE0);	// Enable Compare match interrupt 
  TCCR2 = 0x6A;		// Fast pulse width modulation mode(non-inverting) & prescaler = 8, frequency = 7812.5Hz 
  TCNT2 = 0;		// clear initial value
  OCR2 = 63;		// initial light_level setting
  sei();		// enable interrupt
  
  unsigned char key;
  unsigned char change_ms = 0;
  unsigned char fade_ms = 0;
  
  while(1)
  {
    key = Key_input();
    switch(key) 
	{
	  case(KEY1) :	// turn on/off
	  {
	     if(LED_mode == 0)
	     {
	       Restart_reset();
	     }
	     else
	     {
	       LED_off();
	     }
	     break; 
	  }
	  
	  case(KEY2) : // auto mode on/off
	  {
	    if(auto_light_mode == 0)
	     auto_light_mode = 1;
	    else
	     auto_light_mode = 0;
	    break;
	  }
	  
	  case(KEY3) : // level select
	  {
	    auto_light_mode = 0; 
	    if(light_level == 4) 
	      light_level = 1;
	    else
	      light_level++;
	    break;
	  }
	  
	  case(KEY4) : // timer mode on/off
	  {
	    if(timer_mode == 3)
	      timer_mode = 0;
	    else
	      timer_mode++;
	    ms_count = 0;
	    sec_count = 0;
	    min_count = 0;  // clear time count
	    TCNT0 = 0;
	    Blink(timer_mode);
	    break;
	  }
	  default : // no key or error
	    break;
	} // switch
	
  if(LED_mode == 1) 
	{
	  now = OCR2;
	  if(ms_count == 1000)
	  {
	    ms_count = 0;
	    sec_count++;
	  } 
	  if(sec_count == 60)
	  {
	    sec_count = 0;
	    min_count++;
	  }
	  if(fadeout_flag != 1)
	  {
	    if(auto_light_mode == 1) // sensor O
	    {
	     light = Read_light();
	     if((light <= CDS_30) && (hys_flag == 0)) 
	     {
	       OCR2++;
	       if(ms_count < 950)
		change_ms = ms_count + 50;
	       else
		change_ms = ms_count - 950;
	       hys_flag = 1;
	     }
	     else if((light >= CDS_60) && (hys_flag == 0)) 
	     {
	       OCR2--;
	       if(ms_count < 950)
		change_ms = ms_count + 50;
	       else
		change_ms = ms_count - 950;
	       hys_flag = 2;
	     }
	     if((hys_flag == 1) && (change_ms == ms_count)) // UTP(Upper Triggering Point)
	     {
		if((light <= CDS_60) && (OCR2 <= 120))
		  {
		    OCR2++;
		    if(change_ms < 950)
		      change_ms += 50;
		    else
		      change_ms -= 950;		    
		  }
		if(light >= CDS_70)
		  hys_flag = 2;
	     }
	     else if((hys_flag == 2) && (change_ms == ms_count)) // LTP(Lower Triggering Point)
	     {
	       if((light >= CDS_30) && (OCR2 >= 10))
		{
		  OCR2--;
		  if(change_ms < 950)
		    change_ms += 50;
		  else
		    change_ms -= 950;		  
		}
	       if(light <= CDS_20)
		hys_flag = 1;
	     }
	    }
	   else if(auto_light_mode == 0) // sensor X
	    {
	      if(light_level == 1)
		OCR2 = 0;
	      else if(light_level == 2)
		OCR2 = 60;
	      else if(light_level == 3)
		OCR2 = 120;
	      else if(light_level == 4)
		OCR2 = 180;
	    }
	   if(((timer_mode == 1) && (min_count == 15)) || ((timer_mode == 2) && (min_count == 30)) || ((timer_mode == 3) && (min_count == 60)))
	     {
		fadeout_flag = 1;
		if(ms_count < 950)
		  fade_ms = ms_count + 50;
		else
		  fade_ms = ms_count - 950;
	     }
	  }
	  if((fadeout_flag == 1) && (fade_ms == ms_count))
	  {
	    OCR2--;
	    if(fade_ms < 950)
	      fade_ms += 50;
	    else
	      fade_ms -= 950;
	    if(OCR2 == 0)
	    {
	      fadeout_flag = 0;
	      LED_off(); 
	    }
	  }
	    
      } // LED_mode = 1
	
  } // while
} // main

unsigned int Read_light(void)
{
	unsigned char i;
	unsigned int result;
	unsigned char ADC_index, ADC_flag = 0;
	unsigned int ADC_buffer[16];
	
	ADMUX = 0x41;	  // select channel ADC1 with AVCC
	Delay_us(150);
	ADCSRA = 0xD7;	  // ADC start & clear ADIF
	while((ADCSRA & 0x10) != 0x10);
	result = ADCW;
	
	if(ADC_flag == 1) // not new start
	{
		ADC_buffer[ADC_index] = result;
		ADC_index++;
		ADC_index = ADC_index % 16;
	}
	else		      // new start
	{
		for(i=0; i<16; i++) 
		{
			ADC_buffer[i] = result;
		}
		ADC_flag = 1;
	}
	
	result = 0;	      // clear
	for(i=0; i<16; i++)
	{
		result += ADC_buffer[i];
	}
	result = result >> 4; // result/16
	return result;
}

void Restart_reset(void)
{
  sbi(TCCR2,5);		// Activate OC2 pin
  ms_count = 0;
  sec_count = 0;
  min_count = 0;
  light_level = 2;
  auto_light_mode = 0;
  timer_mode = 0;
  OCR2 = 120;
  hys_flag = 0;
  LED_mode = 1;
}

void LED_off(void)
{
  LED_mode = 0;
  cbi(TCCR2,5);		// Deactivate OC2 pin
  TIMSK |= (0<<OCIE0);	// Deactivate Compare match interrupt
}

void Blink(U08 num)
{
  for(int j=0; j<num; j++)
  {
    OCR2 = 0;
    Delay_ms(300);
    OCR2 = now;
    Delay_ms(300);
  }
}
