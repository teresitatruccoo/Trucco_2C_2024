/*! @mainpage Proyecto 1 Ejercicio 3
 *
 * @section genDesc Proyecto 1 Ejercicio 3
 *
 * Si el modo es ON, se procede a encender el LED correspondiente según el número del LED (n_led). 
 * Si n_led es 1, se enciende el LED 1; si es 2, se enciende el LED 2; y si es 3, se enciende el LED 3. 
 * Luego, el proceso termina. Si el modo es OFF, se sigue un procedimiento similar para apagar 
 * el LED correspondiente. Si n_led es 1, se apaga el LED 1; si es 2, se apaga el LED 2; y si es 3, 
 * se apaga el LED 3. Después, el proceso finaliza. Si el modo es TOGGLE, el proceso de alternancia 
 * se repite tantas veces como indique n_ciclos. Dependiendo del número del LED (n_led), se alterna el
 *  encendido y apagado del LED correspondiente: LED 1, LED 2 o LED 3. Después de cada alternancia
 * , se espera un tiempo especificado por periodo. Al completar los ciclos, el proceso termina..
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
 * | 12/09/2023 | Document creation		                         |
 *
 * @author Teresita Trucco (teresitatrucco@ingenieria.uner.edu.ar)
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
/*==================[macros and definitions]=================================*/
#define CONFIG_BLINK_PERIOD 100
#define ON 1 
#define OFF 2
#define TOGGLE 3

/*==================[internal data definition]===============================*/
 struct leds
{
    uint8_t mode;      // ON, OFF, TOGGLEe
	uint8_t n_led;       // indica el número de led a controlar
	uint8_t n_ciclos;   //indica la cantidad de ciclos de ncendido/apagado
	uint16_t periodo;   // indica el tiempo de cada ciclo
} my_leds; //Estructura de 5 bytes contiguos en memoria, 3+2
/*==================[internal functions declaration]=========================*/

void control_leds(struct leds *my_leds) {
    uint8_t i = 0;
    uint8_t j = 0;
	LedsInit();

    if (my_leds->mode == ON) {
        if (my_leds->n_led == 1) {
            LedOn(LED_1);
        } else if (my_leds->n_led == 2) {
            LedOn(LED_2);
        } else if (my_leds->n_led == 3) {
            LedOn(LED_3);
        }
    } else if (my_leds->mode == OFF) {
        if (my_leds->n_led == 1) {
            LedOff(LED_1);
        } else if (my_leds->n_led == 2) {
           LedOff(LED_2);
        } else if (my_leds->n_led == 3) {
            LedOff(LED_3);
        }
    } else if (my_leds->mode == TOGGLE) {
        while (i < my_leds->n_ciclos) {
            if (my_leds->n_led == 1) {
                LedToggle(LED_1);
            } else if (my_leds->n_led == 2) {
               LedToggle(LED_2);
            } else if (my_leds->n_led == 3) {
                LedToggle(LED_3);
            }
            i++;

            // Retardo

            for (int j=0; j < my_leds->periodo/CONFIG_BLINK_PERIOD; j++){
            vTaskDelay(CONFIG_BLINK_PERIOD / portTICK_PERIOD_MS); //No ocupa espacio en el micro.
			}

        }
    }
}
/*==================[external functions definition]==========================*/
void app_main(void) {
    struct leds my_leds = {TOGGLE, 1, 10, 500}; //Para probar pongo osciloscopio en la pista entre R4 y R5 y GND
    control_leds(&my_leds);
}
/*==================[end of file]============================================*/