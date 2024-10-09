/*! @mainpage Proyecto 2 Ejercicio 4
 *
 * @section genDesc A partir de una entrada analogica por el canal CH1 se leen los valores de la misma y se envia los valores por UART para visualizarlos.
 * Finalmente se reproduce una señal de ECG originalmente digital a traves de una salida analogica, que posteriormente ingresara a la entrada analogica
 * por CH1. El objetivo de este proyecto es visualizar la señal de ECG en el osciloscopio Serial Oscilloscope.
 *
 *
 * <a href="https://drive.google.com/...">Operation Example</a>
 *
 * @section hardConn Hardware Connection
 *
 * 	|    Peripheral |   ESP32   	|
 * 	|:-------------:|:--------------|
 *	| 	Pin1	 	| 	CH0			|
 *	| 	Pin2	 	| 	CH1			|
 * 	| 	Pin3 		| 	GND			|
 * 
 * @section changelog Changelog
 *
 * |   Date	    | Description                                    |
 * |:----------:|:-----------------------------------------------|
 * | 25/09/2023 | Document creation		                         |
 *
 * @author Teresita Trucco (teresita.trucco@ingenieria.uner.edu.ar)
 *
 */

/*==================[inclusions]=============================================*/
#include <stdio.h>
#include <stdint.h>
#include "stdint.h"
#include "stdint.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "timer_mcu.h"
#include "uart_mcu.h"
#include "analog_io_mcu.h"
/*==================[macros and definitions]=================================*/

/*==================[internal data definition]===============================*/
/** @def BUFFER_SIZE
 * @brief Tamaño del buffer de la señal ECG
 */
#define BUFFER_SIZE 231
/** @def CONFIG_BLINK_PERIOD_MEDICION
 * @brief Periodo de muestreo equivalente a Fm:500Hz
 */
#define CONFIG_BLINK_PERIOD_medicion 2000		  // Frecuencia de muestreo requerida para la medicion (500 conversiones por segundo, 2ms). C/2ms se aciva la medicion
/** @def CONFIG_BLINK_PERIOD_reproduccion_ECG
 * @brief Periodo para la transmision de datos
 */
#define CONFIG_BLINK_PERIOD_reproduccion_ECG 4000 // Periodo timer para transmision. Elegido arbitrariamente siempre menor a 2000
TaskHandle_t medicion_task_handle = NULL;		  // Inicializa luego la tarea medicion
TaskHandle_t ECG_task_handle = NULL;			  // Inicializa la tarea reporducir ECG

const char ECG[BUFFER_SIZE] = {
	76,
	77,
	78,
	77,
	79,
	86,
	81,
	76,
	84,
	93,
	85,
	80,
	89,
	95,
	89,
	85,
	93,
	98,
	94,
	88,
	98,
	105,
	96,
	91,
	99,
	105,
	101,
	96,
	102,
	106,
	101,
	96,
	100,
	107,
	101,
	94,
	100,
	104,
	100,
	91,
	99,
	103,
	98,
	91,
	96,
	105,
	95,
	88,
	95,
	100,
	94,
	85,
	93,
	99,
	92,
	84,
	91,
	96,
	87,
	80,
	83,
	92,
	86,
	78,
	84,
	89,
	79,
	73,
	81,
	83,
	78,
	70,
	80,
	82,
	79,
	69,
	80,
	82,
	81,
	70,
	75,
	81,
	77,
	74,
	79,
	83,
	82,
	72,
	80,
	87,
	79,
	76,
	85,
	95,
	87,
	81,
	88,
	93,
	88,
	84,
	87,
	94,
	86,
	82,
	85,
	94,
	85,
	82,
	85,
	95,
	86,
	83,
	92,
	99,
	91,
	88,
	94,
	98,
	95,
	90,
	97,
	105,
	104,
	94,
	98,
	114,
	117,
	124,
	144,
	180,
	210,
	236,
	253,
	227,
	171,
	99,
	49,
	34,
	29,
	43,
	69,
	89,
	89,
	90,
	98,
	107,
	104,
	98,
	104,
	110,
	102,
	98,
	103,
	111,
	101,
	94,
	103,
	108,
	102,
	95,
	97,
	106,
	100,
	92,
	101,
	103,
	100,
	94,
	98,
	103,
	96,
	90,
	98,
	103,
	97,
	90,
	99,
	104,
	95,
	90,
	99,
	104,
	100,
	93,
	100,
	106,
	101,
	93,
	101,
	105,
	103,
	96,
	105,
	112,
	105,
	99,
	103,
	108,
	99,
	96,
	102,
	106,
	99,
	90,
	92,
	100,
	87,
	80,
	82,
	88,
	77,
	69,
	75,
	79,
	74,
	67,
	71,
	78,
	72,
	67,
	73,
	81,
	77,
	71,
	75,
	84,
	79,
	77,
	77,
	76,
	76,
};
/*==================[internal functions declaration]=========================*/

