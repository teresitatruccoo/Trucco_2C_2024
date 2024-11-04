/*! @mainpage Examen
 *
 * @section genDesc Examen - Alerta para ciclistas
 *
 * Este programa consiste en un medidor de distancia que ve que vehiculos tiene detras el ciclista
 * y enciende alarmas luminicas y de tipo buzzer dandole un mensaje al ciclista al respecto. 
 * Ademas, cuenta con un acelerometro que sirve como detector de caidas
 * <a href="https://drive.google.com/...">Operation Example</a>
 *
 * @section hardConn Hardware Connection
 *
 * |    Acelerometro  |   ESP32   	|
 * |:--------------:|:--------------|
 * | 	X		| 	CHE1		|
 * | 	Y		| 	CHE2		|
 * | 	Z		| 	CHE3		|
 *
 * |    Buzzer  |   ESP32   	|
 * |:--------------:|:--------------|
 * | 	Pin Buzzer		| 	GPIO_15		|
 *
 *  |    HC_SR04  |   ESP32   	|
 * 	|:-------------:|:--------------|
 * 	| 	ECHO	 	| 	GPIO_3		|
 *	| 	TRIGGER	 	| 	GPIO_2		|
 *	| 	+5V	 		| 	+5V			|
 * 	| 	GND 		| 	GND			|
 *
 * 	|    HC_05  |   ESP32   	|
 * 	|:-------------:|:--------------|
 * 	| 	TX	 		| 	GPIO_18		|
 *	| 	RX	 		| 	GPIO_19		|
 *	| 	+5V	 		| 	+5V			|
 * 	| 	GND 		| 	GND			|
 *
 * @section changelog Changelog
 *
 * |   Date	    | Description                                    |
 * |:----------:|:-----------------------------------------------|
 * | 4/11/2023 | Document creation		                         |
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
#include "pwm_mcu.h"
#include "hc_sr04.h"
#include "timer_mcu.h"
#include "uart_mcu.h"
#include "analog_io_mcu.h"
#include "math.h"



/*==================[macros and definitions]=================================*/
/** @def CONFIG_PERIODO_MEDICION
 * @brief Periodo asociado a la frecuencia de medicion de distancia
 */
#define CONFIG_PERIODO_MEDICION 500000 // En microsegundos
/** @def CONFIG_PERIODO_BUZZER_PRECAUCION
 * @brief Periodo asociado a la frecuencia de sonido del buzzer de precacuion
 */
#define CONFIG_PERIODO_BUZZER_PRECAUCION 1000000
/** @def CONFIG_PERIODO_BUZZER_PELIGRO
 * @brief Periodo asociado a la frecuencia de sonido del buzzer de precacuion
 */
#define CONFIG_PERIODO_BUZZER_PELIGRO 500000
/** @def CONFIG_PERIODO_ACELEROMETRO
 * @brief Periodo asociado a la frecuencia de medicion deL acelerometro.
 */
#define CONFIG_PERIODO_ACELEROMETRO 10000 //frecuencia de muestreo 100Hz.


/*==================[internal data definition]===============================*/

/**
 * @brief Variable para almacenar los valores de la señal de distancia
 */
uint16_t valor_medicion = 0;	
/**
 * @brief Variable para almacenar los valores de la señal del acelerometro en X
 */	
uint16_t signalACELEROMETRO_X=0; 
/**
 * @brief Variable para almacenar los valores de la señal de acelerometro en Y
 */ 
uint16_t signalACELEROMETRO_Y=0;
/**
 * @brief Variable para almacenar los valores de la señal del acelerometro en Z
 */
uint16_t signalACELEROMETRO_Z=0;
/**
 * @brief Elemento de tipo TaskHandle para manejar la tarea de medir distancia
 */
TaskHandle_t medir_task_handle = NULL; // Para la accion de medir.
/**
 * @brief Elemento de tipo TaskHandle para manejar la tarea de encender el buzzer de precaucion 
 */
TaskHandle_t buzzerprecaucion_task_handle = NULL; //Para el GPIO ON en precuacion
/**
 * @brief Elemento de tipo TaskHandle para manejar la tarea de encender el buzzer de peligro
 */
TaskHandle_t buzzerpeligro_task_handle = NULL; //Para el GPIO ON en peligro
/**
 * @brief Elemento de tipo TaskHandle para manejar la tarea de medir señal del acelerometro
 */
