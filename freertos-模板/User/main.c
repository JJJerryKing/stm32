#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "OLED.h"
#include "LED.h"
#include "Key.h"
#include "Serial.h"

/* FreeRTOS头文件 */
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "semphr.h"
#include "event_groups.h"

 /* 创建任务句柄 */
static TaskHandle_t AppTaskCreate_Handle = NULL;
static TaskHandle_t Receive_Task_Handle = NULL;	//Receive_Task句柄
static TimerHandle_t Swtmr2_Handle = NULL;	//软件定时器句柄

static uint32_t TmrCb_Count2 = 0;	//记录软件定时器2回调函数执行次数

static void AppTaskCreate(void);/* 用于创建任务 */
static void Receive_Task(void* pvParmeters);//Receive_Task任务实现
static void Swtmr2_Callback(void* pvParameters);

QueueHandle_t Test_Queue =NULL;
#define QUEUE_LEN	4	//队列长度
#define QUEUE_SIZE	20	//队列消息大小（字节）



int main(void)
{
	BaseType_t xReturn = pdPASS;/* 定义一个创建信息返回值，默认为pdPASS */
	
	/*模块初始化*/
	//OLED_Init();		//OLED初始化
	Serial_Init();		//串口1初始化
//	Key_Init();			//KEY初始化
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
	Test_Queue = xQueueCreate((UBaseType_t)QUEUE_LEN,
								(UBaseType_t)QUEUE_SIZE);
	if(Test_Queue!=NULL){
		printf("Test_Queue消息队列创建成功 \r\n");
	}
	
	
	Swtmr2_Handle=xTimerCreate((const char*			)"OneShotTimer",
                             (TickType_t			)5000,/* 定时器周期 5000(tick) */
                             (UBaseType_t			)pdFALSE,/* 单次模式 */
                             (void*					  )2,/* 为每个计时器分配一个索引的唯一ID */
                             (TimerCallbackFunction_t)Swtmr2_Callback); 
  if(Swtmr2_Handle != NULL)
  {
    xTimerStart(Swtmr2_Handle,0);	//开启周期定时器
	}


  xReturn = xTaskCreate((TaskFunction_t )Receive_Task,  /* 任务入口函数 */
                        (const char*    )"Receive_Task",/* 任务名字 */
                        (uint16_t       )512,  /* 任务栈大小 */
                        (void*          )NULL,/* 任务入口函数参数 */
                        (UBaseType_t    )3, /* 任务的优先级 */
                        (TaskHandle_t*  )&Receive_Task_Handle);/* 任务控制块指针 */ 
  if(pdPASS == xReturn)
    printf("创建Receive_Task任务成功!\n");

  
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
  BaseType_t xReturn = pdPASS;/* 定义一个创建信息返回值，默认为pdPASS */
  char r_queue[40]={0};	/* 定义一个接收消息的变量 */
  int i;
  while (1)
  {
			/* 队列读取（接收），等待时间为一直等待 */
			xReturn = xQueueReceive( Test_Queue,    /* 消息队列的句柄 */
									 &r_queue,      /* 发送的消息内容 */
									 portMAX_DELAY); /* 等待时间 一直等 */
										
				if(pdPASS == xReturn)
				{
					printf("回传 %s ",r_queue);
				}
				else
				{
					printf("数据接收出错\n");
				}
			
  }
}


/***********************************************************************
  * @ 函数名  ： Swtmr2_Callback
  * @ 功能说明： 软件定时器2 回调函数，打印回调函数信息&当前系统时间
  *              软件定时器请不要调用阻塞函数，也不要进行死循环，应快进快出
  * @ 参数    ： 无  
  * @ 返回值  ： 无
  **********************************************************************/
static void Swtmr2_Callback(void* parameter)
{	
  TickType_t tick_num2;
 
  TmrCb_Count2++;						/* 每回调一次加一 */
 
  tick_num2 = xTaskGetTickCount();	/* 获取滴答定时器的计数值 */
 
  printf("Swtmr2_Callback函数执行 %d 次 \r\n", TmrCb_Count2);
  printf("滴答定时器数值=%d \r\n", tick_num2);
}

