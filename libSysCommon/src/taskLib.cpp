

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/time.h>
#include <dlfcn.h>
#include <sys/syscall.h>
#include <sys/types.h>

#include "taskLib.h"
#include "monitor.h"
#include "bnlogif.cpp"
#include "errCode.h"



TG_TASK_INFO_ENTRY *g_TaskTable = NULL;       /*全局任务表*/
TG_TASK_STATS_ENTRY g_TaskStatistic;                     /*当前任务统计信息*/
UINT32 g_TaskWatchAtionType = TG_TW_EXIT_PROCESS;          /*任务检测发现死锁的时候的处理动作*/

UCHAR g_AccommDeadLockDealSwitch = TASK_DEAD_LOCK_DEAL_SWITCH_OFF;
UINT32 g_uiAccommDeadLockConfirmTime = DEFAULT_TASK_DEAD_LOCK_CONFIRM_TIME;
UINT32 g_threadMuxNum = 0;                                                   /*最大支持的任务个数*/

/**************************************************************************
** 函数名: taskLibInit
** 输  入:  threadMuxNum：线程任务表记录的最大任务数
** 输  出: TASK_RT_OK（成功），
**            TASK_RT_ERROR（分配内存失败），
**            TASK_RT_INV_PARAM(传入的最大任务数为0)
** 描  述: 初始化线程任务表
** 作  者: xiaodong
** 日  期:
** 修  改:
** 备  注:
**************************************************************************/
INT32 taskLibInit(UINT32 threadMuxNum)
{
	BN_LOG_INTERFACE("Call the interface: %s.\n", __func__);
    if(threadMuxNum == 0)
    {
        return TASK_RT_INV_PARAM;
    }
    g_threadMuxNum = threadMuxNum;

    if(NULL == (g_TaskTable = (TG_TASK_INFO_ENTRY *)malloc(sizeof(TG_TASK_INFO_ENTRY) * g_threadMuxNum)))
    {//(TG_TASK_INFO_ENTRY *) is added by haijing or the g++ complier could show error message with type mismatch "cannot convert from void* to TG_TASK_INFO_ENTRY *"
        return TASK_RT_ERROR;
    }

    /*初始化任务表*/
    memset((void*)g_TaskTable, 0, sizeof(TG_TASK_INFO_ENTRY)*g_threadMuxNum);


    UINT32 uiCount, taskEntryId;

    /*将任务表中的消息队列数组初始化为非法值*/
    for(taskEntryId = 0; taskEntryId < g_threadMuxNum; taskEntryId++)
    {
        for(uiCount = 0; uiCount < TASK_MAX_MSG_QUEUE; uiCount++)
        {
            g_TaskTable[taskEntryId].taskMsgQueue[uiCount] = -1;
        }
    }

    //监控SIGSEGV信号
    //registerSIGSEGV();It is not necessary to use now, commented by haijing20171031

    return TASK_RT_OK;
}
/**************************************************************************
** 函数名:taskSpawn
** 输  入:
** 输  出:
** 描  述:
** 作  者: xiaodong
** 日  期:
** 修  改:
** 备  注:
**************************************************************************/
INT32 taskSpawn(const PCHAR name, INT32 priority, INT32 options, INT32 stackSize, INT8 checkFlag, OS_TASK_PTR pFunc, void* arg)
{//eg.taskSpawn("receDbgThread", TASK_DEFAULT_PRIORITY, TASK_DEFAULT_OPTION, TASK_DEFAULT_STACK_SIZE, 0, receDbgMsgThread, NULL)
	BN_LOG_INTERFACE("Call the interface: %s.\n", __func__);
    pthread_attr_t taskAttr;//设置线程属性
    pthread_t taskId = 0;//线程句柄，线程标识
    struct sched_param schedParm;//设置线程优先级

    static INT32 taskEntryId = 0;
    TG_TASK_INFO_ENTRY *taskEntry = g_TaskTable + (taskEntryId++);

//    TG_TASK_PARAM  TaskParam;
    INT32 iResult = TASK_RT_OK;

    if(taskEntryId > g_threadMuxNum)
    {
        return TASK_RT_THREAD_NUM_OUT_RANGE;
    }

    /*参数检测*/
    if((NULL == pFunc) || (NULL ==  name))
        return TASK_RT_INV_PARAM;

    if(0 == options)
        options = TASK_DEFAULT_OPTION;

    if(0 == stackSize)
        stackSize = TASK_DEFAULT_STACK_SIZE;

    if(0 != checkFlag)
        checkFlag = 1;

    /*给task的各个参数赋值*/

//    TaskParam.arg1 = arg;

    /*初始化线程的属性*/
    if(pthread_attr_init(&taskAttr) != 0)
    {
        iResult = TASK_RT_CREATE_ERROR;
        goto TASK_SPAWN_ERROR_PROCESS;
    }

    /*设置线程的堆栈*/
    if(pthread_attr_setstacksize(&taskAttr, stackSize) != 0)
    {
        pthread_attr_destroy(&taskAttr);
        iResult = TASK_RT_CREATE_ERROR;
        goto TASK_SPAWN_ERROR_PROCESS;
    }

    /*设置线程的调度策略*/
    if(0 != pthread_attr_setschedpolicy(&taskAttr,  options))
    {
        pthread_attr_destroy(&taskAttr);
        iResult = TASK_RT_CREATE_ERROR;
        goto TASK_SPAWN_ERROR_PROCESS;
    }

    /*设置线程的优先级*/
    if(priority != 0)
    {
        schedParm.sched_priority = priority;
        pthread_attr_setschedparam(&taskAttr, &schedParm);
    }


    /*启动线程*/
    /*更新当前的任务表*/
    taskEntry->taskPri = priority;
    strncpy((char*)taskEntry->taskName, (char*)name, TASK_MAX_TASK_NAME);//modified by haijig
   // strncpy((void*)taskEntry->taskName, (void*)name, TASK_MAX_TASK_NAME);
    taskEntry->taskRestartPtr = pFunc;
    taskEntry->taskStackSize = stackSize;
    taskEntry->taskOption = options;
    taskEntry->checkFlag =  checkFlag;

    if(0 != pthread_create(&taskId, &taskAttr, (OS_TASK_PTR)pFunc,  arg))
    {
        pthread_attr_destroy(&taskAttr);
        iResult = TASK_RT_CREATE_ERROR;
        goto TASK_SPAWN_ERROR_PROCESS;
    }

//    semWLockTake(g_TaskRwLock, 0);
    taskEntry->taskId = taskId;
//    semRwGive(g_TaskRwLock);

    pthread_detach(taskId);//设置为分离线程
    pthread_attr_destroy(&taskAttr);

    /*分配全局任务ID*/
//    return taskEntryId-1;
    return (taskEntry - g_TaskTable) / sizeof(TG_TASK_INFO_ENTRY);

TASK_SPAWN_ERROR_PROCESS:
    /*释放参数的内存*/

    /*释放任务表的表项*/
//    if (taskEntryId > 0)
//    {
//        taskFreeTaskEntry(taskEntryId);
//    }
    return iResult;
}
/**************************************************************************
** 函数名:
** 输  入:
** 输  出:
** 描  述:
** 作  者: xiaodong
** 日  期:
** 修  改:
** 备  注:
**************************************************************************/
INT32 taskRegister()
{
    INT32 iIndex = taskIdSelf();
    time_t curTime;
    memset((void*)&curTime, 0, sizeof(curTime));

    if(iIndex < 0)
        return TASK_RT_ERROR;

    curTime = time(NULL);

    /*累加注册的次数*/

    ++g_TaskTable[iIndex].taskRegEntry.uiExeCnt;

    memcpy((void*)&g_TaskTable[iIndex].taskRegEntry.tgExeTime, (void*)&curTime, sizeof(time_t));

    return TASK_RT_OK;
}