/**
 * @brief Función invocada en la interrupción del timer A
 *
 * La función es invocada en la interrupción del timer A y envía una notificación
 * a la tarea asociada a la medicion.
 *
 * @param param No se utiliza
 */
void FuncTimerA(void *param)
{
	xTaskNotifyGive(medicion_task_handle); // Envía una notificación
}

/**
 * @brief Función invocada en la interrupción del timer B
 *
 * La función es invocada en la interrupción del timer B y envía una notificación
 * a la tarea asociada a la reproducción de la ECG que emitira un valor de ECG por la salida
 * analogica
 *
 * @param param No se utiliza
 */
void FuncTimerB(void *param)
{
	xTaskNotifyGive(ECG_task_handle); // Envía una notificación
}

/**
 * @brief Lee los valores de la entrada analogica del canal CH1 y los envia por UART
 *
 * La tarea espera en este punto hasta recibir una notificación. Luego, lee el valor del canal CH1 y lo envía por UART con un salto de linea al final.
 *
 * @param pvParameter No se utiliza
 */
void signal_Task(void *pvParameter)
{
	uint16_t dato; //12 bits
	while (1)
	{
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);			 // Recibe una notificacion
		AnalogInputReadSingle(CH1, &dato);					 // Se lee el dato del canal 1 y se guarda en "valor"
		UartSendString(UART_PC, (char *)UartItoa(dato, 10)); // Se muestra el dato trasmitiendolo via UART al puerto serie (ver SerialOscilloscope)
		UartSendString(UART_PC, "\r\n");
	}
}

/**
 * @brief Tarea encargada de reproducir la ECG almacenada en el arreglo ECG.
 *
 * La tarea espera en este punto hasta recibir una notificación. Luego, escribe
 * en la salida analógica el valor del arreglo ECG en el índice actual y
 * incrementa el índice. Si el índice es igual al tamaño del arreglo ECG, se
 * reinicia el índice a 0.
 *
 * @param pvParameter No se utiliza
 */
void reproducir_ECG_Task(void *pvParameter)
{
	uint16_t indice = 0;
	while (1)
	{
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY); // Recibe una notificacion
		AnalogOutputWrite(ECG[indice]);			 // Escribe en la salida analogica el valor del indice
		indice++;
		if (indice == sizeof(ECG))
		{
			indice = 0;
		}
	}
}
/*==================[external functions definition]==========================*/
void app_main(void)
{
	timer_config_t timer_medicion_senial = {
		.timer = TIMER_A,
		.period = CONFIG_BLINK_PERIOD_medicion,
		.func_p = FuncTimerA,
		.param_p = NULL};
	timer_config_t timer_reproducir_ECG = {
		.timer = TIMER_B,
		.period = CONFIG_BLINK_PERIOD_reproduccion_ECG,
		.func_p = FuncTimerB,
		.param_p = NULL};
	analog_input_config_t senial_CH1 = {
		.input = CH1,
		.mode = ADC_SINGLE,
		.func_p = NULL,
		.param_p = NULL,
		.sample_frec = 0};
	serial_config_t puerto = {
		.port = UART_PC,
		.baud_rate = 115200,
		.func_p = NULL,
		.param_p = NULL};
	TimerInit(&timer_medicion_senial);
	TimerInit(&timer_reproducir_ECG);

	AnalogInputInit(&senial_CH1);
	AnalogOutputInit();
	UartInit(&puerto);
	/* Creación de tareas */
	xTaskCreate(&signal_Task, "senial", 2048, NULL, 5, &medicion_task_handle);
	xTaskCreate(&reproducir_ECG_Task, "senial_ecg", 2048, NULL, 5, &ECG_task_handle);
	/* Inicialización del conteo de los timers */
	TimerStart(TIMER_A);
	TimerStart(TIMER_B);
}
/*==================[end of file]============================================*/
//Consutas: debo conectar la salida del DAC a CH1 para ver en el serial oscilloscope? y el potenciometro? 
//CH1-2-3 solo aptos entrada, CH0 pero para CAD