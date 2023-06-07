/************************************************************************
* 5 semestre - Eng. da Computao - Insper
*
* 2021 - Exemplo com HC05 com RTOS
*
*/

#include <asf.h>
#include "conf_board.h"
#include <string.h>

/************************************************************************/
/* defines                                                              */
/************************************************************************/

// LEDs
#define LED_PIO      PIOC
#define LED_PIO_ID   ID_PIOC
#define LED_IDX      8
#define LED_IDX_MASK (1 << LED_IDX)

// Botão
#define BUT_PIO_R      PIOC
#define BUT_PIO_ID_R   ID_PIOC
#define BUT_IDX_R      17
#define BUT_IDX_MASK_R (1 << BUT_IDX_R)

#define BUT_PIO_G      PIOC
#define BUT_PIO_ID_G   ID_PIOC
#define BUT_IDX_G      30
#define BUT_IDX_MASK_G (1 << BUT_IDX_G)

#define BUT_PIO_B      PIOB
#define BUT_PIO_ID_B   ID_PIOB
#define BUT_IDX_B      2
#define BUT_IDX_MASK_B (1 << BUT_IDX_B)

#define BUT_PIO_Y      PIOA
#define BUT_PIO_ID_Y   ID_PIOA
#define BUT_IDX_Y      19
#define BUT_IDX_MASK_Y (1 << BUT_IDX_Y)

#define AFEC_6_1			AFEC1
#define AFEC_6_1_ID			ID_AFEC1
#define AFEC_6_1_CHANNEL	6 // Canal do pino PC31

#define AFEC_POT			AFEC0
#define AFEC_POT_ID			ID_AFEC0
#define AFEC_POT_CHANNEL	0 // Canal do pino PD30

#define SW_PIO      PIOB
#define SW_PIO_ID   ID_PIOB
#define SW_IDX      3
#define SW_IDX_MASK (1 << SW_IDX)


// usart (bluetooth ou serial)
// Descomente para enviar dados
// pela serial deb

//#define DEBUG_SERIAL

#ifdef DEBUG_SERIAL
#define USART_COM USART1
#define USART_COM_ID ID_USART1
#else
#define USART_COM USART0
#define USART_COM_ID ID_USART0
#endif

typedef struct {
	uint value;
	char ax;
} adcData;

/************************************************************************/
/* RTOS                                                                 */
/************************************************************************/

#define TASK_BLUETOOTH_STACK_SIZE            (4096/sizeof(portSTACK_TYPE))
#define TASK_BLUETOOTH_STACK_PRIORITY        (tskIDLE_PRIORITY)

/************************************************************************/
/* prototypes                                                           */
/************************************************************************/

extern void vApplicationStackOverflowHook(xTaskHandle *pxTask,
signed char *pcTaskName);
extern void vApplicationIdleHook(void);
extern void vApplicationTickHook(void);
extern void vApplicationMallocFailedHook(void);
extern void xPortSysTickHandler(void);

/************************************************************************/
/* constants                                                            */
/************************************************************************/

/************************************************************************/
/* variaveis globais                                                    */
/************************************************************************/

SemaphoreHandle_t xSemaphoreY;
SemaphoreHandle_t xSemaphoreB;
SemaphoreHandle_t xSemaphoreG;
SemaphoreHandle_t xSemaphoreR;
SemaphoreHandle_t xSemaphoreOK;
SemaphoreHandle_t xSemaphoreSW;
QueueHandle_t xQueueADC;
QueueHandle_t xQueuePos;
TimerHandle_t xTimer;

/************************************************************************/
/* RTOS application HOOK                                                */
/************************************************************************/

/* Called if stack overflow during execution */
extern void vApplicationStackOverflowHook(xTaskHandle *pxTask,
signed char *pcTaskName) {
	printf("stack overflow %x %s\r\n", pxTask, (portCHAR *)pcTaskName);
	/* If the parameters have been corrupted then inspect pxCurrentTCB to
	* identify which task has overflowed its stack.
	*/
	for (;;) {
	}
}