INT32 taskRegisterPid()
{
    INT32 iIndex = taskIdSelf();
    g_TaskTable[iIndex].taskPid = syscall(SYS_gettid);
    return TASK_RT_OK;
}
/**************************************************************************
** 函数名:
** 输  入:
** 输  出:
** 描  述:
** 作  者: xiaodong
** 日  期:
** 修  改:
** 备  注:
**************************************************************************/
INT32 taskIdSelf()
{
    UINT32 uiCount = 0;
    pthread_t pThread = pthread_self();

    /*遍历找到对应的TASK ENTRY*/
    for(uiCount = 0; uiCount < g_threadMuxNum; uiCount++)
    {
        if(pthread_equal(g_TaskTable[uiCount].taskId, pThread))
        {
            return uiCount;
        }
    }

    return TASK_RT_ERROR;
}

/**************************************************************************
** 函数名:
** 输  入:
** 输  出:
** 描  述:
** 作  者: xiaodong
** 日  期:
** 修  改:
** 备  注:
**************************************************************************/
void taskSendTestMsg()
{
    UINT32 uiCount, taskEntryId;

    for(taskEntryId = 0; taskEntryId < g_threadMuxNum; taskEntryId++)
    {
        TG_TASK_INFO_ENTRY *taskEntry = g_TaskTable + taskEntryId;
        /*如果当前任务ID为空, 则直接跳转到下一条记录*/
        if(taskEntry->taskId  == 0)
        {
            continue;
        }
        /*查找当前任务的消息队列记录表，如果有记录，调用响应的发送测试包函数*/
        for(uiCount = 0; uiCount < TASK_MAX_MSG_QUEUE; uiCount++)
        {
            if(taskEntry->taskMsgQueue[uiCount] != -1)
            {
                if(taskEntry->SendTestMsg[uiCount] != NULL)
                {
                    taskEntry->SendTestMsg[uiCount](NULL);
                }
            }
        }
    }
}

