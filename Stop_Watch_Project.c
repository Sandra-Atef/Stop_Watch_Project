/*
 ============================================================================
 Name        : Stop_Watch_Project
 Author      : Sandra Atef Kamal Kamel
 Date        : 11/8/2024
 Description : Implementation of a digital stopwatch with two operational modes:
               counting up (increment mode)and counting down (countdown mode).
               The project utilizes an ATmega32 microcontroller,
               six multiplexed seven-segment displays (common anode),
               and multiple push buttons for user interaction.
 ============================================================================
 */

#include<avr/io.h>
#include<avr/interrupt.h>
#include<util/delay.h>

/*global variable to indicate the mode (count up or count down)*/
unsigned char mode=0;
/* global variables contain hours, minutes, seconds*/
unsigned char hr=0,min=0,sec=0;
/* Description:
 * For System Clock = 16Mhz and prescaler F_CPU/1024.
 * Timer frequency will be equal 15625hz, Ttimer = 64us.
 * For compare value equals to 15625 the timer will generate compare match interrupt every 1sec.
 */
void Timer1_Init_CTC_Mode(void){
	TCNT1 = 0;   /*Set Timer initial value to 0*/

	OCR1A = 15625; /*Set Compare Value*/

	/* Configure timer control register TCCR1A
	 * 1. Disconnect OC1A and OC1B  COM1A1=0 COM1A0=0 COM1B0=0 COM1B1=0
	 * 2. FOC1A=1 FOC1B=0
	 * 3. CTC Mode WGM10=0 WGM11=0
	 */
	TCCR1A = (1<<FOC1A);

	/* Configure timer control register TCCR1B
	 * 1. CTC Mode WGM12=1 WGM13=0
	 * 2. Prescaler = F_CPU/1024 CS10=1 CS11=0 CS12=1
	 */
	TCCR1B = (1<<WGM12)|(1<<CS10)|(1<<CS12);

	TIMSK |= (1<<OCIE1A); /* Enable Timer1 Compare A Interrupt */

	SREG |= (1<<7);    /*Enable global interrupts in MC.*/
}

ISR(TIMER1_COMPA_vect){
	/*Count Down Mode*/
	if(mode==1)
	{
		if (sec == 0 && min == 0 && hr == 0)
		{
			PORTD |= (1<<PD0);
		}
		else if (sec == 0 && min == 0) {
			sec = 59;
			min = 59;
			hr--;
		}
		else if (sec == 0) {
			min--;
			sec = 59;
		}
		else {
			sec--;
		}
	}
	/*Count Up Mode*/
	else if(mode==0)
	{
		sec++;
		if(sec == 60){
			sec = 0;
			min++;
		}
		if(min == 60){
			sec = 0;
			min = 0;
			hr++;
		}
		if(hr == 24){
			sec = 0;
			min = 0;
			hr = 0;
		}
	}
}

/* External INT0 enable and configuration function */
void Reset_INT0_INIT(void){
	DDRD &= ~(1<<PD2);    /* Configure INT0/PD2 as input pin*/
	PORTD |= (1<<PD2); /* Activate internal pull up resistor*/
	MCUCR |= (1<<ISC01);  /*Trigger INT0 with the falling edge*/
	GICR |= (1<<INT0);     /*Enable external interrupt pin INT0*/
	SREG |= (1<<7);        /* Enable interrupts by setting I-bit*/
}

/* External INT0 Interrupt Service Routine */
ISR(INT0_vect)
{

	if(mode==1 && hr==0 && min==0 && sec==0){
		PORTD &= ~(1<<PD0); /*turn off the buzzer*/
		mode=0;        /*back to count up mode*/
		PORTD |= (1 << PD4); /* Turn on red led*/
		PORTD &= ~(1 << PD5);/* Turn off yellow led*/
	}

	hr = 0;
	min = 0;
	sec = 0;

}

/* External INT1 enable and configuration function */
void Pause_INT1_INIT(void){
	DDRD &= ~(1<<PD3);    /* Configure INT1/PD3 as input pin*/
	MCUCR |= (1<<ISC11)|(1<<ISC10);  /*Trigger INT1  with the rising edge*/
	GICR |= (1<<INT1); /*Enable external interrupt pin INT1*/
	SREG |= (1<<7);  /* Enable interrupts by setting I-bit*/
}

/* External INT1 Interrupt Service Routine */
ISR(INT1_vect){
	TCCR1B &= 0XF8; /* Stop the Timer*/
}

/* External INT2 enable and configuration function */
void Resume_INT2_INIT(void){
	DDRB &= ~(1<<PB2);    /* Configure INT2/PB2 as input pin*/
	PORTB |= (1<<PB2); /* Activate internal pull up resistor*/
	MCUCSR &= ~(1<<ISC2);  /*Trigger INT2  with the falling edge*/
	GICR |= (1<<INT2); /*Enable external interrupt pin INT2*/
	SREG |= (1<<7);  /* Enable interrupts by setting I-bit*/
}

