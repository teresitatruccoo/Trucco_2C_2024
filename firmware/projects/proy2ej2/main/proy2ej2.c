/*! @mainpage Proyecto 2 Ejercicio 2
 *
 * @section genDesc General Se resuelve el proyecto de medicion de distancia por ultrasonido con interrupciones y timers.
 *
 *
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
 * | 18/09/2023 | Document creation		                         |
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
#include "timer_mcu.h"

/*==================[macros and definitions]=================================*/
// #define CONFIG_BLINK_PERIOD_TECLAS 200 //Intervarlo entre lectura de teclas
// #define CONFIG_BLINK_PERIOD_MEDICION 1000 //Intervalo de actualizacion de la medicion del sensor
#define CONFIG_BLINK_PERIOD_MEDICION_US 1000000 // En microsegundos

/*==================[internal data definition]===============================*/
bool hold = false;					   // Variable de estado para mantener el ultimo valor
bool medir = true;					   // Activa o no la medicion
uint16_t valor_medicion = 0;		   // Distancia medida
TaskHandle_t medir_task_handle = NULL; // Inicializador de tarea
TaskHandle_t mostrar_task_handle = NULL;

/*==================[internal functions declaration]=========================*/
void FuncTimerA(void *param)
{
	vTaskNotifyGiveFromISR(medir_task_handle, pdFALSE);
	vTaskNotifyGiveFromISR(mostrar_task_handle, pdFALSE);
}

/**
 * @brief Funcion encargada de controlar los LEDs segun el valor de valor_medicion
 *
 * Si valor_medicion es menor a 10 cm, los LEDs estan apagados. Si es mayor a 10 y menor a 20 cm, solo el LED_1 esta encendido.
 * Si es mayor a 20 y menor a 30 cm, los LEDs LED_1 y LED_2 estan encendidos. Si es mayor a 30 cm, los tres LEDs estan encendidos.
 */
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
 * @param
 * @param
 * @param
 *
 */
static void medicion(void *pvParameter)
{

	while (true)
	{
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY); // Manda notificacion
		if (medir)
		{
			valor_medicion = HcSr04ReadDistanceInCentimeters();
		}
	}
}


/**
 * @brief Muestra la lectura de distancia en los LEDS y en el display.
 *
 * Si medir es true y hold es true, no actualiza el LCD, pero sí los LEDs. Si no está en hold, muestra el valor de valor_medicion en el LCD y ajusta los LEDs.
 * Si medir es false, apaga el LCD y los LEDs.
 * Espera 1000 ms antes de volver a actualizar, alineado con la medición.
 *
 *
 */
static void mostrar(void *pvParameter)
{

	while (true)
	{
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY); // Le llega notificacion
		if (medir)
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
	}
}

/**
 * @brief Función de interrupción para el botón TEC1. Alterna el valor de la variable medir.
 *
 * Al presionar el botón TEC1 se activa esta interrupción, lo que hace alterna el valor de la variable medir.
 * Si medir es true, se inicia la medición y se muestra en el display. Si es false, se detiene la medición y se apaga el display.
 *
 */
void InterrupcionTecla1(void)
{
	medir = !medir;
}
/**
 * @brief Función de interrupción para el botón TEC2. Alterna el valor de la variable hold.
 *
 * Al presionar el botón TEC2 se activa esta interrupción, lo que hace alterna el valor de la variable hold.
 * Si hold es true, se mantiene el valor de la última medicion en el display. Si es false, se actualiza el valor de la medicion en el display.
 *
 */
void InterrupcionTecla2(void)
{
	hold = !hold;
}

/*==================[external functions definition]==========================*/
void app_main(void)
{
	// Inicializo LCD, Switchs y LEDS
	LcdItsE0803Init();
	LedsInit();
	HcSr04Init(GPIO_3, GPIO_2);
	SwitchesInit();

	timer_config_t timer_medicion = {
		.timer = TIMER_A,
		.period = CONFIG_BLINK_PERIOD_MEDICION_US, // De este tiempo a 0.
		.func_p = FuncTimerA,
		.param_p = NULL};

	TimerInit(&timer_medicion);

	SwitchActivInt(SWITCH_1, &InterrupcionTecla1, NULL);
	SwitchActivInt(SWITCH_2, &InterrupcionTecla2, NULL);
	// Se crean las tareas
	xTaskCreate(&mostrar, "mostrar", 512, NULL, 5, &mostrar_task_handle);
	xTaskCreate(&medicion, "medicion", 512, NULL, 5, &medir_task_handle);

	TimerStart(timer_medicion.timer);
}
/*==================[end of file]============================================*/