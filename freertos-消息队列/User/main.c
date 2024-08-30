#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "OLED.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "Serial.h"
#include "LED.h"
#include "Key.h"


 /* 创建任务句柄 */
static TaskHandle_t AppTaskCreate_Handle = NULL;
/* Receive任务句柄 */
static TaskHandle_t Receive_Task_Handle = NULL;
/* KEY任务句柄 */
static TaskHandle_t KEY_Task_Handle = NULL;


static void AppTaskCreate(void);/* 用于创建任务 */
static void Receive_Task(void* pvParameters);/* LED_Task任务实现 */
static void KEY_Task(void* pvParameters);/* KEY_Task任务实现 */

/*队列*/
QueueHandle_t Test_Queue = NULL ;
#define  QUEUE_LEN    4   /* 队列的长度，最大可包含多少个消息 */
#define  QUEUE_SIZE   4   /* 队列中每个消息大小（字节） */


int main(void)
{
	BaseType_t xReturn = pdPASS;/* 定义一个创建信息返回值，默认为pdPASS */
	
	/*模块初始化*/
	//OLED_Init();		//OLED初始化
	Serial_Init();		//串口1初始化
	LED_Init();			//LED1，LED2初始化
	Key_Init();			//KEY初始化
//	
	printf("start\r\n");
//	Serial_SendString("hello\r\n");
	xReturn = xTaskCreate((TaskFunction_t )AppTaskCreate,  /* 任务入口函数---即任务函数的名称，需要我们自己定义并且实现。*/
                        (const char*    )"AppTaskCreate",/* 任务名字---字符串形式， 最大长度由 FreeRTOSConfig.h 中定义的configMAX_TASK_NAME_LEN 宏指定，多余部分会被自动截掉，这里任务名字最好要与任务函数入口名字一致，方便进行调试。*/
                        (uint16_t       )512,  /* 任务栈大小---字符串形式， 最大长度由 FreeRTOSConfig.h 中定义的configMAX_TASK_NAME_LEN 宏指定，多余部分会被自动截掉，这里任务名字最好要与任务函数入口名字一致，方便进行调试。*/
                        (void*          )NULL,/* 任务入口函数参数---字符串形式， 最大长度由 FreeRTOSConfig.h 中定义的configMAX_TASK_NAME_LEN 宏指定，多余部分会被自动截掉，这里任务名字最好要与任务函数入口名字一致，方便进行调试。*/
                        (UBaseType_t    )1, /* 任务的优先级---优先级范围根据 FreeRTOSConfig.h 中的宏configMAX_PRIORITIES 决定， 如果使能 configUSE_PORT_OPTIMISED_TASK_SELECTION，这个宏定义，则最多支持 32 个优先级；如果不用特殊方法查找下一个运行的任务，那么则不强制要求限制最大可用优先级数目。在 FreeRTOS 中， 数值越大优先级越高， 0 代表最低优先级。*/
                        (TaskHandle_t*  )&AppTaskCreate_Handle);/* 任务控制块指针---在使用内存的时候，需要给任务初始化函数xTaskCreateStatic()传递预先定义好的任务控制块的指针。在使用动态内存的时候，任务创建函数 xTaskCreate()会返回一个指针指向任务控制块，该任务控制块是 xTaskCreate()函数里面动态分配的一块内存。*/ 
 /* 启动任务调度。*/          
	if(pdPASS == xReturn)
		vTaskStartScheduler();   /* 启动任务，开启调度 */
	else
		return -1;  

	

	while (1)
	{
		
	}
}


/***********************************************************************
  * @ 函数名  ： AppTaskCreate
  * @ 功能说明： 为了方便管理，所有的任务创建函数都放在这个函数里面
  * @ 参数    ： 无  
  * @ 返回值  ： 无
  **********************************************************************/
static void AppTaskCreate(void)
{
  BaseType_t xReturn = pdPASS;/* 定义一个创建信息返回值，默认为pdPASS */
  
  taskENTER_CRITICAL();           //进入临界区
  
  /* 创建Test_Queue */
  Test_Queue=xQueueCreate((UBaseType_t) QUEUE_LEN,/* 消息队列的长度 */
							(UBaseType_t) QUEUE_SIZE);/* 消息的大小 */
  if(Test_Queue!=NULL)
	printf("创建Test_Queue消息队列成功!\r\n");
  
  
  /* 创建Receive_Task任务 */
  xReturn = xTaskCreate((TaskFunction_t )Receive_Task, /* 任务入口函数 */
                        (const char*    )"Receive_Task",/* 任务名字 */
                        (uint16_t       )512,   /* 任务栈大小 */
                        (void*          )NULL,	/* 任务入口函数参数 */
                        (UBaseType_t    )3,	    /* 任务的优先级 */
                        (TaskHandle_t*  )&Receive_Task_Handle);/* 任务控制块指针 */
  if(pdPASS == xReturn)
    printf("创建Receive_Task任务成功!\r\n");
  
	/* 创建KEY_Task任务 */
  xReturn = xTaskCreate((TaskFunction_t )KEY_Task, /* 任务入口函数 */
                        (const char*    )"KEY_Task",/* 任务名字 */
                        (uint16_t       )512,   /* 任务栈大小 */
                        (void*          )NULL,	/* 任务入口函数参数 */
                        (UBaseType_t    )2,	    /* 任务的优先级 */
                        (TaskHandle_t*  )&KEY_Task_Handle);/* 任务控制块指针 */
  if(pdPASS == xReturn)
    printf("创建KEY_Task任务成功!\r\n");
  
  vTaskDelete(AppTaskCreate_Handle); //删除AppTaskCreate任务
  
  taskEXIT_CRITICAL();            //退出临界区
}


/**********************************************************************
  * @ 函数名  ： Receive_Task
  * @ 功能说明： Receive_Task任务主体
  * @ 参数    ：   
  * @ 返回值  ： 无
  ********************************************************************/
static void Receive_Task(void* parameter)
{	
	BaseType_t xReturn = pdTRUE;		/* 定义一个创建信息返回值，默认为pdTRUE */
	uint32_t r_queue;					/* 定义一个接收消息的变量 */
    while (1)
    {
        xReturn = xQueueReceive(Test_Queue,	/* 消息队列的句柄 */
								&r_queue,	/* 发送的消息内容 */
								portMAX_DELAY);	/*等待时间 一直等 */
		if(xReturn == pdTRUE)
			printf("本次接收到的数据是%d\r\n",r_queue);
		else 
			printf("数据接收出错，错误代码0x%lx\r\n",xReturn);
    }
}

/**********************************************************************
  * @ 函数名  ： KEY_Task
  * @ 功能说明： KEY_Task任务主体
  * @ 参数    ：   
  * @ 返回值  ： 无
  ********************************************************************/
static void KEY_Task(void* parameter)
{	
	uint32_t send_data=0;
	BaseType_t xReturn = pdPASS;	/* 定义一个创建信息返回值，默认为pdPASS */
    
	while (1)
    {
		if(Key_GetNum()){
			printf("发送消息!\r\n");
			xReturn = xQueueSend(Test_Queue, /* 消息队列的句柄 */
								&send_data,		/* 发送的消息内容 */
								0);				/* 等待时间0 */
		
			if(xReturn == pdPASS)
				printf("消息send_data发送成功 \r\n");
		send_data++;
		}
//		vTaskDelay(20);		/* 延时20个tick */
	}
}