/**************************************************************************
** 函数名:
** 输  入:
** 输  出:
** 描  述:
** 作  者: xiaodong
** 日  期:
** 修  改:
** 备  注:
**************************************************************************/
BOOL taskDeadLockCheck()//死锁检测
{
    UINT32 uiCount = 0;
    UINT32 deadLockCheckFlag = 0;
    for(uiCount = 0; uiCount < g_threadMuxNum; uiCount++)
    {
        TG_TASK_INFO_ENTRY *taskEntry = g_TaskTable + uiCount;
        /*如果当前任务ID为空,或者当前任务不检测,则直接跳转到下一条记录*/
        if(taskEntry->taskId  == 0 ||  taskEntry->checkFlag == 0)
        {
            continue;
        }

        /*如果查询到此次执行时间和上执行次时间，说明线程没有继续执行*/
        if(taskEntry->taskRegEntry.tgLastExeTime == taskEntry->taskRegEntry.tgExeTime)
        {
            char szBuf[64] = {0};
            time_t curTime;
            BN_LOG_ERROR("task %s:0X%X Dead Lock Detected!\n", taskEntry->taskName, (unsigned int)taskEntry->taskId);
            //SYSMONITOR_DEBUG_ERR("task %s:0X%X Dead Lock Detected!\n", taskEntry->taskName, (unsigned int)taskEntry->taskId);
            curTime = time(NULL);
            strncpy(szBuf, (char*)&curTime, sizeof(szBuf));
            BN_LOG_ERROR("CurTime       :%s", szBuf);
            //SYSMONITOR_DEBUG_ERR("CurTime       :%s", szBuf);
            memset(szBuf, 0, sizeof(szBuf));
            strncpy(szBuf, ctime(&(taskEntry->taskRegEntry.tgExeTime)), sizeof(szBuf));
            BN_LOG_ERROR("ExTime       :%s", szBuf);
            //SYSMONITOR_DEBUG_ERR("ExTime       :%s", szBuf);
            memset(szBuf, 0, sizeof(szBuf));
            strncpy(szBuf, ctime(&(taskEntry->taskRegEntry.tgLastExeTime)), sizeof(szBuf));
            BN_LOG_ERROR("tgLastExeTime:%s", szBuf);
            BN_LOG_ERROR("ExeCnt       :%d\n", taskEntry->taskRegEntry.uiExeCnt);
            //SYSMONITOR_DEBUG_ERR("tgLastExeTime:%s", szBuf);
            //SYSMONITOR_DEBUG_ERR("ExeCnt       :%d\n", taskEntry->taskRegEntry.uiExeCnt);


            sprintf(szBuf, "kill -10 %d", taskEntry->taskPid);
            system(szBuf);
            taskDelay(100);
            deadLockCheckFlag++;
        }
        else
        {
            /*如果查询到此次执行时间和上执行次时间不一致，说明线程有继续执行*/
            taskEntry->taskRegEntry.tgLastExeTime = taskEntry->taskRegEntry.tgExeTime;
//            memcpy((void*)&taskEntry->taskRegEntry.tgLastExeTime, (void*)&taskEntry->taskRegEntry.tgExeTime, sizeof(time_t));
        }
    }

    if(deadLockCheckFlag != 0)
    {
        return true;
    }
    return false;
}
/**************************************************************************
** 函数名:
** 输  入:
** 输  出:
** 描  述: 微秒级延迟
** 作  者: xiaodong
** 日  期:
** 修  改:
** 备  注:
**************************************************************************/
INT32 taskDelay(time_t ticks)
{
    int sec = 0;
    struct timespec delayTime;
    struct timespec leftTime;
    memset((void*)&delayTime, 0, sizeof(struct timespec));
    memset((void*)&leftTime, 0, sizeof(struct timespec));
    /*ns最大值为999999, 超过此值将无效, 故将时间戳大于1S的部分存放到sec中*/
    if((sec = ticks / (1000 * 1000)) > 0)
    {
        delayTime.tv_sec = sec;
        ticks -= sec * 1000 * 1000;
    }
    delayTime.tv_nsec = ticks * 1000; /*not tv_nsec,tv_nsec is nanosecond*/

    /*nanosleep会被信号异常中断, 所以为了保证taskDelay时间准确, 一旦nanosleep返回-1就继续睡眠*/
    while(nanosleep(&delayTime, &leftTime) == -1)
    {
        memcpy((void*)&delayTime, (void*)&leftTime, sizeof(struct timespec));
        memset((void*)&leftTime, 0, sizeof(struct timespec));
    }

    return TASK_RT_OK;
}

