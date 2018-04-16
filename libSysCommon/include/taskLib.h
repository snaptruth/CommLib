#ifndef TASKLIB_H
#define TASKLIB_H
#include "bnlogif.h"
#ifdef __cplusplus
extern "C" {
#endif
#include "common.h"
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <assert.h>
#include <ucontext.h>

/***定义基本宏,系统时间以微秒为最小单位**/
#define MAKE_SEC(x)  x*1000*1000
#define MAKE_MSEC(x) x*1000
#define MAKE_USEC(x) x

#define TASK_DEFAULT_TAKE_SEM_FUNC_LEN (128)        /*调用函数名字段长度*/
#define TASK_DEFAULT_PRIORITY      (60)                   /*默认的任务优先级*/
#define TASK_WATCH_TASK_DELAY_TIME (10000)      /*休眠的默认时间*/
#define TASK_DEFAULT_STACK_SIZE     (4*1024*1024)  /*任务默认的堆栈*/
#define TASK_DEFAULT_OPTION         SCHED_FIFO          /*任务默认调度方式*/
#define TASK_ID_INIT_VALUE   (-1)               /*任务ID初始化的默认值*/
#define TASK_INIT_ACTION() (usleep(MAKE_MSEC(50)))                /*任务初始化动作*/
typedef  void*(* OS_TASK_PTR)(void *);

#define TASK_MAX_TASK_NAME 48   /*最大支持的任务名称为48个字符*/
//#define TASK_MAX_TASK_NUM 16    /*最大支持的任务个数*/
//#define TASK_MAX_WAIT_SEM 8     /*信号量记录到8个*/

#define TASK_WAIT_CHECK_TIME_UNIT (60)  /*单位:秒*/

#define TASK_DEAD_LOCK_DEAL_SWITCH_ON (1)
#define TASK_DEAD_LOCK_DEAL_SWITCH_OFF (0)

#define DEFAULT_TASK_DEAD_LOCK_CONFIRM_TIME (5*60)

#define TASK_MAX_MSG_QUEUE 3 /*线程中最大的消息队列数*/

//typedef struct task_sem_entry
//{
//   SEM_ID semId;            /*sem的指针*/
//   UINT takeCount;          /*被本任务take的次数*/
//   UINT takeSucCnt;         /*take成功的次数*/
//   time_t tgLastExtTime;    /*最后一次操作的时间*/

///* take信号量的函数和行信息*/
//   CHAR   szTakeFunc[TASK_DEFAULT_TAKE_SEM_FUNC_LEN];
//   UINT   uiTakeLine;


//    UINT uiTakeLastTime;


//}TG_TASK_SEM_ENTRY, *PTG_TASK_SEM_ENTRY;

typedef struct task_param
{
    INT32 arg1;
    INT32 arg2;
    INT32 arg3;
    INT32 arg4;
    INT32 arg5;
    INT32 arg6;
    INT32 arg7;
    INT32 arg8;
    INT32 arg9;
    INT32 arg10;
} TG_TASK_PARAM, *PTG_TASK_PARAM;
typedef INT32(*TASK_RESTART_PTR)(void*);

typedef struct task_register_entry
{
    UINT32 uiExeCnt;                  /*总共执行的次数*/
    time_t tgExeTime;                 /*这一次执行的时间*/
    time_t tgLastExeTime;           /*上一次执行的时间*/
} TG_TASK_REGISTER_ENTRY, *PTG_TASK_REGISTER_ENTRY;

typedef struct task_info_entry
{
    INT8 taskName[TASK_MAX_TASK_NAME];      /*任务名称*/
    pthread_t taskId;                       /*任务ID*/
    pid_t     taskPid;                      /*任务PID*/
    INT32 taskPri;                            /*线程优先级*/
    INT32 taskOption;                         /*任务的选项*/
    INT32 taskStackSize;                      /*堆栈大小*/
    TG_TASK_REGISTER_ENTRY taskRegEntry;    /*线程注册的信息*/
    INT8 checkFlag;                                   /*是否检测标志*/
    OS_TASK_PTR taskRestartPtr;             /*线程重启的函数指针*/
    INT32 taskMsgQueue[TASK_MAX_MSG_QUEUE];  /*线程等待接收的消息队列*/
    OS_TASK_PTR SendTestMsg[TASK_MAX_MSG_QUEUE]; /*向接收消息队列发送测试包的函数*/
} TG_TASK_INFO_ENTRY, *PTG_TASK_INFO_ENTRY;


typedef struct task_stats_entry
{
    UINT32 taskCurrNum;                  /*当前的任务数*/
    UINT32 taskSpawnErrCount;            /*创建错误多少次*/
    UINT32 taskSpawnCount;               /*总共创建了多少次*/
    UINT32 taskDeleteCount;              /*删除任务的次数*/
    UINT32 taskForceDeleteCount;         /*强制删除任务的次数*/
    UINT32 taskWatchRestartTaskCount;    /*重启任务的次数*/
} TG_TASK_STATS_ENTRY, *PTG_TASK_STAS_ENTRY;

/*任务监控发现出问题的行为定义*/
enum TG_TASK_WATCH_ACTION
{
    TG_TW_EXIT_PROCESS = 0,
    TG_TW_JUST_RECORD
};

/*TASK模块错误码定义*/
#define TASK_RT_OK 0
#define TASK_RT_ERROR -1
#define TASK_RT_INV_PARAM -2
#define TASK_RT_CREATE_ERROR -3
#define TASK_RT_DELETE_ERROR -4
#define TASK_RT_SEM_ARRAY_FULL -5
#define TASK_RT_SEM_CANT_FIND -6
#define TASK_RT_THREAD_NUM_OUT_RANGE -7

/*系统内部操作宏*/
#define TASK_LIB_MALLOC  malloc
#define TASK_LIB_FREE   free

/*taskLib外部接口*/
extern INT32 taskLibInit(UINT32 threadMuxNum);
//extern INT32 taskLibFinit();
//extern UINT taskFindFirstEmptyTaskEntry(ULLONG bitMap);
extern INT32 taskSpawn(const PCHAR name, INT32 priority, INT32 options, INT32 stackSize, INT8 checkFlag, OS_TASK_PTR pFunc, void *arg);
//extern INT32 taskDelete(INT32 tId);
//extern INT32 taskDeleteForce(INT32 tId);
extern INT32 taskDelay(time_t ticks);
//extern INT32 taskPrioritySet(INT32 tId, INT32 newPriority);
//extern INT32 taskPriorityGet(INT32 tId, INT32 *pPriority);
extern INT32 taskShow();
extern INT32 taskRecord();
//extern INT32 taskSemShow();
extern INT32 taskIdSelf();
//extern INT32 taskSemEntrySet(SEM_ID sem);
//extern INT32 taskSemEntryClear(SEM_ID sem);
extern INT32 taskRegister();
extern INT32 taskRegisterPid();
//extern INT32 taskSemEntryTakeSuccCntUpdate(SEM_ID SemId);
//extern INT32 taskMainTaskRegister();
//extern INT32 taskOnceTaskRegister();
extern INT32 taskSemLog();
extern void* taskWatch(void* pArg);
//extern void taskFreeTaskEntry(UINT32 tId);
extern INT32 taskWatchTaskEnable();

extern void setTaskWatchFalg(int value);
extern int  getTaskWatchFalg();
extern void setTaskMonitorFlag(int value);
extern int  getTaskMonitorFlag();
extern int setSingalFlag(int value);
extern int getSingalFlag();

//extern INT32 taskSetAccommDeadLockDealSwitchToMem(UINT8 DeadLockDealSwitch);
//extern INT32 taskSetAccommDeadLockConfirmTimeToMem(UINT32 uiDeadLockConfirmTime);

//extern UINT8 taskGetAccommDeadLockDealSwitch();
//extern UINT32 taskGetAccommDeadLockConfirmTime();

extern INT32 taskMsgRec(INT32 msqid, void *msgp, size_t msgsz, OS_TASK_PTR pSendTestFun);
extern void taskSendTestMsg();

struct ucontext_ce123
{
    unsigned long     uc_flags;
    struct ucontext  *uc_link;
    stack_t       uc_stack;
    struct sigcontext uc_mcontext;
    sigset_t      uc_sigmask;   /* mask last for extensibility */
} ucontext_ce123_;

struct sigframe_ce123
{
    struct sigcontext sc;//保存一组寄存器上下文
    unsigned long extramask[1];
    unsigned long retcode;//保存返回地址
    //struct aux_sigframe aux __attribute__((aligned(8)));
} sigframe_ce123;

char ** backtrace_symbols_apcomm(void *const *array, int size);
int backtrace_xd(void **array, int size, void* _fp);
void debugBacktrace(unsigned int sn , siginfo_t  *si , void *ptr);
void registerSIGSEGV();

#ifdef __cplusplus
}
#endif

#define SYSMONITOR_DEBUG_INFO(str, args...)\
    do{\
        printf(str, ## args);\
        fflush(stdout);\
    }while(0)


/* 错误提示信息调试输出接口 */
#define SYSMONITOR_DEBUG_ERR(str, args...)\
    do\
    {\
       OutputLib_OutputInfo(OTLB_LOG_EMERG, SYSMONITOR_MOD_NUM, str, ## args);\
    }while(0)

#endif // TASKLIB_H

