/*  -------------------------------------------
Student ID: ec18446 
Lab 5
    1. The ADC inputs are
      - Single: ADC0_SE8 (PTB0)
    2. Measures voltage when button pressed
      - Red LED shows first single measurement
      - Green LED shows second measurement
			- After first two button pressed, possible to take more measurements

    The system uses code from 
       adc.c   - provides 4 functions for using the ADC
       SysTick.c  - provides 2 functions for using SysTick
       gpio.c - provide code to initialise and use GPIO button and LED
			
		3. Multimeter readings used in code:
			- maximum voltage: 2.98V with multimeter, thus 2980mV.
			- minimum voltage: 0.008V with multimeterm, thus 8mV.
			
			Further readings of resistor and potentiometer in series, not used in code:
			- resistor: 0.98 kOhms 	
			- potentiometer Max: 9.99 kOhms					  
			- potentionmeter Min: 0.002 kOhms	
		
		4. Following function added:
			- task2FlaashLED: It takes voltage measurements, determines time on/off of blue LED
				and flashes blue LED with a period (time on + time off) of 4s.
------------------------------------------- */
#include <MKL25Z4.H>
#include <stdbool.h>
#include <stdint.h>

#include "..\include\gpio_defs.h"
#include "..\include\adc_defs.h"
#include "..\include\SysTick.h"


/*----------------------------------------------------------------------------
    Task 1: checkButton

  This tasks polls the button and signals when it has been pressed
*----------------------------------------------------------------------------*/
int buttonState ; // current state of the button
int bounceCounter ; // counter for debounce
bool pressed ; // signal if button pressed

void init_ButtonState() {
    buttonState = BUTTONUP ;
    pressed = false ; 
}

void task1ButtonPress() {
    if (bounceCounter > 0) bounceCounter-- ;
    switch (buttonState) {
        case BUTTONUP:
            if (isPressed()) {
                buttonState = BUTTONDOWN ;
                pressed = true ; 
            }
          break ;
        case BUTTONDOWN:
            if (!isPressed()) {
                buttonState = BUTTONBOUNCE ;
                bounceCounter = BOUNCEDELAY ;
            }
            break ;
        case BUTTONBOUNCE:
            if (isPressed()) {
                buttonState = BUTTONDOWN ;
            }
            else if (bounceCounter == 0) {
                buttonState = BUTTONUP ;
            }
            break ;
    }                
}





/*--------------------------------------------------------------------------------------------------------
		Task 2: flashLED
		
		Measure the voltage produced by a potentiometer and determine the time on and time off of the blue LED
----------------------------------------------------------------------------------------------------------*/
// declare volatile to see in debug mode
volatile float vM1;				// store first measurement value
volatile float vM2;				// store second measurement value
volatile float vIn;				// store new measurement value
volatile int ledTimeOn;		// blue LED time on
volatile int ledTimeOff;	// blue LED time off

int vMax = 2980;    			// Measured 2.98V, thus 2980mV
int vMin = 8;       			// Measured 0.008V, thus 8mV
bool finished;		  			// start/stop flashing flag
			
#define MFIRST 1					// first measurement
#define MSECOND 2					// second measurement
#define MNEW 3						// new measurement
int measureState ;				// current measure state

void Init_MeasureState(void) {
    measureState = MFIRST ;
    greenLEDOnOff(LED_OFF) ;
    redLEDOnOff(LED_ON) ;  
}

int blueCounter;	// this stores the integer value of the blue LED time on and time off 
int blueLEDState;	// blue LED is ON or OFF
void init_blueLED(void)
{
    blueLEDOnOff(LED_OFF);	// blue LED off
    blueLEDState = LED_OFF;	// initial state is Off
}