/**************************************************************************
** 函数名:
** 输  入:
** 输  出:
** 描  述:
** 作  者: xiaodong
** 日  期:
** 修  改:
** 备  注:
**************************************************************************/
INT32 taskMsgRec(INT32 msqid, void *msgp, size_t msgsz, OS_TASK_PTR pSendTestFun)
{
    INT32 iTaskId = taskIdSelf();/*遍历找到对应的TASK ENTRY*/
    TG_TASK_INFO_ENTRY *taskEntry = g_TaskTable + iTaskId;
    UINT32 uiCount;

    for(uiCount = 0; uiCount < TASK_MAX_MSG_QUEUE; uiCount++)
    {
        if(msqid == taskEntry->taskMsgQueue[uiCount])
        {
            break;
        }

        if(-1 == taskEntry->taskMsgQueue[uiCount])
        {
            taskEntry->taskMsgQueue[uiCount] = msqid;
            taskEntry->SendTestMsg[uiCount] = pSendTestFun;
            break;
        }
    }

    if(uiCount == TASK_MAX_MSG_QUEUE)
    {
    	BN_LOG_ERROR("%s thread messageQues is out of range \n", taskEntry->taskName);
    	//SYSMONITOR_DEBUG_ERR("%s thread messageQues is out of range \n", taskEntry->taskName);
    }

    return(msgrcv(msqid, msgp, msgsz, 0, 0));
}

