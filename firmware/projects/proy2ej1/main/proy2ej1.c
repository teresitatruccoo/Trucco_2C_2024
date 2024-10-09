/*! @mainpage Proyecto 2 Ejercicio 1
 *
 * @section genDesc Se realiza el Ej2-Proyecto 2 "medidor de distancia con ultrasonido"
 *
 * En este proyecto el sistema mide la distancia y controla los LEDs según el valor obtenido: apaga los LEDs si es menor a 10 cm, enciende progresivamente LED_1, LED_2 y LED_3 a medida que la distancia aumenta. También muestra la distancia en un display LCD. Se puede iniciar o detener la medición con el botón TEC1, mientras que TEC2 permite mantener el último valor mostrado en pantalla. La medición se actualiza cada segundo.
 *
 * <a href="https://drive.google.com/...">Operation Example</a>
 *
 * @section hardConn Hardware Connection
 *
 * 	|    Peripheral |   ESP32   	|
 * 	|:-------------:|:--------------|
 * 	| 	ECHO	 	| 	GPIO_3		|
 *	| 	TRIGGER	 	| 	GPIO_2		|
 *	| 	+5V	 		| 	+5V			|
 * 	| 	GND 		| 	GND			|
 * @section changelog Changelog
 *
 * |   Date	    | Description                                    |
 * |:----------:|:-----------------------------------------------|
 * | 12/09/2023 | Document creation		                         |
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
#include "hc_sr04.h"
#include "lcditse0803.h"

/*==================[macros and definitions]=================================*/
/** @def CONFIG_BLINK_PERIOD_TECLAS
 * @brief Periodo de interrupcion de teclas
 */
#define CONFIG_BLINK_PERIOD_TECLAS 200	  // Intervarlo entre lectura de teclas
/** @def CONFIG_BLINK_PERIOD_MEDICION
 * @brief Periodo de actualizacion de la medicion
 */
#define CONFIG_BLINK_PERIOD_MEDICION 1000 // Intervalo de actualizacion de la medicion del sensor

/*==================[internal data definition]===============================*/
bool hold = false;			 // Variable de estado para mantener el ultimo valor
bool medir = true;			 // Activa o no la medicion
uint16_t valor_medicion = 0; // Distancia medida

/*==================[internal functions declaration]=========================*/
void Controlar_Leds(void)
{

	if (valor_medicion < 10)
	{
		LedOff(LED_1);
		LedOff(LED_2);
		LedOff(LED_3);
	}
	else
	{
		if (valor_medicion > 10 && valor_medicion < 20)
		{
			LedOn(LED_1);
			LedOff(LED_2);
			LedOff(LED_3);
		}
		else
		{
			if (valor_medicion > 20 && valor_medicion < 30)
			{
				LedOn(LED_1);
				LedOn(LED_2);
				LedOff(LED_3);
			}
			else
			{
				if (valor_medicion > 30)
				{
					LedOn(LED_1);
					LedOn(LED_2);
					LedOn(LED_3);
				}
			}
		}
	}
}

/**
 * @brief Mide la distancia con el sensor ultrasónico HC-SR04
 *
 * Si medir es true llama a la funcion ReadDistanceInCentemeter() para obtener el valor de la medicion
 * y almacenarlo en valor_medicion. Luego espera 1000ms para actualizar la medicion cada este tiempo
 *
 *
 */
static void medicion(void *pvParameter)
{

	while (true)
	{
		if (medir == true)
		{
			valor_medicion = HcSr04ReadDistanceInCentimeters();
		}
		vTaskDelay(CONFIG_BLINK_PERIOD_MEDICION / portTICK_PERIOD_MS);
	}
}


/**
 * @brief Muestra la lectura de distancia en los LEDS y en el display.
 *
 * Si medir es true y hold es true, no actualiza el LCD, pero sí los LEDs. Si no está en hold, muestra el valor de valor_medicion en el LCD y ajusta los LEDs.
 * Si medir es false, apaga el LCD y los LEDs.
 * Espera 1000 ms antes de volver a actualizar, alineado con la medición.
 *
 */
static void mostrar(void *pvParameter)
{

	while (true)
	{

		if (medir && hold)
		{
			Controlar_Leds();

			if (!hold)
			{
				LcdItsE0803Write(valor_medicion);
			}
		}
		else
		{
			LcdItsE0803Off();
			LedOff(LED_1);
			LedOff(LED_2);
			LedOff(LED_3);
		}

		vTaskDelay(CONFIG_BLINK_PERIOD_MEDICION / portTICK_PERIOD_MS);
	}
}
/**
 * @brief Maneja las teclas para activar o detener la medicion y para mantener el valor medido
 *
 * Lee el estado de los botones usando SwitchesRead().
 * Si se presiona SWITCH_1 (TEC1), alterna la variable medir para iniciar o detener la medición.
 * Si se presiona SWITCH_2 (TEC2), alterna la variable hold para mantener o no el último valor.
 * La tarea se ejecuta cada 200 ms
 *
 *
 */
static void teclas(void *pvParameter)
{
	uint8_t teclas;
	while (true)
	{
		teclas = SwitchesRead();
		switch (teclas)
		{
		case SWITCH_1:
			medir = !medir;

			break;
		case SWITCH_2:
			hold = !hold;
			break;
		}
		vTaskDelay(CONFIG_BLINK_PERIOD_TECLAS / portTICK_PERIOD_MS);
	}
}

/*==================[external functions definition]==========================*/
void app_main(void)
{
	// Inicializo LCD, Switchs y LEDS
	LcdItsE0803Init();
	LedsInit();
	HcSr04Init(GPIO_3, GPIO_2);
	SwitchesInit();
	// Se crean las tareas
	xTaskCreate(&teclas, "teclas", 512, NULL, 5, NULL);
	xTaskCreate(&mostrar, "mostrar", 512, NULL, 5, NULL);
	xTaskCreate(&medicion, "medicion", 512, NULL, 5, NULL);
}
/*==================[end of file]============================================*/