/* External INT2 Interrupt Service Routine */
ISR(INT2_vect){
	Timer1_Init_CTC_Mode(); /*Resume the timer*/
}

int main(void){

	unsigned char flag=0;

	DDRC |= 0x0F;   /* Configure the first four pins in PORTC as output pins*/

	PORTC &= 0xF0;  /* Initialize the 7-seg display zero at the beginning*/

	DDRA |= 0x3F;   /* Configure the first six pins in PORTA as output pins*/

	PORTA &= 0xC0;   /*Disable all seven-segments at the beginning*/

	DDRD |= 0x31;    /* Configure pin 0,4,5 in PORTD as output pins (two leds and the buzzer are off)*/

	PORTD |= (1<<PD4); /* Set PD4 turn on at the beginning (Red Led ON)*/

	PORTD &= ~(1<<PD5); /* Set PD5 turn off at the beginning (Yellow Led OFF)*/

	PORTD &= ~(1<<PD0); /* Set PD0 turn off at the beginning (Buzzer OFF)*/

	DDRB &=~ (1<<PB7);  /* Configure pin 7 in PORTB as input pin (Toggle button)*/

	PORTB |= (1<<PB7); /* Activate internal pull up resistor of Toggle button*/

	DDRB &= 0x84;  /* Configure pin 0,1,3,4,5,6 in PORTB as input pins(Adjustment buttons)*/

	PORTB |= 0x7B; /* Activate internal pull up resistors of Adjustment buttons*/

	Timer1_Init_CTC_Mode(); /*Start the timer*/
	Reset_INT0_INIT();
	Pause_INT1_INIT();
	Resume_INT2_INIT();

	while(1){
		/*Enable the segement that display units of seconds*/
		PORTA=(PORTA & 0xC0)|(1<<PA5);
		PORTC=(sec%10);
		_delay_ms(2);

		/*Enable the segement that display tens of seconds*/
		PORTA = (PORTA & 0xC0)|(1<<PA4);
		PORTC = (sec/10);
		_delay_ms(2);

		/*Enable the segement that display units of minutes*/
		PORTA = (PORTA & 0xC0)|(1<<PA3);
		PORTC = (min%10);
		_delay_ms(2);

		/*Enable the segement that display tens of minutes*/
		PORTA = (PORTA & 0xC0)|(1<<PA2);
		PORTC = (min/10);
		_delay_ms(2);

		/*Enable the segement that display units of hours*/
		PORTA = (PORTA & 0xC0)|(1<<PA1);
		PORTC = (hr%10);
		_delay_ms(2);

		/*Enable the segement that display tens of hours*/
		PORTA = (PORTA & 0xC0)|(1<<PA0);
		PORTC = (hr/10);
		_delay_ms(2);

		/*Mode change*/
		if(!(PINB & (1<<PB7)))
		{
			_delay_ms(30);
			if(!(PINB & (1<<PB7)))
			{
				if(flag==0)
				{
					PORTD ^= (1 << PD5);
					PORTD ^= (1 << PD4);
					mode = !(mode);
					flag = 1;
				}
			}
		}

		/*Increment Hours*/
		else if(!(PINB & (1<<PB1)))
		{
			_delay_ms(30);
			if(!(PINB & (1<<PB1)))
			{
				if(flag==0){
					if(hr==23)
					{
						hr=0;
					}
					else
					{
						hr++;
					}
					flag=1;
				}
			}
		}

		/*Decrement Hours*/
		else if(!(PINB &(1<<PB0)))
		{
			_delay_ms(30);
			if(!(PINB &(1<<PB0)))
			{
				if(flag==0){
					if(hr==0)
					{
						hr=23;
					}
					else
					{
						hr--;
					}
					flag=1;
				}
			}
		}

		/*Increment Minutes*/
		else if(!(PINB &(1<<PB4)))
		{
			_delay_ms(30);
			if(!(PINB &(1<<PB4)))
			{
				if(flag==0){
					if(min==59)
					{
						min=0;
					}
					else
					{
						min++;
					}
					flag=1;
				}
			}
		}

		/*Decrement Minutes*/
		else if(!(PINB &(1<<PB3)))
		{
			_delay_ms(30);
			if(!(PINB &(1<<PB3)))
			{
				if(flag==0){
					if(min==0)
					{
						min=59;
					}
					else
					{
						min--;
					}
					flag=1;
				}
			}
		}

		/*Increment Seconds*/
		else if(!(PINB &(1<<PB6)))
		{
			_delay_ms(30);
			if(!(PINB &(1<<PB6)))
			{
				if(flag==0){
					if(sec==59)
					{
						sec=0;
					}
					else
					{
						sec++;
					}
					flag=1;
				}
			}
		}

		/*Decrement Seconds*/
		else if(!(PINB &(1<<PB5)))
		{
			_delay_ms(30);
			if(!(PINB &(1<<PB5)))
			{
				if(flag==0){
					if(sec==0)
					{
						sec=59;
					}
					else
					{
						sec--;
					}
					flag=1;
				}
			}
		}
		else
		{
			flag=0;
		}

	}
}