/**************************************************************************
** 函数名:
** 输  入:
** 输  出:
** 描  述:
** 作  者: xiaodong
** 日  期:
** 修  改:
** 备  注:
**************************************************************************/
int taskWatchFlag = 1;
int taskMonitorFlag = 1;
int singalFlag =1 ;

void* taskWatch(void* pArg)
{
    /*******

    ***************检测死锁的算法**************************
    *1. 1.5分钟一次扫描 任务注册表
    *2. 如果1.5分钟内任务的注册次数 不大于0 则认为是发生了死锁
    *3. 如果大于0, 则将此任务的注册次数清0
    ****************************************************************/

    TASK_INIT_ACTION();

    setTaskMonitorFlag(1);
    setTaskWatchFalg(1);
    setSingalFlag(1);

    /*初始化监控消息队列*/
    if(initMonitorMsgque() == SYS_RC_ERR)
    {
    	BN_LOG_ERROR("%s:initMsg err\n", __func__);
    	//SYSMONITOR_DEBUG_ERR("%s:initMsg err\n", __func__);
        return NULL ;
    }
    UCHAR msgSrc = *((UCHAR*)pArg);

    /*监测当前的任务列表*/
    while(getSingalFlag())
    {
        if(taskWatchFlag == 1)
        {
            //向监控进程发送注册消息
            if(taskMonitorFlag == 1)
            {
                sendMonitorMsg(msgSrc);
            }
            taskDelay(MAKE_SEC(10));//10秒为周期向监控进程发送消息（钟）

//由于当前没有监控线程，这里就不需要了 start ，后续需要监控线程时打开
//            //发送消息队列测试包
//            taskSendTestMsg();
//            taskDelay(MAKE_SEC(10));

//            //向监控进程发送注册消息
//            if(taskMonitorFlag == 1)
//            {
//                sendMonitorMsg(msgSrc);
//            }
//            taskDelay(MAKE_SEC(10));
//

//            /*死锁检测*/
//            if(taskDeadLockCheck())
//            {
//                SYSMONITOR_DEBUG_ERR("%s Dead Lock Detected! kill self!\n", __func__);
//                /*记录日志*/

//                /*程序退出*/
//                if(TG_TW_EXIT_PROCESS == g_TaskWatchAtionType)
//                {
//                    taskRecord();
//                    _exit(-1);
//                }
//            }
//由于当前没有监控线程，这里就不需要了 end
        }
        else
        {
            if(taskMonitorFlag == 1)
            {
                sendMonitorMsg(msgSrc);
            }

            taskDelay(MAKE_SEC(10));
        }
    }
}

/**************************************************************************
** 函数名: setTaskWatchFalg
** 输  入: value=1 表示监控，其他值不监控
** 输  出:
** 描  述: 设置监控任务标志
** 作  者: xiaodong
** 日  期:
** 修  改:
** 备  注:
**************************************************************************/
void setTaskWatchFalg(int value)
{
    taskWatchFlag = value;
}

int getTaskWatchFalg()
{
    return taskWatchFlag;
}

/**************************************************************************
** 函数名: setTaskMonitorFlag
** 输  入: value=1 表示监控，其他值不监控
** 输  出:
** 描  述: 设置监控任务标志
** 作  者: xiaodong
** 日  期:
** 修  改:
** 备  注:
**************************************************************************/
void setTaskMonitorFlag(int value)
{
    taskMonitorFlag = value;
}

int getTaskMonitorFlag()
{
    return taskMonitorFlag;
}

/**************************************************************************
** 函数名: setSingalFlag
** 输  入: 适配signal接管进程
** 输  出:
** 描  述: 设置监控任务标志
** 作  者: xiaodong
** 日  期:
** 修  改:
** 备  注:
**************************************************************************/
int setSingalFlag(int value)
{
    singalFlag  = value;
}