/* This function is called by FreeRTOS idle task */
extern void vApplicationIdleHook(void) {
	pmc_sleep(SAM_PM_SMODE_SLEEP_WFI);
}

/* This function is called by FreeRTOS each tick */
extern void vApplicationTickHook(void) { }

extern void vApplicationMallocFailedHook(void) {
	/* Called if a call to pvPortMalloc() fails because there is insufficient
	free memory available in the FreeRTOS heap.  pvPortMalloc() is called
	internally by FreeRTOS API functions that create tasks, queues, software
	timers, and semaphores.  The size of the FreeRTOS heap is set by the
	configTOTAL_HEAP_SIZE configuration constant in FreeRTOSConfig.h. */

	/* Force an assert. */
	configASSERT( ( volatile void * ) NULL );
}

/************************************************************************/
/* handlers / callbacks                                                 */
/************************************************************************/
void but_callback_r(void) {
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	xSemaphoreGiveFromISR(xSemaphoreR, &xHigherPriorityTaskWoken);
}
void but_callback_g(void) {
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	xSemaphoreGiveFromISR(xSemaphoreG, &xHigherPriorityTaskWoken);
}
void but_callback_b(void) {
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	xSemaphoreGiveFromISR(xSemaphoreB, &xHigherPriorityTaskWoken);
}
void but_callback_y(void) {
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	xSemaphoreGiveFromISR(xSemaphoreY, &xHigherPriorityTaskWoken);
}
void sw_callback(void) {
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	xSemaphoreGiveFromISR(xSemaphoreSW, &xHigherPriorityTaskWoken);
}
static void AFEC_pot_callbackY(void) {
	adcData adcY;
	adcY.value = afec_channel_get_value(AFEC_POT, AFEC_POT_CHANNEL);
	adcY.ax = 1;
	BaseType_t xHigherPriorityTaskWoken = pdTRUE;
	xQueueSendFromISR(xQueueADC, &adcY, &xHigherPriorityTaskWoken);
}
static void AFEC_pot_callbackX(void) {
	adcData adcX;
	adcX.value = afec_channel_get_value(AFEC_6_1, AFEC_6_1_CHANNEL);
	adcX.ax = 0;
	BaseType_t xHigherPriorityTaskWoken = pdTRUE;
	xQueueSendFromISR(xQueueADC, &adcX, &xHigherPriorityTaskWoken);
}
void vTimerCallback(TimerHandle_t xTimer) {
	/* Selecina canal e inicializa convers�o */
	afec_channel_enable(AFEC_POT, AFEC_POT_CHANNEL);
	afec_start_software_conversion(AFEC_POT);
	
	afec_channel_enable(AFEC_6_1, AFEC_6_1_CHANNEL);
	afec_start_software_conversion(AFEC_6_1);
}
/************************************************************************/
/* funcoes                                                              */
/************************************************************************/

static void config_AFEC_pot(Afec *afec, uint32_t afec_id, uint32_t afec_channel, afec_callback_t callback) {
  /*************************************
   * Ativa e configura AFEC
   *************************************/
  /* Ativa AFEC - 0 */
  afec_enable(afec);

  /* struct de configuracao do AFEC */
  struct afec_config afec_cfg;

  /* Carrega parametros padrao */
  afec_get_config_defaults(&afec_cfg);

  /* Configura AFEC */
  afec_init(afec, &afec_cfg);

  /* Configura trigger por software */
  afec_set_trigger(afec, AFEC_TRIG_SW);

  /*** Configuracao espec�fica do canal AFEC ***/
  struct afec_ch_config afec_ch_cfg;
  afec_ch_get_config_defaults(&afec_ch_cfg);
  afec_ch_cfg.gain = AFEC_GAINVALUE_0;
  afec_ch_set_config(afec, afec_channel, &afec_ch_cfg);

  /*
  * Calibracao:
  * Because the internal ADC offset is 0x200, it should cancel it and shift
  down to 0.
  */
  afec_channel_set_analog_offset(afec, afec_channel, 0x200);

  /***  Configura sensor de temperatura ***/
  struct afec_temp_sensor_config afec_temp_sensor_cfg;

  afec_temp_sensor_get_config_defaults(&afec_temp_sensor_cfg);
  afec_temp_sensor_set_config(afec, &afec_temp_sensor_cfg);

  /* configura IRQ */
  afec_set_callback(afec, afec_channel, callback, 1);
  NVIC_SetPriority(afec_id, 4);
  NVIC_EnableIRQ(afec_id);
}

