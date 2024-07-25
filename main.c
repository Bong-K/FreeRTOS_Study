

/* Standard includes. */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

/* FreeRTOS kernel includes. */
#include "FreeRTOS.h"
#include "task.h"

/* Local includes. */
#include "console.h" // posix 환경에서 console을 사용하기 위한 헤더파일. printf 대신 사용

// Simulation of the CPU hardware sleeping mode
// Idle 상태일 때 호출되는 hook 함수
void vApplicationIdleHook( void )
{
    usleep( 15000 );
}

// Task "Sleep"
static void prvTaskSleep( void *t_alarm )
{
    while ( 1 )
    {
        vTaskDelay( 1000 );
        console_print( "zzzzzzz\n" );
    }
}

// Task "Ring"
static void prvTaskRing( void *t_ring )
{
	//t_ring 인자를 TaskHandle_t 타입 포인터로 변환한 값을 l_task_id로 저장
    TaskHandle_t l_task_id = * ( TaskHandle_t * ) t_ring;
    while ( 1 )
    {
		// suspend 호출로 중단된 PuttOff 작업이 다시 재개됨
		vTaskDelay( 5000 );
		console_print( "알람! 알람! 알람!\n" );
        vTaskResume( l_task_id );
    }
}

// Task "PutOff"
static void prvTaskPutOff( void *t_alarm )
{
    while ( 1 )
    {
		// suspend 호출로 PuttOff 작업 중단 대기 상태 돌입
        vTaskSuspend( NULL );
		console_print( "5분만..\n" );
    }
}

int main()
{
    // Init output console
    console_init();

    TaskHandle_t l_handle;

    // Three tasks
    xTaskCreate( prvTaskSleep, "Sleep", configMINIMAL_STACK_SIZE, NULL, configMAX_PRIORITIES - 2, NULL );
	// l_handle은 PutOff 작업의 핸들
    xTaskCreate( prvTaskPutOff, "PutOff", configMINIMAL_STACK_SIZE, NULL, configMAX_PRIORITIES - 3, &l_handle );
	// Ring 작업에서 PutOff 작업제어 가능(PutOff의 핸들을 인자로 전달) 
    xTaskCreate( prvTaskRing, "Ring", configMINIMAL_STACK_SIZE, &l_handle, configMAX_PRIORITIES - 4, NULL );

    // Start scheduler
    vTaskStartScheduler();

    return 0;
}