int getSingalFlag()
{
    return singalFlag;
}

/**************************************************************************
** 函数名:
** 输  入:
** 输  出:
** 描  述:
** 作  者: xiaodong
** 日  期:
** 修  改:
** 备  注:
**************************************************************************/
INT32 taskWatchTaskEnable()
{
    INT32 iTaskWatchId = taskSpawn("taskWatch",
                                   TASK_DEFAULT_PRIORITY, TASK_DEFAULT_OPTION,
                                   TASK_DEFAULT_STACK_SIZE, 0, taskWatch,
                                   NULL);

    return iTaskWatchId;

}
/**************************************************************************
** 函数名:
** 输  入:
** 输  出:
** 描  述:
** 作  者: xiaodong
** 日  期:
** 修  改:
** 备  注:
**************************************************************************/
INT32 taskShow()//打印当前任务列表
{
    UINT32 uiCount;
    char szBuf[64];
    const char szFormat[] = {"%-2d    %-16s    0x%-8x       %-8d    %-8d    0x%-8x    0x%-8x    %s\n\r"};
   // SYSMONITOR_DEBUG_INFO("taskWatchFlag:%d\n", taskWatchFlag);
    /*遍历打印当前的任务列表*/
   // SYSMONITOR_DEBUG_INFO("Id    Name                ThreadId      ThreadPid      Priority    StackSize     ExeCnt        ExTime(Hello Zhonghaijing!!!)\n\r");
   // SYSMONITOR_DEBUG_INFO("------------------------------------------------------------------------------------------------------\n\r");

    for(uiCount = 0; uiCount < g_threadMuxNum; uiCount++)
    {
        if(g_TaskTable[uiCount].taskId != 0)
        {
            memset(szBuf, 0, sizeof(szBuf));
            strncpy(szBuf, ctime(&g_TaskTable[uiCount].taskRegEntry.tgExeTime), sizeof(szBuf));
           // SYSMONITOR_DEBUG_INFO(szFormat, uiCount, g_TaskTable[uiCount].taskName, (unsigned int)g_TaskTable[uiCount].taskId, g_TaskTable[uiCount].taskPid,
                                 // g_TaskTable[uiCount].taskPri,  g_TaskTable[uiCount].taskStackSize, g_TaskTable[uiCount].taskRegEntry.uiExeCnt, szBuf);
        }
    }
   // SYSMONITOR_DEBUG_INFO("------------------------------------------------------------------------------------------------------\n\r");
    return TASK_RT_OK;
}

