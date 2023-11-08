/************************************************************************
* 5 semestre - Eng. da Computao - Insper
* 2021 - Exemplo com HC05 com RTOS
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
#define BUT_PIO      PIOA
#define BUT_PIO_ID   ID_PIOA
#define BUT_IDX      4
#define BUT_IDX_MASK (1 << BUT_IDX)

#define BUT_PIO2      PIOB
#define BUT_PIO_ID2   ID_PIOB
#define BUT_IDX2      2
#define BUT_IDX_MASK2 (1 << BUT_IDX2)

#define BUT_PIO3      PIOC
#define BUT_PIO_ID3   ID_PIOC
#define BUT_IDX3      30
#define BUT_IDX_MASK3 (1 << BUT_IDX3)

#define BUT_PIO4      PIOC
#define BUT_PIO_ID4   ID_PIOC
#define BUT_IDX4      17
#define BUT_IDX_MASK4 (1 << BUT_IDX4)

#define BUT_PIO5      PIOA
#define BUT_PIO_ID5   ID_PIOA
#define BUT_IDX5      11
#define BUT_IDX_MASK5 (1 << BUT_IDX5)

// AFEC
#define AFEC_POT AFEC0
#define AFEC_POT_ID ID_AFEC0
#define AFEC_POT_CHANNEL 0 // Canal do pino PD30

// usart (bluetooth ou serial)
// Descomente para enviar dados
// pela serial debug

//#define DEBUG_SERIAL

#ifdef DEBUG_SERIAL
#define USART_COM USART1
#define USART_COM_ID ID_USART1
#else
#define USART_COM USART0
#define USART_COM_ID ID_USART0
#endif

/************************************************************************/
/* RTOS                                                                 */
/************************************************************************/

#define TASK_BLUETOOTH_STACK_SIZE            (4096/sizeof(portSTACK_TYPE))
#define TASK_BLUETOOTH_STACK_PRIORITY        (tskIDLE_PRIORITY)

QueueHandle_t xQueuePot;
QueueHandle_t xQueueButton;
QueueHandle_t xQueueLiga;

TimerHandle_t xTimer;

/************************************************************************/
/* prototypes                                                           */
/************************************************************************/

extern void vApplicationStackOverflowHook(xTaskHandle *pxTask,
signed char *pcTaskName);
extern void vApplicationIdleHook(void);
extern void vApplicationTickHook(void);
extern void vApplicationMallocFailedHook(void);
extern void xPortSysTickHandler(void);
static void config_AFEC_pot(Afec *afec, uint32_t afec_id, uint32_t afec_channel, afec_callback_t callback);


/************************************************************************/
/* constants                                                            */
/************************************************************************/

/************************************************************************/
/* variaveis globais                                                    */
/************************************************************************/
typedef struct {
  uint value;
} adcData;

typedef struct {
	char tipo;
	int value;
} pack;

volatile int liga = 0;

/************************************************************************/
/* RTOS application HOOK                                                */
/************************************************************************/

extern void vApplicationStackOverflowHook(xTaskHandle *pxTask,
signed char *pcTaskName) {
	printf("stack overflow %x %s\r\n", pxTask, (portCHAR *)pcTaskName);
	for (;;) { }
}

extern void vApplicationIdleHook(void) {
	pmc_sleep(SAM_PM_SMODE_SLEEP_WFI);
}

extern void vApplicationTickHook(void) { }

extern void vApplicationMallocFailedHook(void) {
	configASSERT( ( volatile void * ) NULL );
}

/************************************************************************/
/* handlers / callbacks                                                 */
/************************************************************************/

static void AFEC_pot_callback(void) {
  adcData adc;
  adc.value = afec_channel_get_value(AFEC_POT, AFEC_POT_CHANNEL);
  BaseType_t xHigherPriorityTaskWoken = pdTRUE;
  xQueueSendFromISR(xQueuePot, &adc, &xHigherPriorityTaskWoken);
}

void vTimerCallback(TimerHandle_t xTimer) {
  /* Selecina canal e inicializa conversão */
  afec_channel_enable(AFEC_POT, AFEC_POT_CHANNEL);
  afec_start_software_conversion(AFEC_POT);
}