void sw_init(void) {
	// Configura SW
	pio_configure(SW_PIO, PIO_INPUT, SW_IDX_MASK, PIO_PULLUP | PIO_DEBOUNCE);
	
	pio_handler_set(SW_PIO,
					SW_PIO_ID,
					SW_IDX_MASK,
					PIO_IT_FALL_EDGE,
					&sw_callback);

	pio_enable_interrupt(SW_PIO, SW_IDX_MASK);
	pio_get_interrupt_status(SW_PIO);
	
	NVIC_EnableIRQ(SW_PIO_ID);
	NVIC_SetPriority(SW_PIO_ID, 4);
}

void io_init(void) {

	// Ativa PIOs
	pmc_enable_periph_clk(LED_PIO_ID);
	pio_set_output(LED_PIO, LED_IDX_MASK, 0, 0, 0);
	pio_set(LED_PIO,LED_IDX_MASK);

	// Configura Pinos
	pio_configure(BUT_PIO_R, PIO_INPUT, BUT_IDX_MASK_R, PIO_PULLUP | PIO_DEBOUNCE);

	pio_handler_set(BUT_PIO_R,
					BUT_PIO_ID_R,
					BUT_IDX_MASK_R,
					PIO_IT_FALL_EDGE,
					&but_callback_r);

	pio_enable_interrupt(BUT_PIO_R, BUT_IDX_MASK_R);
	pio_get_interrupt_status(BUT_PIO_R);
	
	NVIC_EnableIRQ(BUT_PIO_ID_R);
	NVIC_SetPriority(BUT_PIO_ID_R, 4);
	
	//////////////////////////////////////////////////////////////////////
	
	pio_configure(BUT_PIO_G, PIO_INPUT, BUT_IDX_MASK_G, PIO_PULLUP | PIO_DEBOUNCE);

	pio_handler_set(BUT_PIO_G,
					BUT_PIO_ID_G,
					BUT_IDX_MASK_G,
					PIO_IT_FALL_EDGE,
					&but_callback_g);

	pio_enable_interrupt(BUT_PIO_G, BUT_IDX_MASK_G);
	pio_get_interrupt_status(BUT_PIO_G);
	
	NVIC_EnableIRQ(BUT_PIO_ID_G);
	NVIC_SetPriority(BUT_PIO_ID_G, 4);
	
	//////////////////////////////////////////////////////////////////////
	
	pio_configure(BUT_PIO_B, PIO_INPUT, BUT_IDX_MASK_B, PIO_PULLUP | PIO_DEBOUNCE);

	pio_handler_set(BUT_PIO_B,
					BUT_PIO_ID_B,
					BUT_IDX_MASK_B,
					PIO_IT_FALL_EDGE,
					&but_callback_b);

	pio_enable_interrupt(BUT_PIO_B, BUT_IDX_MASK_B);
	pio_get_interrupt_status(BUT_PIO_B);
	
	NVIC_EnableIRQ(BUT_PIO_ID_B);
	NVIC_SetPriority(BUT_PIO_ID_B, 4);
	
	//////////////////////////////////////////////////////////////////////
	
	pio_configure(BUT_PIO_Y, PIO_INPUT, BUT_IDX_MASK_Y, PIO_PULLUP | PIO_DEBOUNCE);

	pio_handler_set(BUT_PIO_Y,
					BUT_PIO_ID_Y,
					BUT_IDX_MASK_Y,
					PIO_IT_FALL_EDGE,
					&but_callback_y);

	pio_enable_interrupt(BUT_PIO_Y, BUT_IDX_MASK_Y);
	pio_get_interrupt_status(BUT_PIO_Y);
	
	NVIC_EnableIRQ(BUT_PIO_ID_Y);
	NVIC_SetPriority(BUT_PIO_ID_Y, 4);
}

