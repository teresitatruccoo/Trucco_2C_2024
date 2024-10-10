/*! @mainpage Template
 *
 * @section genDesc Proyecto Integrador - Bomba de anestesia
 *
 * Este proyecto consiste en, a partir de una señal analogica correspondiente a la actividad muscular de un paciente sedado en
 * cirugia, implementar una bomba de anestesia que inyectara una unidad de anestesia adicional cada vez que se detecte que la
 * misma esta superficial. La anestesia superficial puede evidenciarse con una actividad muscular aumentada.
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
 * | 09/10/2023 | Document creation		                         |
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
#include "pwm_mcu.h"
#include "servo_sg90.h"
#include "iir_filter.h"

/*==================[macros and definitions]=================================*/

/*==================[internal data definition]===============================*/
#define CONFIG_BLINK_PERIOD_medicion 1000 //Fm siendo FmaxEMG 500Hz

#define FS 1000 //Frecuencia de muestreo
#define FC_LOW 10 //Frecuencia de corte
#define ORDER_2 2 //Orden del filtro
#define SIGNAL_SIZE 1000 //Tamaño de la señal adquirida
#define UMBRAL_VALUE 100 //Umbral de señal que mueve el servo
#define SERVO_SG90_PIN GPIO_0 //Pin del servo
#define SERVO_SG90_POSITION 90 //Angulo de mov del servo
#define CONFIG_BLINK_PERIOD_medicion 1000 //Periodo timer A para Fm 1000Hz
#define CONFIG_BLINK_PERIOD_DETECCION 5000 //50ms es suficiente y da margen para el filtrado y procesado. 


TaskHandle_t medicionEMG_task_handle = NULL;
TaskHandle_t procesamientoEMG_task_handle = NULL;
TaskHandle_t deteccionEMG_task_handle = NULL;
/*==================[internal functions declaration]=========================*/

void calcularEnvolvente(float* señalFiltrada, float* envolvente, uint16_t tamaño) {
    float alpha = 0.1; // Factor de suavizado
    envolvente[0] = fabs(señalFiltrada[0]); // Inicializar la envolvente

    for (uint16_t i = 1; i < tamaño; ++i) {
        envolvente[i] = alpha * fabs(señalFiltrada[i]) + (1 - alpha) * envolvente[i - 1]; // Suavizado exponencial
    }
}

///////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

void FuncTimerA(void *param)
{
	xTaskNotifyGive(medicionEMG_task_handle); // Envía una notificación
	xTaskNotifyGive(procesamientoEMG_task_handle);
}
/*************  ✨ Codeium Command ⭐  *************/
/**
 * @brief Función invocada en la interrupción del timer B
 *
 * La función es invocada en la interrupción del timer B y envía una notificación
 * a la tarea asociada a la detección de la señal EMG.
 *
 * @param param No se utiliza
 */
/******  2cebb69b-ad8d-4548-87a3-4fe69be08565  *******/
void FuncTimerB(void *param)
{
	xTaskNotifyGive(deteccionEMG_task_handle); // Envía una notificación
}

void signalEMG_Task(void *pvParameter)
{
	float signalEMG[SIGNAL_SIZE]; //12 bits
	while (1)
	{
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);			 // Recibe una notificacion
		AnalogInputReadSingle(CH1, &signalEMG);					 // Se lee el dato del canal 1 y se guarda en "signalEMG"
		UartSendString(UART_PC, (char *)UartItoa(signalEMG, 10)); // Se muestra el dato trasmitiendolo via UART al puerto serie (ver SerialOscilloscope)
		//UartSendString(UART_PC, "\r\n"); 
	}
}

void procesamientoEMG_Task(void *pvParameter) //Tarea vinculada al mismo timer que la anterior.
{
	float signalEMGfiltrada[SIGNAL_SIZE];
	float signalEMGprocesada[SIGNAL_SIZE]
	while (1)
	{
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);			 // Recibe una notificacion
		signalEMGprocesada = HiPassFilter(&signalEMG, &signalEMGfiltrada, sizeof(signalEMG)); //FiltroPA
		calcularEnvolvente(signalEMGfiltrada, singalEMGprocesada, sizeof(signalEMGfiltrada) / sizeof(signalEMGfiltrada[0]));



	}
}

void deteccionumbralEMG_Task(void *pvParameter) //TimerB
{
	while (1)
	{
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		for (int i = 0; i < 100; i++) {
        if (senalEMGprocesada[i] > UMBRAL_VALUE) {
            // Mover el servo
            ServoMove(SERVO_PIN, SERVO_POSITION);
        }
    }
	}
}

void inyectaranestesia_Task(void *pvParameter) //TimerB
{
	while (1)
	{
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);			 // Recibe una notificacion
	    
	}
}
/*==================[external functions definition]==========================*/
void app_main(void){
	timer_config_t timer_medicion_senial = {
		.timer = TIMER_A,
		.period = CONFIG_BLINK_PERIOD_medicion,
		.func_p = FuncTimerA,
		.param_p = NULL};
	timer_config_t timer_deteccion_senial = {
		.timer = TIMER_B,
		.period = CONFIG_BLINK_PERIOD_DETECCION,
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
TimerInit(&timer_deteccion_senial);

AnalogInputInit(&senial_CH1);
AnalogOutputInit();
UartInit(&puerto);
HiPassInit(FS, FC_LOW, ORDER_2);
ServoInit(SERVO_PIN, SERVO_POSITION);
xTaskCreate(&signalEMG_Task, "senial", 2048, NULL, 5, &medicionEMG_task_handle);
xTaskCreate(&procesamientoEMG_Task, "senial", 2048, NULL, 5, &procesamientoEMG_task_handle);
xTaskCreate(&deteccionumbralEMG_Task, "senial", 2048, NULL, 5, &deteccionEMG_task_handle);
TimerStart(&timer_medicion_senial);
TimerStart(&timer_deteccion_senial);
}
/*==================[end of file]============================================*/