void but1_callback(void) {
	pack p;
	p.tipo = 'b';
	p.value = 1;
	BaseType_t xHigherPriorityTaskWoken = pdTRUE;
	xQueueSendFromISR(xQueueButton, &p, &xHigherPriorityTaskWoken);
}
void but2_callback(void) { 
	pack p;
	p.tipo = 'b';
	p.value = 2;
	BaseType_t xHigherPriorityTaskWoken = pdTRUE;
	xQueueSendFromISR(xQueueButton, &p, &xHigherPriorityTaskWoken);
}
void but3_callback(void) { 
	pack p;
	p.tipo = 'b';
	p.value = 3;
	BaseType_t xHigherPriorityTaskWoken = pdTRUE;
	xQueueSendFromISR(xQueueButton, &p, &xHigherPriorityTaskWoken);
}
void but4_callback(void) {
	pack p;
	p.tipo = 'b';
	p.value = 4;
	BaseType_t xHigherPriorityTaskWoken = pdTRUE;
	xQueueSendFromISR(xQueueButton, &p, &xHigherPriorityTaskWoken);
}
void but5_callback(void) { 
	pack p;
	p.tipo = 'b';
	p.value = 7;
	BaseType_t xHigherPriorityTaskWoken = pdTRUE;
	xQueueSendFromISR(xQueueLiga, &p, &xHigherPriorityTaskWoken);
}

/************************************************************************/
/* funcoes                                                              */
/************************************************************************/

void io_init(void) {

	// Ativa PIOs
	pmc_enable_periph_clk(LED_PIO_ID);
	pmc_enable_periph_clk(BUT_PIO_ID);
	pmc_enable_periph_clk(BUT_PIO_ID2);
	pmc_enable_periph_clk(BUT_PIO_ID3);
	pmc_enable_periph_clk(BUT_PIO_ID4);

	// Configura Pinos
	pio_configure(LED_PIO, PIO_OUTPUT_0, LED_IDX_MASK, PIO_DEFAULT);
	pio_configure(BUT_PIO, PIO_INPUT, BUT_IDX_MASK, PIO_PULLUP);
	pio_configure(BUT_PIO2, PIO_INPUT, BUT_IDX_MASK2, PIO_PULLUP | PIO_DEBOUNCE);
	pio_configure(BUT_PIO3, PIO_INPUT, BUT_IDX_MASK3, PIO_PULLUP | PIO_DEBOUNCE);
	pio_configure(BUT_PIO4, PIO_INPUT, BUT_IDX_MASK4, PIO_PULLUP | PIO_DEBOUNCE);
	pio_configure(BUT_PIO5, PIO_INPUT, BUT_IDX_MASK5, PIO_PULLUP | PIO_DEBOUNCE);

	// Ativa interrupt
	pio_enable_interrupt(BUT_PIO, BUT_IDX_MASK);
	pio_enable_interrupt(BUT_PIO2, BUT_IDX_MASK2);
	pio_enable_interrupt(BUT_PIO3, BUT_IDX_MASK3);
	pio_enable_interrupt(BUT_PIO4, BUT_IDX_MASK4);
	pio_enable_interrupt(BUT_PIO5, BUT_IDX_MASK5);

	//callbacks
	pio_handler_set(BUT_PIO5, BUT_PIO_ID5, BUT_IDX_MASK5, PIO_IT_FALL_EDGE, but5_callback);
	pio_handler_set(BUT_PIO, BUT_PIO_ID, BUT_IDX_MASK, PIO_IT_FALL_EDGE, but1_callback);
	pio_handler_set(BUT_PIO2, BUT_PIO_ID2, BUT_IDX_MASK2, PIO_IT_FALL_EDGE, but2_callback);
	pio_handler_set(BUT_PIO3, BUT_PIO_ID3, BUT_IDX_MASK3, PIO_IT_FALL_EDGE, but3_callback);
	pio_handler_set(BUT_PIO4, BUT_PIO_ID4, BUT_IDX_MASK4, PIO_IT_FALL_EDGE, but4_callback);

	// Configura NVIC para receber interrupcoes do PIO do botao
	// com prioridade 4 (quanto mais próximo de 0 maior)
	NVIC_EnableIRQ(BUT_PIO_ID);
	NVIC_SetPriority(BUT_PIO_ID, 4);
	NVIC_EnableIRQ(BUT_PIO_ID2);
	NVIC_SetPriority(BUT_PIO_ID2, 4);
	NVIC_EnableIRQ(BUT_PIO_ID3);
	NVIC_SetPriority(BUT_PIO_ID3, 4);
	NVIC_EnableIRQ(BUT_PIO_ID4);
	NVIC_SetPriority(BUT_PIO_ID4, 4);
	NVIC_EnableIRQ(BUT_PIO_ID5);
	NVIC_SetPriority(BUT_PIO_ID5, 4);
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

	stdio_serial_init(CONF_UART, &uart_serial_options);

	#if defined(__GNUC__)
	setbuf(stdout, NULL);
	#else
	#endif
}

