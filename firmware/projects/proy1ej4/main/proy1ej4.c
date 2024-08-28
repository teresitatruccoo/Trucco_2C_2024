/*! @mainpage Proyecto 1 Ejercicios 4 5 y 6.
 *
 * @section genDesc Se realizan los ejercicios 4 5 y 6 del Proyecto 1 en este mismo proyecto.
 *
 * This section describes how the program works.
 *
 * <a href="https://drive.google.com/...">Operation Example</a>
 *
 * @section hardConn Hardware Connection
 *
 * |    Peripheral  |   ESP32   	|
 * |:--------------:|:--------------|
 * | 	PIN_X	 	| 	GPIO_X		|
 *
 *
 * @section changelog Changelog
 *
 * |   Date	    | Description                                    |
 * |:----------:|:-----------------------------------------------|
 * | 14/08/2024 | Document creation		                         |
 *
 * @author Teresita Trucco (teresita.trucco@ingenieria.uner.edu.ar)
 *
 */

/*==================[inclusions]=============================================*/
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led.h"
#include "switch.h"
#include "gpio_mcu.h"

/*==================[macros and definitions]=================================*/

/*==================[internal data definition]===============================*/
typedef struct
{
	gpio_t pin;			/*!< GPIO pin number */
	io_t dir;			/*!< GPIO direction '0' IN;  '1' OUT*/
} gpioConf_t;


/*==================[internal functions declaration]=========================*/
uint8_t convertToBcdArray(uint32_t data, uint8_t digits, uint8_t *bcd_number)
{
	for (uint8_t i = 0; i < digits; i++)
	{
		bcd_number[i] = 0;
	}

	// Convertir cada dígito del número en BCD
	for (int8_t i = digits - 1; i >= 0; i--)
	{
		uint8_t digit = data % 10; // Extrae el dígito menos significativo
		bcd_number[i] = digit;    // Almacena el dígito en el arreglo BCD
		data /= 10; // Eliminar el dígito menos significativo del número, ES NECESARIO? si
	}
	return 0;
}

void decimalToBCD(uint8_t decimal, uint8_t *numBCD)
{
	for (uint8_t i = 0; i < 4; i++)
	{
		numBCD[i] = decimal % 2;    
		decimal /= 2; 
	}
}
void mapeoGPIO(uint8_t *numBCD, gpioConf_t *arregloGPIOS)
{
	for (uint8_t j = 0; j < 4; j++)
	{
		if (numBCD[j]) //True por defecto
		{
			GPIOOn(arregloGPIOS[j].pin);
		}
		else
		{
			GPIOOff(arregloGPIOS[j].pin);
		}
	}
}

void mostrarNumeroEnDisplay(uint32_t data, uint8_t digits, gpioConf_t *bcd_pins, gpioConf_t *digit_pins) 
{
	uint8_t bcd_array[digits]; //Almacena digitos BCD
	
	convertToBcdArray(data, digits, bcd_array);
	
	for (uint8_t i = 0; i < digits; i++)
	{
		// Convertir el dígito a su formato BCD binario
		uint8_t bcd_bits[4];
		decimalToBCD(bcd_array[i], bcd_bits);
		
		mapeoGPIO(bcd_bits, bcd_pins);
		
		//Armo el puslo
		GPIOOn(digit_pins[i].pin);
				
		GPIOOff(digit_pins[i].pin);
	}
}



/*==================[external functions definition]==========================*/
void app_main(void){
	uint32_t number = 321;   // Dato de 32 bits
    uint8_t digits = 3;        // Cantidad de dígitos a convertir
    uint8_t bcd_array[10];     // Arreglo donde se almacenarán los dígitos BCD
	
    convertToBcdArray(number, digits, bcd_array);

    // Imprimir los valores en formato BCD
    printf("Numero BCD: ");
    for (uint8_t i = 0; i < digits; i++)
    {
        printf("%d", bcd_array[i]); // Imprimir cada dígito BCD. %d permite printear un numero, por esto recorro con un for y lo uso.
    }
    printf("\n");

	//Ejercicio 5
	uint8_t decimal = 1;
    uint8_t numBCD[4];

    decimalToBCD(decimal, numBCD);


	gpioConf_t arregloGPIOS[]=
	{
		{GPIO_20, GPIO_OUTPUT}, //Defino todos los GPIO como salida
		{GPIO_21, GPIO_OUTPUT},
		{GPIO_22, GPIO_OUTPUT},
		{GPIO_22, GPIO_OUTPUT},
	};
	
	GPIOInit(GPIO_20, GPIO_OUTPUT); //Inicializo todos los GPIO
	GPIOInit(GPIO_21, GPIO_OUTPUT);
	GPIOInit(GPIO_22, GPIO_OUTPUT);
	GPIOInit(GPIO_23, GPIO_OUTPUT);

	mapeoGPIO(numBCD, arregloGPIOS);


	//Ejercicio 6

	gpioConf_t bcd_pins[] =
	{
		{GPIO_20, GPIO_OUTPUT}, // Pines para BCD como salida
		{GPIO_21, GPIO_OUTPUT},
		{GPIO_22, GPIO_OUTPUT},
		{GPIO_23, GPIO_OUTPUT},
	};

	// Definición de los GPIOs para los dígitos del display
	gpioConf_t digit_pins[] =
	{
		{GPIO_19, GPIO_OUTPUT}, // Pines para los dígitos del display
		{GPIO_18, GPIO_OUTPUT},
		{GPIO_9, GPIO_OUTPUT},
	};
	
	// Inicializar los pines 
	for (uint8_t i = 0; i < 4; i++) {
		GPIOInit(bcd_pins[i].pin, bcd_pins[i].dir);
	}
	
	for (uint8_t i = 0; i < digits; i++) {
		GPIOInit(digit_pins[i].pin, digit_pins[i].dir);
	}
	
	// Mostrar el número en el display
	mostrarNumeroEnDisplay(number, digits, bcd_pins, digit_pins);

}
/*==================[end of file]============================================*/


