/*
 * Made by Tsz Shing Tsoi
 * For FAU CDA3331C - Design Project: Car Parking Sensor System
 * Date: 04-21-2019
 * 
 * This programs implements the car parking sensor system on PIC16F18855 microcontroller
 * connected with a HC-SR04 Ping sensor, a NeoPixel LED, and a SG90 servo motor.
 * 
 * Pin connections for 4-pin Ping HC-SR04 sensor:
 * Vcc=5V
 * Echo=RC5
 * Trig=RC6
 * Ground = GND
 * 
 * Pin connections for NeoPixel LED:
 * Data in = RB5
 * Vcc = 3.3V
 * Ground = GND
 * Data out is not connected
 *  
 * Pin connections for SG90 servo motor:
 * Control pin = RC7
 * Vcc = 5V
 * Ground = GND
 */

#include "mcc_generated_files/mcc.h"
#define RED 0  //index of Red color in the color[] array
#define GREEN 3  //index of Green color in the color[] array
#define YELLOW 6  //index of Yellow color in the color[] array
#define DC_LOW 95  //duty cycle corresponding to the -90-degree position
#define DC_HIGH 645  //duty cycle corresponding to the +90-degree position
#define DELAY 60  //delay in ms between updating object distance and Neopixcel LED color
                              //and servo motor position

/*
                         Main application
 */

/**
 * Trigger Ping sensor and read Echo duration, and return the object distance in cm.
 * @return 
 * Return object distance in cm.
 */
float getDistance() {
    TRIG_SetHigh(); // sends trigger pulse
    __delay_us(5);
    TRIG_SetLow();
    TMR1GIF = 0; //resets timer
    TMR1_WriteTimer(0);
    TMR1_StartSinglePulseAcquisition();
    while (!TMR1GIF); // waits for return pulse to end
    return TMR1_ReadTimer() / 116.0; //math for cm
}
/**
 * Send the 24-bit bitstream representing RGB color through the SPI module for Neopixel LED.
 * @param p
 * Pointer to an array containing values for RGB. Length of the array should be multiples of 3.
 * @param count
 * Number of Neopixel LED colors to be sent through the SPI module.
 */
void NeoPixel_Stream(uint8_t *p, uint8_t count) { // sends count x RGB data packets (24-bit each)
    uint8_t bitCount, data;
    while (count--) {
        bitCount = 24;
        do {
            if ((bitCount & 7) == 0)
                data = *p++;
            SSP1BUF = ((data & 0x80)) ? 0xFE : 0xC0; // WS2812B 900ns - 350ns           
            data <<= 1;
        } while (--bitCount);
    }
}
/**
 * Return smaller of the two input values.
 * @param val
 * Input value 1 for comparison.
 * @param otherVal
 * Input value 2 for comparison.
 * @return 
 * Smaller of val and otherVal.
 */
inline float min(float val, float otherVal) {
    return (val <= otherVal) ? val : otherVal;
}
/**
 * Return larger of the two input values.
 * @param val
 * Input value 1 for comparison.
 * @param otherVal
 * Input value 2 for comparison.
 * @return 
 * Larger of val and otherVal.
 */
inline float max(float val, float otherVal) {
    return (val >= otherVal) ? val : otherVal;
}

void main(void) {
    // initialize the device
    SYSTEM_Initialize();
    TRIG_SetLow(); //set trigger low to being pulse

    float distance = getDistance();  //object distance in cm
    uint8_t color[] = {0xFF, 0, 0, 0, 0xFF, 0, 0xFF, 0xFF, 0, 0, 0, 0}; //color matrix: Red, Green, Yellow and OFF

    while (1) {
        if (distance <= 10) {  //imminent collision
            do {
                NeoPixel_Stream(&color[RED], 1);
                PWM6_LoadDutyValue(DC_HIGH);
                __delay_ms(DELAY);
                distance = getDistance();
            } while (distance <= 12); //change from red to yellow at 12cm
        } else if (distance > 30) {  //no possible collision
            NeoPixel_Stream(&color[GREEN], 1);
            PWM6_LoadDutyValue(DC_LOW);
            __delay_ms(DELAY);
            distance = getDistance();
        } else { // 10 < distance <=30, possible collision
            do {
                distance = max(min(distance, 30), 10); //limit the distance to be between 10 and 30
                T4PR = (uint8_t)(((distance - 10) * 224) / 20 + 31); //interpolate between T4PR values of 0 and 255
                                                                                 //resulting toggling frequency is ~1Hz at 30cm, and ~8Hz at 11cm
                NeoPixel_Stream(&color[YELLOW + CLC1CONbits.LC1OUT * 3], 1);  //output Yellow if toggle OFF,
                                                                                                                         //otherwise Blank if toggle ON
                //interpolate duty value between DC_LOW and DC_HIGH
                PWM6_LoadDutyValue((uint16_t)((30 - distance)* (DC_HIGH - DC_LOW) / 20 + DC_LOW));
                __delay_ms(DELAY);
                distance = getDistance();
            } while (distance > 10 && distance <= 32);  //change from yellow to red at 10cm, yellow to green at 32cm
        }
    }
}

/**
 End of File
 */