static void config_AFEC_pot(Afec *afec, uint32_t afec_id, uint32_t afec_channel,
                            afec_callback_t callback) {
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

  /*** Configuracao específica do canal AFEC ***/
  struct afec_ch_config afec_ch_cfg;
  afec_ch_get_config_defaults(&afec_ch_cfg);
  afec_ch_cfg.gain = AFEC_GAINVALUE_0;
  afec_ch_set_config(afec, afec_channel, &afec_ch_cfg);

  // Calibracao:
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

/************************************************************************/
/* TASKS                                                                */
/************************************************************************/

static void task_proc(void *pvParameters){
	 // configura ADC e TC para controlar a leitura
  config_AFEC_pot(AFEC_POT, AFEC_POT_ID, AFEC_POT_CHANNEL, AFEC_pot_callback);
	
  xTimer = xTimerCreate("Timer", 300, pdTRUE, (void *)0, vTimerCallback);
  xTimerStart(xTimer, 0);

  // variável para recever dados da fila
  adcData adc;
  pack tip;
  int v[2] = {};
  while (1) {
    if(xQueueReceive(xQueuePot, &adc, portMAX_DELAY)) {
	  v[0] = v[1];
	  v[1] = adc.value;
	  if(v[1] > (v[0]+100)){
		if(liga == 1){
			tip.tipo = 'a';
			tip.value = 1;
			xQueueSend(xQueueButton, &tip, 0);
		}
		
	  }
	  else if(v[1] < (v[0]-100)){
		if(liga == 1){
			tip.tipo = 'a';
			tip.value = 0;
			xQueueSend(xQueueButton, &tip, 0);
		}
	  }
	}
  }
}

void task_bluetooth(void) {
	printf("Task Bluetooth started \n");
	printf("Inicializando HC05 \n");
	config_usart0();
	hc05_init();

	// configura LEDs e Botões
	io_init();

	char button1;
	char eof = 'X';

	pack tip;

	// Task não deve retornar.
	while(1) {
		if (liga == 0){
			if (xQueueReceive(xQueueLiga, &tip, 0)) {
				if (tip.value == 7){
					liga = 1;
				}
				button1 = '0';
			}
		}
		else {
			// atualiza valor do botão
			if (xQueueReceive(xQueueButton, &tip, 0)) {
				if (tip.tipo == 'b'){
					if (tip.value == 1){button1 = '1';}
					else if (tip.value == 2){button1 = '2';}
					else if (tip.value == 3){button1 = '3';}
					else if (tip.value == 4){button1 = '4';}
				}
				else if (tip.tipo == 'a'){
					if (tip.value == 1){button1 = '5';}
					else {button1 = '6';}
				}
				else { button1 = '0'; }
			}
			else { button1 = '0'; }
		}
		
		
		// envia status botão
		while(!usart_is_tx_ready(USART_COM)) {
			vTaskDelay(10 / portTICK_PERIOD_MS);
		}
		usart_write(USART_COM, button1);
		
		// envia fim de pacote
		while(!usart_is_tx_ready(USART_COM)) {
			vTaskDelay(10 / portTICK_PERIOD_MS);
		}
		usart_write(USART_COM, eof);

		// dorme por 500 ms
		vTaskDelay(500 / portTICK_PERIOD_MS);
	}
}

/************************************************************************/
/* main                                                                 */
/************************************************************************/

int main(void) {
	/* Create tasks */
	xTaskCreate(task_bluetooth, "BLT", TASK_BLUETOOTH_STACK_SIZE, NULL,	TASK_BLUETOOTH_STACK_PRIORITY, NULL);
	xTaskCreate(task_proc, "PROC", TASK_BLUETOOTH_STACK_SIZE, NULL,	TASK_BLUETOOTH_STACK_PRIORITY, NULL);

	/* Create queues */
	xQueuePot = xQueueCreate(100, sizeof(adcData));
  	if (xQueuePot == NULL)
		printf("falha em criar a queue xQueueADC \n");

	xQueueButton = xQueueCreate(100, sizeof(pack));
	if (xQueueButton == NULL)
	printf("falha em criar a queue xQueueButton \n");
	
	xQueueLiga = xQueueCreate(100, sizeof(pack));
	if (xQueueLiga == NULL)
	printf("falha em criar a queue xQueueLiga \n");
	
	/* Initialize the SAM system */
	sysclk_init();
	board_init();
	io_init();
	delay_init();
	configure_console();

	/* Start the scheduler. */
	vTaskStartScheduler();

	while(1){}

	pmc_sleep(SAM_PM_SMODE_SLEEP_WFI);
	/* Will only get here if there was insufficient memory to create the idle task. */
	return 0;
}
