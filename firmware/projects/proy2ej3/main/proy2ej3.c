/*! @mainpage Proyecto 2 Ejercicio 3
 *
 * @section genDesc Se agrega en este ejercicio el puerto serie al proyecto de medir distancia por ultrasonido.
 *
 * Se agrega ahora el puerto serie al proyecto de medir distancia por ultrasonido
 * para visualizar los datos en la PC.
 * El formato de los datos se vera con 3 digitos ascii y expresados en cm. Ademas, las teclas O y H 
 * heredaran las funcionalidades de las teclas 1 y 2 respectivamente. 
 *
 * Cree un nuevo proyecto en el que modifique la actividad del punto 2 agregando ahora el puerto serie. Envíe los datos de las mediciones para poder 
 * observarlos en un terminal en la PC, siguiendo el siguiente formato:
 * 3 dígitos ascii + 1 carácter espacio + dos caracteres para la unidad (cm) + cambio de línea “ \r\n”
* Además debe ser posible controlar la EDU-ESP de la siguiente manera:
* Con las teclas “O” y “H”, replicar la funcionalidad de las teclas 1 y 2 de la EDU-ESP

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
#include "uart_mcu.h"

/*==================[macros and definitions]=================================*/
//#define CONFIG_BLINK_PERIOD_TECLAS 200 //Intervarlo entre lectura de teclas
//#define CONFIG_BLINK_PERIOD_MEDICION 1000 //Intervalo de actualizacion de la medicion del sensor
#define CONFIG_BLINK_PERIOD_MEDICION_US 100000


/*==================[internal data definition]===============================*/
bool hold = false; //Variable de estado para mantener el ultimo valor
bool medir = true; //Activa o no la medicion
uint16_t valor_medicion = 0; //Distancia medida
TaskHandle_t medir_task_handle = NULL;
TaskHandle_t mostrar_task_handle = NULL;


/*==================[internal functions declaration]=========================*/
void FuncTimerA(void *param){
    vTaskNotifyGiveFromISR(medir_task_handle, pdFALSE);  //Notificara tareas
	vTaskNotifyGiveFromISR(mostrar_task_handle, pdFALSE);   
 
}


/**
 * @brief Controla los LEDs segun el valor de valor_medicion
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
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);    /* La tarea espera en este punto hasta recibir una notificación */
		if (medir)
		{
			valor_medicion = HcSr04ReadDistanceInCentimeters();
			UartSendString(UART_PC,(char*)UartItoa(valor_medicion,10));
         	UartSendString(UART_PC,"cm \r\n");
		}
	}
}


/**
 * @brief Muestra la lectura de distancia en los LEDS y en el display.
 *
 * Si medir es true y hold es true, no actualiza el LCD, pero sí los LEDs. Si no está en hold, muestra el valor de valor_medicion en el LCD y ajusta los LEDs.
 * Si medir es false, apaga el LCD y los LEDs.
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
void InterrupcionTecla1 (void)
{
	medir=!medir;
	}
/**
 * @brief Función de interrupción para el botón TEC2. Alterna el valor de la variable hold.
 *
 * Al presionar el botón TEC2 se activa esta interrupción, lo que hace alterna el valor de la variable hold.
 * Si hold es true, se mantiene el valor de la última medicion en el display. Si es false, se actualiza el valor de la medicion en el display.
 *
 */
void InterrupcionTecla2 (void)
{
	hold=!hold;
	}

/**
 * @brief Lee las teclas enviadas por el puerto serial y llama a las interrupciones correspondientes.
 *
 * Lee el caracter enviado por el puerto serial y lo utiliza para decidir
 * si llamar a InterrupcionTecla1 o InterrupcionTecla2. Si el caracter
 * es 'O', llama a InterrupcionTecla1, que alterna el valor de la variable
 * medir. Si el caracter es 'H', llama a InterrupcionTecla2, que alterna el
 * valor de la variable hold.
 *
 */
void leer_teclas(){
uint8_t teclas;
UartReadByte(UART_PC, &teclas);
    	switch(teclas){
    		case 'O':
    			 InterrupcionTecla1();
    		break;
    		case 'H':
    			 InterrupcionTecla2();
    		break;
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

	serial_config_t puerto={
		.port=UART_PC,
		.baud_rate = 9600, //9600 definimos en clase
		.func_p=&leer_teclas,
		.param_p=NULL,
	};
	UartInit(&puerto);

	timer_config_t timer_medicion = {
		.timer = TIMER_A,
		.period = CONFIG_BLINK_PERIOD_MEDICION_US,
		.func_p = FuncTimerA,
		.param_p = NULL
	};
	
	TimerInit(&timer_medicion);

	// Se crean las tareas
	xTaskCreate(&mostrar, "mostrar", 512, NULL, 5, &mostrar_task_handle);
	xTaskCreate(&medicion, "medicion", 512, NULL, 5, &medir_task_handle);

	SwitchActivInt(SWITCH_1, &InterrupcionTecla1, NULL);
	SwitchActivInt(SWITCH_2, &InterrupcionTecla2, NULL);

	TimerStart(timer_medicion.timer);

}
/*==================[end of file]============================================*/