/**************************************************************************
** 函数名:
** 输  入:
** 输  出:
** 描  述:
** 作  者: xiaodong
** 日  期:
** 修  改:
** 备  注:
**************************************************************************/
INT32 taskRecord()
{
    UINT32 uiCount;
    char szBuf[64];
    const char szFormat[] = {"%-2d    %-16s    0x%-8x       %-8d    %-8d    0x%-8x    0x%-8x    %s\n\r"};
    BN_LOG_ERROR("taskWatchFlag:%d\n", taskWatchFlag);
    //SYSMONITOR_DEBUG_ERR("taskWatchFlag:%d\n", taskWatchFlag);
    /*遍历打印当前的任务列表*/
    BN_LOG_ERROR("Id    Name                ThreadId      ThreadPid      Priority    StackSize     ExeCnt        ExTime\n\r");
    BN_LOG_ERROR("------------------------------------------------------------------------------------------------------\n\r");

    //SYSMONITOR_DEBUG_ERR("Id    Name                ThreadId      ThreadPid      Priority    StackSize     ExeCnt        ExTime\n\r");
    //SYSMONITOR_DEBUG_ERR("------------------------------------------------------------------------------------------------------\n\r");

    for(uiCount = 0; uiCount < g_threadMuxNum; uiCount++)
    {
        if(g_TaskTable[uiCount].taskId != 0)
        {
            memset(szBuf, 0, sizeof(szBuf));
            strncpy(szBuf, ctime(&g_TaskTable[uiCount].taskRegEntry.tgExeTime), sizeof(szBuf));
            BN_LOG_ERROR(szFormat, uiCount, g_TaskTable[uiCount].taskName, (unsigned int)g_TaskTable[uiCount].taskId, g_TaskTable[uiCount].taskPid,
                                 g_TaskTable[uiCount].taskPri,  g_TaskTable[uiCount].taskStackSize, g_TaskTable[uiCount].taskRegEntry.uiExeCnt, szBuf);

            //SYSMONITOR_DEBUG_ERR(szFormat, uiCount, g_TaskTable[uiCount].taskName, (unsigned int)g_TaskTable[uiCount].taskId, g_TaskTable[uiCount].taskPid,
                                 //g_TaskTable[uiCount].taskPri,  g_TaskTable[uiCount].taskStackSize, g_TaskTable[uiCount].taskRegEntry.uiExeCnt, szBuf);
        }
    }
    BN_LOG_ERROR("------------------------------------------------------------------------------------------------------\n\r");
    //SYSMONITOR_DEBUG_ERR("------------------------------------------------------------------------------------------------------\n\r");
    for(uiCount = 0; uiCount < g_threadMuxNum; uiCount++)
    {
        memset(szBuf, 0, sizeof(szBuf));
        sprintf(szBuf, "kill -10 %d", g_TaskTable[uiCount].taskPid);
        system(szBuf);
        taskDelay(100);
    }
    return TASK_RT_OK;
}

/**************************************************************************
** 函数名:
** 输  入:
** 输  出:
** 描  述:
** 作  者: xiaodong
** 日  期:
** 修  改:
** 备  注:
**************************************************************************/
int backtrace_xd(void **array, int size, void* _fp)
{
    if(size <= 0)
        return 0;

    int *fp = 0, *next_fp = 0;
    int cnt = 0;
    int ret = 0;

    fp = (int *)_fp;

    array[cnt++] = (void *)(*fp);

    next_fp = (int *)(*(fp - 1));

    //printf("sp:0X%X = 0X%X\n",fp-3,next_fp);
    while((cnt <= size) && (next_fp != 0))
    {
        //printf("fp:0X%X\n",next_fp);
        array[cnt++] = (void *) * (next_fp);
        //printf("pc:0X%X = 0X%X\n",next_fp,*next_fp);
        //printf("next_fp:0X%X = 0X%X\n",(int*)next_fp-1,*((int*)next_fp-1));
        next_fp = (int *)(*(next_fp - 1));
    }

    ret = ((cnt <= size) ? cnt : size);
    //printf("Backstrace (%d deep)\n", ret);

    return ret;
}

