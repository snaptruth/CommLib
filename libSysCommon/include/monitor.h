#ifndef MONITOR_H
#define MONITOR_H

#include "common.h"
#include <errno.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/time.h>

//监控处理类型
enum monitor_proc_flag
{
    MONITOR_PROC_REBOOT,
    MONITOR_PROC_RESTART
};

#define RECORD_RESET_TIME_MUX 5    //最多可统计的复位时间次数
#define RESET_TIME_GAP 1800        //30分钟，如果30分钟内出现类RECORD_RESET_TIME_MUX次复位，这时需要复位系统
#define FEED_DOG_INTERVAL 200      //喂狗周期200毫秒

//监控进程复位时间记录
typedef struct
{
    time_t   resetTime[RECORD_RESET_TIME_MUX]; /*记录最近RECORD_RESET_TIME_MUX次的复位时间*/
    UINT8    resetIndex;
    UINT8    resetNum;
} RESET_TIME;

/* 监控应用信息 */
typedef struct monitor_app_info
{
    char                 procFlag;                  /* 处理标志, see monitor_proc_flag */
    UINT32               checkInterval;             /* 检测间隔毫秒 */
    UINT32               curCheckInterval;          /* 当前检测剩余间隔毫秒*/
    const char          *pAppName;                  /* 应用名称 */
    UINT32               msgSrc;                    /* 应用监控消息源编号 */
    const char          *pRestartCmd;               /* 进程重启命令 */
    const char          *pRebootCmd;                /* 系统重启命令 */
    UINT32               appRegCount;               /*任务注册次数*/
    UINT32               lastAppRegCount;           /*上次任务注册次数*/
    UINT32               restartAppCount;           /*主动重启任务的次数，即监控程序重启任务的次数*/
    pthread_t            pid;                       /*任务PID*/
    UINT32               appIdChangeTimes;          /*任务ID变化的次数*/
    RESET_TIME           resetTimeTable;            /*重启时间记录表*/
} MONITOR_APP_INFO, *pMONITOR_APP_INFO;

//监控注册消息净荷,注册消息格式
typedef struct
{
    UCHAR msgSrc;      //消息来源
    pthread_t pidNum;  //pid号
}
MONITOR_MSG_PAYLOAD;

#define MONITOR_MSG_TYPE 121
//监控注册消息
typedef struct
{
    long type;
    MONITOR_MSG_PAYLOAD payload;//注册消息的格式
} MONITOR_MSG;
typedef  void(* watchDogPtr)();

extern INT32 initMonitorMsgque();
extern INT32 sendMonitorMsg(UCHAR msgSrc);
extern INT32 receMonitorMsg();
extern INT32  systemMonitorInit();
extern void monitor_200ms_process(union sigval v);
extern void* monitor_rcv_queue_msg_sevice(void* arg);//建立以0X5AA5为key的消息队列，通过此队列接收被监控进程发送过来的注册消息（钟）
extern void registerWatchDog(watchDogPtr fun);
#endif // MONITOR_H