TaskHandle_t acelerometro_task_handle = NULL;

/*==================[internal functions declaration]=========================*/
/**
 * @brief Función invocada en la interrupción del timer A
 *
 * La función es invocada en la interrupción del timer B y envía una notificación
 * a la tarea asociada a la medicion de distancia y al buzzer de peligro.
 *
 * @param param No se utiliza
 */
void FuncTimerA(void *param)
{
	vTaskNotifyGiveFromISR(medir_task_handle, pdFALSE);
	vTaskNotifyGiveFromISR(buzzerpeligro_task_handle, pdFALSE);
}

/**
 * @brief Función invocada en la interrupción del timer B
 *
 * La función es invocada en la interrupción del timer B y envía una notificación
 * a la tarea asociada al buzzer de precaución.
 *
 * @param param No se utiliza
 */
void FuncTimerB(void *param)
{
	vTaskNotifyGiveFromISR(buzzerprecaucion_task_handle, pdFALSE);
}

/**
 * @brief Función invocada en la interrupción del timer C
 *
 * La función es invocada en la interrupción del timer C y envía una notificación
 * a la tarea asociada a la medicion de la señal del acelerometro.
 *
 * @param param No se utiliza
 */
void FuncTimerC(void *param)
{
	vTaskNotifyGiveFromISR(acelerometro_task_handle, pdFALSE);
}


/**
 * @brief Controla los LEDs segun el valor de la medicion de distancia
 *
 * Si la medicion de distancia es mayor a 5 metros, solo enciende el LED_1 esta encendido.
 * Si es mayor a 3 metros y menor a 5 metros, los LEDs LED_1 y LED_2 estan encendidos.
 * Si es menor a 3 metros, los tres LEDs estan encendidos.
 * En cada caso, se muestra un mensaje en la UART
 * indicando si debe tener precaucion "Precaución, vehículo cerca" o si esta en peligro "Peligro, vehículo cerca".
 *
 */
void Controlar_Leds(void)
{
	if (valor_medicion > 5*100)
	{
		LedOn(LED_1);
		LedOff(LED_2);
		LedOff(LED_3);
	}
	else
	{
		if (valor_medicion > 3*100 && valor_medicion < 5*100)
		{
			LedOn(LED_1);
			LedOn(LED_2);
			LedOff(LED_3);
			UartSendString(UART_CONNECTOR, "Precaución, vehículo cerca \r\n");
		}
		else
		{
			if (valor_medicion < 3*100)
			{
				LedOn(LED_1);
				LedOn(LED_2);
				LedOn(LED_3);
				UartSendString(UART_CONNECTOR, "Peligro, vehículo cerca \r\n");
			}
		}
	}
}

/**
 * @brief Tarea que mide la distancia con el sensor ultrasónico HC-SR04 y actualiza los LEDS.
 *
 * La tarea se encarga de medir la distancia con el sensor HC-SR04 y
 * actualizar los LEDs segun el valor de la medicion. 
 *
 */
void mediciondistancia_Task(void *pvParameter)
{

	while (true)
	{
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);    /* La tarea espera en este punto hasta recibir una notificación */
		{
			valor_medicion = HcSr04ReadDistanceInCentimeters();
			Controlar_Leds();	
		}
	}
}

/**
 * @brief Tarea que enciende un buzzer para indicar un peligro
 *
 * La tarea se encarga de encender el buzzer
 * si la medicion de distancia es menor a 3 m. Se
 * enciende por 500ms y se apaga. Si la medicion de
 * distancia es mayor a 3m, se apaga.
 *
 */
void alarmapeligro_Task(void *pvParameter)
{

	while (true)
	{
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);    /* La tarea espera en este punto hasta recibir una notificación */
		{
			if (valor_medicion < 3*100) 
			{
			GPIOOn(GPIO_15);
			vTaskDelay(500/portTICK_PERIOD_MS);
			GPIOOFF(GPIO_15);
			}
				else
				{
					GPIOOff(GPIO_15);
				}	
		}
	}
}
/**
 * @brief Tarea que enciende un buzzer para indicar precaucion
 *
 * La tarea se encarga de encender el buzzer
 * si la medicion de distancia esta entre 3 y 5 metros. Se
 * enciende por 1 y se apaga. En otro caso de medicion se apaga.
 *
 */