void task2flashLED() {
	// Stage 1: Take measurements and determine time on/off of blue LED 
		switch(measureState)
		{
			case MFIRST:
				if (pressed)
				{
					pressed = false;	// acknowledge event
					
					MeasureVoltage();	// take a simple-ended voltage reading
					vM1 = 1000 * (VREF * sres) / ADCRANGE ;  // Get voltage in mV
					
					vIn = vM1;
					
					measureState = MSECOND;	// Take second measurement
					
					redLEDOnOff(LED_OFF);		// red LED Off
					greenLEDOnOff(LED_ON);	// green LED On
				}
				break;
				
			case MSECOND:
				if (pressed)
				{
					pressed = false;	// acknowledge event
					
					MeasureVoltage(); // take a simple-ended voltage reading
					vM2 = 1000 * (VREF * sres) / ADCRANGE ;  // voltage in mV
					
					measureState = MNEW;	// Take a new measurement
					
					redLEDOnOff(LED_OFF);		// red LED Off
					greenLEDOnOff(LED_OFF);	// green LED Off
					
					if (vM1 > vM2) 
					{
						ledTimeOn = 500;	// 0.5s
						ledTimeOff = 500;	// 0.5s 
					}
					else
					{
						ledTimeOn = 3800;	// Maximum on time 3800ms
						ledTimeOff= 200;	// Maximum off time 200ms
					}
					finished = true;	// start flashing
				} 
				break;
			case MNEW:
				if (pressed)
				{
					finished = false;	// stop flashing
					blueLEDOnOff(LED_OFF);	// blue LED Off
					
					pressed = false;	// acknowledge event
					
					MeasureVoltage();	// Measure voltage
					vIn = 1000 * (VREF * sres) / ADCRANGE ;  // voltage in mV
					
					ledTimeOn = ((vIn-vMin)/(vMax-vMin) * 3600) + 200;	// calculate time on
					ledTimeOff= 4000 - ledTimeOn; // calculate time off
					
					finished = true;	// restart flashing
				}
				break;
		}
		
		// Stage 2: Flash blue LED with a counter. Computed time on/off used to determine counter initial value
		
		if (!finished) return;	// wait until conversion, time on/off calculations finished
		
		if (blueCounter > 0) blueCounter--;	// decrement counter
		
		switch(blueLEDState)
		{
			case LED_ON:
				if(blueCounter == 0)
				{
					blueLEDOnOff(LED_OFF);	// blue LED Off
					blueLEDState = LED_OFF;	// current state is Off
					blueCounter = ledTimeOff/10;	// e.g time off = 3800ms/10 = 380 - since cycle time is 10ms
				}
				break;
			
			case LED_OFF:
				if(blueCounter == 0)
				{
					blueLEDOnOff(LED_ON);	// blue LED On
					blueLEDState = LED_ON;	// current state is On
					blueCounter = ledTimeOn/10; // e.g time on = 3800ms/10 = 380 - since cycle time is 10ms
				}
				break;
		}
}
/*----------------------------------------------------------------------------
																MAIN function
 *----------------------------------------------------------------------------*/
volatile uint8_t calibrationFailed ; // zero expected
int main (void) {
    // Enable clock to ports B, D and E
    SIM->SCGC5 |= SIM_SCGC5_PORTB_MASK | SIM_SCGC5_PORTD_MASK | SIM_SCGC5_PORTE_MASK ;

    init_LED() ; // initialise LED
    init_blueLED(); // initialise blue LED
    init_ButtonGPIO() ; // initialise GPIO input
    init_ButtonState() ; // initialise button state variables
    Init_ADC() ; // Initialise ADC
    calibrationFailed = ADC_Cal(ADC0) ; // calibrate the ADC 
    while (calibrationFailed) ; // block progress if calibration failed
    Init_ADC() ; // Reinitialise ADC
    Init_MeasureState() ;  // Initialise measure state 
    Init_SysTick(1000) ; // initialse SysTick every 1ms
    waitSysTickCounter(10) ;

    while (1) {        
        task1ButtonPress() ;	// Poll button presses
        task2flashLED();			// Flash blue LED
        // delay
        waitSysTickCounter(10) ;  // cycle every 10 ms
    }
}
