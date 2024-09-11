/* Standard includes. */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

/* FreeRTOS kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "timers.h"

/* Local includes. */
#include "console.h" // posix 환경에서 console을 사용하기 위한 헤더파일. printf 대신 사용

// Simulation of the CPU hardware sleeping mode
// Idle task hook, 지우면 안됨
void vApplicationIdleHook( void )
{
    usleep( 15000 );
}

// 1000ms Tick 설정
const TickType_t xTicksToWait = pdMS_TO_TICKS( 1000 );

// Queue, Semaphore, Timer 핸들 정의
QueueHandle_t xQueue1;									// LED 1 상태를 저장하는 Queue
QueueHandle_t xQueue2;									// LED 2 상태를 저장하는 Queue
TimerHandle_t xTimer = NULL;							// 3000ms 주기로 동작하는 Timer
SemaphoreHandle_t xSemaphore = NULL;     				// Semaphore를 위한 핸들, Task 간 동기화

// LED 1 제어 태스크
// 1000ms마다 LED 1을 ON
// Queue1에 LED 1의 상태(ON/OFF)를 기록
static void vLED1Task( void *pvParameters )
{
	int32_t lValueToSend;								// 32비트 정수형 변수 선언 lValueToSend
	BaseType_t xStatus;									// 함수의 성공 또는 실패 상태를 표현
	lValueToSend = ( int32_t ) pvParameters;		    // pvParameters를 32비트 정수형 타입으로 변환하여 lValueToSend에 저장


	for( ;; )
	{
		vTaskDelay( pdMS_TO_TICKS( xTicksToWait ) );			// 무한 루프 내에서 매 1000ms마다 실행되도록 설정
		xStatus = xQueueSendToBack( xQueue1, &lValueToSend, 0); // LED 1 상태 &lValueToSend를 xQueue1에 전송
		if( xStatus == pdPASS )
		{
			console_print( "LED 1 task - LED 1 task가 수행되었습니다. LED 1을 켭니다.\r\n");
		}
		else
		{
			console_print( "Could not send to the queue1.\r\n");
		}
	}
}

// 핸들러 태스크
// Semaphore를 받아 LED2를 킨다.
// Queue2에 LED 2의 상태(ON/OFF)를 기록
static void vHandlerTask( void *pvParameters )
{
	BaseType_t xStatus, xSemaphoreStatus;				// 함수 상태 저장, Semaphore 상태 저장
	int32_t lValueToSend;								// 32비트 정수형 변수 선언 lValueToSend (LED 2 상태)
	lValueToSend = ( int32_t ) pvParameters;			// pvParameters를 32비트 정수형 타입으로 변환하여 lValueToSend에 저장

	for(;;)
	{
		xSemaphoreStatus = xSemaphoreTake( xSemaphore, xTicksToWait );	// 무한 루프에서 Semaphore를 1000ms 동안 기다리며 획득 시도
		if( xSemaphoreStatus == pdPASS )
		{
			xStatus = xQueueSendToBack( xQueue2, &lValueToSend, 0 );
			console_print( "Handler Task - 세마포어를 성공적으로 가져왔습니다. LED 2를 켭니다.\r\n" );
		} 
		else
		{
			console_print( "Handler Task - 세마포어를 가져오지 못했습니다. LED 2를 켜지 못했습니다.\r\n" );
		}
	}
}

// LED State 태스크
// 주기 1000ms로 LED들의 상태 출력
// LED 상태 정보는 Queue1, 2에서 받음
static void LEDStatusTask( void *pvParameters )
{
	int32_t lReceivedValue;							// Queue에서 수신된 LED 상태
	BaseType_t xStatus1, xStatus2;					// Queue에서 값을 성공적으로 수신하였는지 상태 저장
	UBaseType_t uxQueueLength1, uxQueueLength2;

	for(;;)
	{
		vTaskDelay( xTicksToWait );					// 매 1000ms마다 상태를 출력하기 위해 Task 대기

		uxQueueLength1 = uxQueueMessagesWaiting( xQueue1 ); // Queue에서 대기 중인 메시지 수 출력
		uxQueueLength2 = uxQueueMessagesWaiting( xQueue2 ); // Queue에서 대기 중인 메시지 수 출력

		console_print( "Queue1에 기록된 데이터 수: %u\r\n", uxQueueLength1 );
		console_print( "Queue2에 기록된 데이터 수: %u\r\n", uxQueueLength2 );

		xStatus1 = xQueueReceive( xQueue1, &lReceivedValue, 0);
		xStatus2 = xQueueReceive( xQueue2, &lReceivedValue, 0);
		
		if( xStatus1 == pdPASS )
		{
			console_print( "LED 1 State: ON\r\n" );
		}
		else
		{
			console_print( "LED 1 State: OFF\r\n" );
		}
		if( xStatus2 == pdPASS )
		{
			console_print( "LED 2 State: ON\r\n" );
		}
		else
		{
			console_print( "LED 2 State: OFF\r\n" );
		}
	}
}

// 소프트웨어 타이머
// 3000ms주기로 실행되는 타이머
// 타이머의 콜백 함수에서 Semaphore를 'give'
void vTimerCallback( TimerHandle_t xTimer )
{
	BaseType_t xHigherPriorityTaskWoken;
	xHigherPriorityTaskWoken = pdFALSE;
	xSemaphoreGiveFromISR( xSemaphore, &xHigherPriorityTaskWoken );				// Semaphore를 해제하여 다른 Task에서 사용가능하게 함.
	console_print( "세마포어가 타이머에 의해 이용 가능한 상태가 되었습니다.\r\n" );
}

int main( void )
{
    console_init(); 

	xQueue1 = xQueueCreate( 5, sizeof( int32_t ) );
	xQueue2 = xQueueCreate( 5, sizeof( int32_t ) );
	xSemaphore = xSemaphoreCreateBinary();				// 이진 Semaphore, 값 1일 때 Semaphore 획득
	
	xTaskCreate( vLED1Task, "LED1Task", configMINIMAL_STACK_SIZE, NULL, 1, NULL );
	xTaskCreate( LEDStatusTask, "LEDStatusTask", configMINIMAL_STACK_SIZE, NULL, 1, NULL );

	if( xSemaphore != NULL )							// Semaphore가 제대로 생성되었는지 확인
	{
		xTaskCreate( vHandlerTask, "HandlerTask", configMINIMAL_STACK_SIZE, NULL, 2, NULL );
		xTimer = xTimerCreate( "TImer", pdMS_TO_TICKS( 3000 ), pdTRUE, NULL, vTimerCallback );
		if( xTimer != NULL )							// Timer가 제대로 생성되었는지 확인
		{
			xTimerStart( xTimer, 0);
		}
	}
    
	vTaskStartScheduler();
	for( ;; );
}