void alarmaprecaucion_Task(void *pvParameter)
{

	while (true)
	{
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY); /* La tarea espera en este punto hasta recibir una notificación */
		{
			if (valor_medicion > 3 * 100 && valor_medicion < 5 * 100)
			{
				GPIOON(GPIO_15);
				vTaskDelay(1000/portTICK_PERIOD_MS);
				GPIOOFF(GPIO_15);
				}
				else
				{
					GPIOOff(GPIO_15);
				}	
			
		}
	}
}

/**
 * @brief Tarea que mide la aceleracion con el acelerometro y envia mensajes por bt
 *
 * La tarea se encarga de medir la aceleracion con el acelerometro y enviar un mensaje por BT cuando
 * la aceleracion es mayor a 4G.
 *
 */
void medicion_caida_Task(void *pvParameter)
{
uint16_t aceleraciontotal=0;
float umbral_caida=(4*0.3)+1.65; //4G umbral convertido por la sensibilidad. 

	while (true)
	{
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY); /* La tarea espera en este punto hasta recibir una notificación */
		{
			AnalogInputReadSingle(CH1, &signalACELEROMETRO_X);
			AnalogInputReadSingle(CH2, &signalACELEROMETRO_Y);
			AnalogInputReadSingle(CH3, &signalACELEROMETRO_Z);
			signalACELEROMETRO_X=abs(signalACELEROMETRO_X)+1.65;
			signalACELEROMETRO_Y=abs(signalACELEROMETRO_Y)+1.65;
			signalACELEROMETRO_Z=abs(signalACELEROMETRO_Z)+1.65;
			aceleraciontotal=signalACELEROMETRO_X+signalACELEROMETRO_Y+signalACELEROMETRO_Z;
			if (aceleraciontotal > umbral_caida)
			{
				UartSendString(UART_CONNECTOR, "Caida detectada \r\n");
			}
		}

	}
}

/*==================[external functions definition]==========================*/
void app_main(void)
{
	timer_config_t timer_medicion_senial = {
		.timer = TIMER_A,
		.period = CONFIG_PERIODO_MEDICION,
		.func_p = FuncTimerA,
		.param_p = NULL,
	};
	timer_config_t timer_buzzer = {
		.timer = TIMER_B,
		.period = CONFIG_PERIODO_BUZZER_PRECAUCION,
		.func_p = FuncTimerB,
		.param_p = NULL,
	};
	timer_config_t timer_acelerometro = {
		.timer = TIMER_C,
		.period = CONFIG_PERIODO_ACELEROMETRO,
		.func_p = FuncTimerB,
		.param_p = NULL,
	};
	analog_input_config_t senial_CH1_X = {
		.input = CH1,
		.mode = ADC_SINGLE,
		.func_p = NULL,
		.param_p = NULL,
		.sample_frec = 0,
	};
	analog_input_config_t senial_CH2_Y = {
		.input = CH2,
		.mode = ADC_SINGLE,
		.func_p = NULL,
		.param_p = NULL,
		.sample_frec = 0,
	};
	analog_input_config_t senial_CH3_Z = {
		.input = CH3,
		.mode = ADC_SINGLE,
		.func_p = NULL,
		.param_p = NULL,
		.sample_frec = 0,
	};
	serial_config_t puerto = {
		.port = UART_CONNECTOR,
		.baud_rate = 115200,
		.func_p = NULL,
		.param_p = NULL,
	};

	TimerInit(&timer_medicion_senial);
	TimerInit(&timer_buzzer);
	TimerInit(&timer_acelerometro);

	AnalogInputInit(&senial_CH1_X);
	AnalogInputInit(&senial_CH2_Y);
	AnalogInputInit(&senial_CH3_Z);
	AnalogOutputInit();
	UartInit(&puerto);

	xTaskCreate(&mediciondistancia_Task, "senial", 2048, NULL, 5, &medir_task_handle);
	xTaskCreate(&alarmapeligro_Task, "senial", 2048, NULL, 5, &buzzerpeligro_task_handle);
	xTaskCreate(&alarmaprecaucion_Task, "senial", 2048, NULL, 5, &buzzerprecaucion_task_handle);
	xTaskCreate(&medicion_caida_Task, "senial", 2048, NULL, 5, &acelerometro_task_handle);
	
	//Inicializo timers
	TimerStart(timer_medicion_senial.timer);
	TimerStart(timer_acelerometro.timer);
	TimerStart(timer_buzzer.timer);
	}
/*==================[end of file]============================================*/