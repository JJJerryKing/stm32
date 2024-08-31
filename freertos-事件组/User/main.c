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
/* Wait_Task任务句柄 */
static TaskHandle_t Wait_Task_Handle = NULL;
/* KEY_Task任务句柄 */
static TaskHandle_t KEY_Task_Handle = NULL;


static void AppTaskCreate(void);/* 用于创建任务 */
static void Wait_Task(void* pvParameters);/* Wait_Task任务实现 */
static void KEY_Task(void* pvParameters);/* KEY_Task任务实现 */


/* 事件组 */ 
static EventGroupHandle_t Event_Handle = NULL;

#define KEY1_EVENT	(0x01 << 0)	//设置事件掩码的位0
#define KEY2_EVENT	(0x01 << 1)	//设置事件掩码的位1


int main(void)
{
	BaseType_t xReturn = pdPASS;/* 定义一个创建信息返回值，默认为pdPASS */
	
	/*模块初始化*/
	//OLED_Init();		//OLED初始化
	Serial_Init();		//串口1初始化
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
  
  /* 创建Event_Handle */
	Event_Handle = xEventGroupCreate(); /* 创建事件组 */
	if(Event_Handle != NULL)
		printf("Event_Handle计数信号量创建成功 \r\n");
  
  /* 创建Wait_Task任务 */
  xReturn = xTaskCreate((TaskFunction_t )Wait_Task, /* 任务入口函数 */
                        (const char*    )"Wait_Task",/* 任务名字 */
                        (uint16_t       )512,   /* 任务栈大小 */
                        (void*          )NULL,	/* 任务入口函数参数 */
                        (UBaseType_t    )2,	    /* 任务的优先级 */
                        (TaskHandle_t*  )&Wait_Task_Handle);/* 任务控制块指针 */
  if(pdPASS == xReturn)
    printf("创建Wait_Task任务成功!\r\n");
  
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
  * @ 函数名  ： Wait_Task
  * @ 功能说明： Wait_Task任务主体
  * @ 参数    ：   
  * @ 返回值  ： 无
  ********************************************************************/
static void Wait_Task(void* parameter)
{	
	EventBits_t r_event = pdTRUE;	//定义一个事件接受变量
	
    while (1)
    {
		r_event = xEventGroupWaitBits(Event_Handle,	/* 事件对象句柄 */
										KEY1_EVENT|KEY2_EVENT,/*接收线程感兴趣的事件*/
										pdTRUE, /*退出时清除事件位*/
										pdTRUE, /* 满足感情取得所有事件 */
										portMAX_DELAY); /* 指定超时时间，一直等 */ 
	
		if((r_event & (KEY1_EVENT|KEY2_EVENT))==(KEY1_EVENT|KEY2_EVENT)){
			printf("KEY1与KEY2都按下 \r\n");
		}
		else{
			printf("事件错误 \r\n");
		}
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
	while (1)
    {
		if(Key_GetNum()==1){
			printf("KEY1被按下 \r\n");
			xEventGroupSetBits(Event_Handle,KEY1_EVENT);
		}
		if(Key_GetNum()==2){
			printf("KEY2被按下 \r\n");
			xEventGroupSetBits(Event_Handle,KEY2_EVENT);
		}
		vTaskDelay(20);		/* 延时20个tick */

	}
}