static void configure_console(void) {
	const usart_serial_options_t uart_serial_options = {
		.baudrate = CONF_UART_BAUDRATE,
		#if (defined CONF_UART_CHAR_LENGTH)
		.charlength = CONF_UART_CHAR_LENGTH,
		#endif
		.paritytype = CONF_UART_PARITY,
		#if (defined CONF_UART_STOP_BITS)
		.stopbits = CONF_UART_STOP_BITS,
		#endif
	};

	/* Configure console UART. */
	stdio_serial_init(CONF_UART, &uart_serial_options);

	/* Specify that stdout should not be buffered. */
	#if defined(__GNUC__)
	setbuf(stdout, NULL);
	#else
	/* Already the case in IAR's Normal DLIB default configuration: printf()
	* emits one character at a time.
	*/
	#endif
}

uint32_t usart_puts(uint8_t *pstring) {
	uint32_t i ;

	while(*(pstring + i))
	if(uart_is_tx_empty(USART_COM))
	usart_serial_putchar(USART_COM, *(pstring+i++));
}

void usart_put_string(Usart *usart, char str[]) {
	usart_serial_write_packet(usart, str, strlen(str));
}

int usart_get_string(Usart *usart, char buffer[], int bufferlen, uint timeout_ms) {
	uint timecounter = timeout_ms;
	uint32_t rx;
	uint32_t counter = 0;

	while( (timecounter > 0) && (counter < bufferlen - 1)) {
		if(usart_read(usart, &rx) == 0) {
			buffer[counter++] = rx;
		}
		else{
			timecounter--;
			vTaskDelay(1);
		}
	}
	buffer[counter] = 0x00;
	return counter;
}

void usart_send_command(Usart *usart, char buffer_rx[], int bufferlen,
char buffer_tx[], int timeout) {
	usart_put_string(usart, buffer_tx);
	usart_get_string(usart, buffer_rx, bufferlen, timeout);
}

void config_usart0(void) {
	sysclk_enable_peripheral_clock(ID_USART0);
	usart_serial_options_t config;
	config.baudrate = 9600;
	config.charlength = US_MR_CHRL_8_BIT;
	config.paritytype = US_MR_PAR_NO;
	config.stopbits = false;
	usart_serial_init(USART0, &config);
	usart_enable_tx(USART0);
	usart_enable_rx(USART0);

	// RX - PB0  TX - PB1
	pio_configure(PIOB, PIO_PERIPH_C, (1 << 0), PIO_DEFAULT);
	pio_configure(PIOB, PIO_PERIPH_C, (1 << 1), PIO_DEFAULT);
}

int hc05_init(void) {
	char buffer_rx[128];
	usart_send_command(USART_COM, buffer_rx, 1000, "AT", 100);
	vTaskDelay( 500 / portTICK_PERIOD_MS);
	usart_send_command(USART_COM, buffer_rx, 1000, "AT", 100);
	vTaskDelay( 500 / portTICK_PERIOD_MS);
	usart_send_command(USART_COM, buffer_rx, 1000, "AT+NAMEagoravai", 100);
	vTaskDelay( 500 / portTICK_PERIOD_MS);
	usart_send_command(USART_COM, buffer_rx, 1000, "AT", 100);
	vTaskDelay( 500 / portTICK_PERIOD_MS);
	usart_send_command(USART_COM, buffer_rx, 1000, "AT+PIN0000", 100);
}

