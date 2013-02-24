#ifndef APP_CFG_H
#define APP_CFG_H
/* 
********************************************************************************************************* 
*                                      TASK PRIORITIES 
********************************************************************************************************* 
*/

#define  OS_TASK_TMR_PRIO     8

#define  TASK_REJPEG_PRIO     11
#define  TASK_CAPTURE_PRIO    12
#define  TASK_CAMERA_PRIO     13
#define  HTTP_PRIO            21
#define  TELNET_PRIO          22
#define  TFTP_PRIO            23
#define  TASK_TFTP1_PRIO      31
#define  TASK_JPEG_PRIO       30
#define  TASK_START_PRIO      50
#define  TASK_DELTASK_PRIO    29

#define  TASK_START_ID        0      /* Application tasks IDs                         */
#define  TASK_REJPEG_ID            1
#define  TASK_CAPTURE_ID      2
#define  TASK_CAMERA_ID       3
#define  TASK_JPEG_ID         4
#define  TASK_DELTASK_ID      5
/* 
********************************************************************************************************* 
*                                      TASK STACK SIZES 
********************************************************************************************************* 
*/
    
#define APP_TASK_START_STK_SIZE   1024   /* Size of each task's stacks (# of WORDs) */
#define APP_TASK_REJPEG_STK_SIZE   200
#define APP_TASK_CAPTURE_STK_SIZE 200
#define APP_TASK_CAMERA_STK_SIZE  1024
#define APP_TASK_JPEG_STK_SIZE    2048
#define APP_TASK_DELTASK_STK_SIZE 256


#define  TRUE                     1
#endif