//char ** backtrace_symbols_apcomm(void *const *array, int size)
//{
//# define WORD_WIDTH 8
//    Dl_info info[size];
//    int status[size];
//    int cnt;
//    size_t total = 0;
//    char **result;
//
//    /* Fill in the information we can get from `dladdr'.  */
//    for(cnt = 0; cnt < size; ++cnt)
//    {
//        status[cnt] = dladdr(array[cnt], &info[cnt]);
//        if(status[cnt] && info[cnt].dli_fname && info[cnt].dli_fname[0] != '\0')
//            /* We have some info, compute the length of the string which will be
//            "<file-name>(<sym-name>) [+offset].  */
//            total += (strlen(info[cnt].dli_fname ? : "")
//                      + (info[cnt].dli_sname ? strlen(info[cnt].dli_sname) + 3 + WORD_WIDTH + 3 : 1)
//                      + WORD_WIDTH + 5);
//        else
//            total += 5 + WORD_WIDTH;
//    }
//
//
//    /* Allocate memory for the result.  */
//    result = (char **) malloc(size * sizeof(char *) + total);
//    if(result != NULL)
//    {
//        char *last = (char *)(result + size);
//
//        for(cnt = 0; cnt < size; ++cnt)
//        {
//            result[cnt] = last;
//
//            if(status[cnt] && info[cnt].dli_fname && info[cnt].dli_fname[0] != '\0')
//            {
//                char buf[20];
//
//                if(array[cnt] >= (void *) info[cnt].dli_saddr)
//                    sprintf(buf, "+%#lx", \
//                            (unsigned long)(array[cnt] - info[cnt].dli_saddr));
//                else
//                    sprintf(buf, "-%#lx", \
//                            (unsigned long)(info[cnt].dli_saddr - array[cnt]));
//
//                last += 1 + sprintf(last, "%s%s%s%s%s[%p]",
//                                    info[cnt].dli_fname ? : "",
//                                    info[cnt].dli_sname ? "(" : "",
//                                    info[cnt].dli_sname ? : "",
//                                    info[cnt].dli_sname ? buf : "",
//                                    info[cnt].dli_sname ? ") " : " ",
//                                    array[cnt]);
//            }
//            else
//                last += 1 + sprintf(last, "[%p]", array[cnt]);
//        }
//        assert(last <= (char *) result + size * sizeof(char *) + total);
//    }
//
//    return result;
//}//commented by haijing
//#ifdef __ARM__
//void debugBacktrace(unsigned int sn , siginfo_t  *si , void *ptr)
//{
//    /*int *ip = 0;
//    __asm__(
//        "mov %0, ip\n"
//        : "=r"(ip)
//    );
//    printf("sp = 0x%x\n", ip);
//    struct sigframe_ce123 * sigframe = (struct sigframe_ce123 * )ip;*/
//
//    if(NULL != ptr)
//    {
//        BN_LOG_ERROR("\n\nreceived signal (%d) at: 0x%08x\n", si->si_signo, si->si_addr);
//        //SYSMONITOR_DEBUG_ERR("\n\nreceived signal (%d) at: 0x%08x\n", si->si_signo, si->si_addr);
//        struct ucontext_ce123 *ucontext = (struct ucontext_ce123 *)ptr;
//        int pc = ucontext->uc_mcontext.arm_pc;
//
//        void *pc_array[1];
//        pc_array[0] = pc;
//        char **pc_name = backtrace_symbols_apcomm(pc_array, 1);
//        BN_LOG_ERROR("%d: %s\n", 0, *pc_name);
//        //SYSMONITOR_DEBUG_ERR("%d: %s\n", 0, *pc_name);
//
//
//
//#define SIZE 100
//        void *array[SIZE];
//        int size, i;
//        char **strings;
//        size = backtrace_xd(array, SIZE, (void*)ucontext->uc_mcontext.arm_fp);
//        //printf("size %d\n",size);
//
//        strings = backtrace_symbols_apcomm(array, size);
//
//        for(i = 0; i < size; i++)
//            BN_LOG_ERROR("%d: %s\n", i + 1, strings[i]);
//        //SYSMONITOR_DEBUG_ERR("%d: %s\n", i + 1, strings[i]);
//        free(strings);
//
//
//    }
//    else
//        BN_LOG_ERROR("error!\n");
//    //SYSMONITOR_DEBUG_ERR("error!\n");
//
//    if(si->si_signo == SIGSEGV)
//    {
//        exit(-1);
//    }
//}
//#else
//void debugBacktrace(unsigned int sn , siginfo_t  *si , void *ptr)
//{}
//#endif

//extern "C"
//{
//void registerSIGSEGV()
//{
//    struct sigaction s;
//    s.sa_flags = SA_SIGINFO;
//    s.sa_sigaction = (void *)debugBacktrace;
//
//    sigaction(SIGSEGV, &s, NULL);
//    sigaction(SIGUSR1, &s, NULL);
//}

//}//commented by haijing