void pin_toggle(Pio *pio, uint32_t mask, uint32_t delay)
{
	if (pio_get_output_data_status(pio, mask))
		pio_clear(pio, mask);
	else
		pio_set(pio,mask);
		
	vTaskDelay(delay / portTICK_PERIOD_MS);
}

void faz_envio(char button) {
	// envia status botão
	while(!usart_is_tx_ready(USART_COM)) {
		vTaskDelay(10 / portTICK_PERIOD_MS);
	}
	usart_write(USART_COM, button);
	
	// envia fim de pacote
	while(!usart_is_tx_ready(USART_COM)) {
		vTaskDelay(10 / portTICK_PERIOD_MS);
	}
	usart_write(USART_COM, 'Z');
}

/************************************************************************/
/* TASKS                                                                */
/************************************************************************/

void task_joypad(void) {
	config_AFEC_pot(AFEC_POT, AFEC_POT_ID, AFEC_POT_CHANNEL, AFEC_pot_callbackY);
	config_AFEC_pot(AFEC_6_1, AFEC_6_1_ID, AFEC_6_1_CHANNEL, AFEC_pot_callbackX);
	
	xTimer = xTimerCreate(/* Just a text name, not used by the RTOS
                        kernel. */
                        "Timer",
                        /* The timer period in ticks, must be
                        greater than 0. */
                        100,
                        /* The timers will auto-reload themselves
                        when they expire. */
                        pdTRUE,
                        /* The ID is used to store a count of the
                        number of times the timer has expired, which
                        is initialised to 0. */
                        (void *)0,
                        /* Timer callback */
                        vTimerCallback);
	xTimerStart(xTimer, 0);
	adcData adc1;
	adcData adc2;
	int yValue;
	int xValue;
	char command;
	while(1) {
		command = 'A';
		if (xQueueReceive(xQueueADC, &(adc1), 0)) {
			if (xQueueReceive(xQueueADC, &(adc2), 0)) {
				if (adc1.ax) {
					yValue = adc1.value - 2048;
					xValue = adc2.value - 2048;
				}
				else {
					yValue = adc2.value - 2048;
					xValue = adc1.value - 2048;
				}
				if (abs(xValue) > abs(yValue)) {
					if (xValue > 100)
						command = 'R';
					else if (xValue < -100)
						command = 'L';
				} else {
					if (yValue > 100)
						command = 'U';
					else if (yValue < -100)
						command = 'D';
				}
				xQueueSend(xQueuePos, &command, 0);
			}
		}
		
		vTaskDelay(10 / portTICK_PERIOD_MS);
	}
}

void task_write(void) {
	config_usart0();
	hc05_init();

	// configura LEDs e Botões
	io_init();
	sw_init();

	// Handshake
	//Send Hello and check OK
	while (!xSemaphoreTake(xSemaphoreOK, 0)) {
		while(!usart_is_tx_ready(USART_COM))
			vTaskDelay(10 / portTICK_PERIOD_MS);
		usart_write(USART_COM, 'H');
	}
	
	// Send Start
	while(!usart_is_tx_ready(USART_COM))
		vTaskDelay(10 / portTICK_PERIOD_MS);
	usart_write(USART_COM, 'S');
	
	// Sinaliza fim do handshake
	for (char i = 0; i < 20; i++)
		pin_toggle(LED_PIO, LED_IDX_MASK, 50);
	char pos;
	
	while(1) {
		// atualiza valor do botão
		if (xSemaphoreTake(xSemaphoreY, 0))
			faz_envio('1');
		else if (xSemaphoreTake(xSemaphoreB, 0))
			faz_envio('2');
		else if (xSemaphoreTake(xSemaphoreG, 0))
			faz_envio('3');
		else if (xSemaphoreTake(xSemaphoreR, 0))
			faz_envio('4');
		else if (xSemaphoreTake(xSemaphoreSW,0))
			faz_envio('5');
		else if (xQueueReceive(xQueuePos, &pos, 0))
			faz_envio(pos);
		
		vTaskDelay(10 / portTICK_PERIOD_MS);
	}
}

void task_read(void) {
	vTaskDelay(100 / portTICK_PERIOD_MS);
	// Handshake
	char rx = 'X';
	while (rx != 'O') {
		while(!usart_is_rx_ready(USART_COM))
			vTaskDelay(10 / portTICK_PERIOD_MS);
		usart_read(USART_COM, &rx);
	}
	
	xSemaphoreGive(xSemaphoreOK);
	
	
	while(1) {
		while(!usart_is_rx_ready(USART_COM))
			vTaskDelay(10 / portTICK_PERIOD_MS);
		usart_read(USART_COM, &rx);
		
		if (rx == '1') {
			for (char i = 0; i < 2; i++)
				pin_toggle(LED_PIO, LED_IDX_MASK, 100);
		}
		else if (rx == '2') {
			for (char i = 0; i < 4; i++)
				pin_toggle(LED_PIO, LED_IDX_MASK, 100);
		}
		else if (rx == '3') {
			for (char i = 0; i < 6; i++)
				pin_toggle(LED_PIO, LED_IDX_MASK, 100);
		}
		else if (rx == '4') {
			for (char i = 0; i < 8; i++)
				pin_toggle(LED_PIO, LED_IDX_MASK, 100);
		}
		else if (rx == '5') {
			for (char i = 0; i < 10; i++)
			pin_toggle(LED_PIO, LED_IDX_MASK, 100);
		}
		vTaskDelay(10 / portTICK_PERIOD_MS);
	}
}
/************************************************************************/
/* main                                                                 */
/************************************************************************/

int main(void) {
	/* Initialize the SAM system */
	sysclk_init();
	board_init();

	configure_console();
	
	xSemaphoreR = xSemaphoreCreateBinary();
	if (xSemaphoreR == NULL)
		printf("falha em criar o semaforo \n");
	
	xSemaphoreG = xSemaphoreCreateBinary();
	if (xSemaphoreG == NULL)
		printf("falha em criar o semaforo \n");
	
	xSemaphoreB = xSemaphoreCreateBinary();
	if (xSemaphoreB == NULL)
		printf("falha em criar o semaforo \n");
	
	xSemaphoreY = xSemaphoreCreateBinary();
	if (xSemaphoreY == NULL)
		printf("falha em criar o semaforo \n");
		
	xSemaphoreOK = xSemaphoreCreateBinary();
	if (xSemaphoreOK == NULL)
		printf("falha em criar o semaforo \n");
	
	xSemaphoreSW = xSemaphoreCreateBinary();
	if (xSemaphoreSW == NULL)
		printf("falha em criar o semaforo \n");
		
	xQueueADC = xQueueCreate(100, sizeof(adcData));
	if (xQueueADC == NULL)
	printf("falha em criar a queue xQueueADC \n");
	
	xQueuePos = xQueueCreate(100, sizeof(char));
	if (xQueuePos == NULL)
		printf("falha em criar a queue xQueueADC \n");

	/* Create task to make led blink */
	xTaskCreate(task_write, "BLT W", TASK_BLUETOOTH_STACK_SIZE, NULL,	TASK_BLUETOOTH_STACK_PRIORITY, NULL);
 	xTaskCreate(task_read, "BLT R", TASK_BLUETOOTH_STACK_SIZE, NULL,	TASK_BLUETOOTH_STACK_PRIORITY, NULL);
	xTaskCreate(task_joypad, "JOYPAD", TASK_BLUETOOTH_STACK_SIZE, NULL,	TASK_BLUETOOTH_STACK_PRIORITY, NULL);
	
	/* Start the scheduler. */
	vTaskStartScheduler();

	while(1){}

	/* Will only get here if there was insufficient memory to create the idle task. */
	return 0;
}
