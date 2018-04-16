/*
 * bnthread.cpp
 *
 *  Created on: 2017年8月22日
 */
#include <memory.h>
#include <malloc.h>
#include <iostream>
#include <assert.h>
#include "bnosutils.h"
#include "bnthread.h"
#include "bnconst.h"

// TODO: use automake tool to find out if HAVE_SYS_PRCTL_H is defined
#ifdef WIN32
#else
#include <sys/prctl.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#endif
#if (defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__DragonFly__))
#include <pthread.h>
#include <pthread_np.h>
#endif
// End for set thread's name

// For set thread's priority
#ifdef WIN32
#include <windows.h>
#else
#include <sys/resource.h>
#endif
// End for set thread's priority

// For POSIX timer handle
#include <signal.h>
#include <time.h>
// End for POSIX timer handle

static void TimerHandle( int sig, siginfo_t *si, void *uc ){
    struct BnThreadTimerMsg *tTimerMsg;
    tTimerMsg = (struct BnThreadTimerMsg *)(si->si_value.sival_ptr);
    if(NULL == tTimerMsg)
    {
    	return;
    }

    struct ThreadMsg threadMsg;
    threadMsg.m_id = tTimerMsg->iMsgId;

    // Wait until message queue is available
    msgsnd(tTimerMsg->m_iMsgQueueId, &threadMsg, sizeof(struct ThreadMsg), 0);
}

int SendMsgToQueueId(int iQueueId, int iMsgId, const void *pvData, int iLength){
    struct ThreadMsg *pThreadMsg;

    char *pcNewData = NULL;
    int iMsgLength = iLength + sizeof(struct ThreadMsg);
    pcNewData = (char *)malloc(iMsgLength);
    if(NULL == pcNewData){
        return ERROR_NOMEMORY;
    }
    memset(pcNewData, 0, iMsgLength);

    pThreadMsg = (struct ThreadMsg *)pcNewData;
    pThreadMsg->m_id = iMsgId;

    if(NULL != pvData && iLength > 0){
        memcpy(pcNewData + sizeof(struct ThreadMsg), pvData, iLength);
    }

    int iResult = msgsnd(iQueueId, (void *)pcNewData, iMsgLength, 0);
    free(pcNewData);

    return iResult;
}

int SendMsgToQueueIdNoWait(int iQueueId, int iMsgId, const void *pvData, int iLength){
    struct ThreadMsg *pThreadMsg;

    char *pcNewData = NULL;
    int iMsgLength = iLength + sizeof(struct ThreadMsg);
    pcNewData = (char *)malloc(iMsgLength);
    if(NULL == pcNewData){
        return ERROR_NOMEMORY;
    }
    memset(pcNewData, 0, iMsgLength);

    pThreadMsg = (struct ThreadMsg *)pcNewData;
    pThreadMsg->m_id = iMsgId;

    if(NULL != pvData && iLength > 0){
        memcpy(pcNewData + sizeof(struct ThreadMsg), pvData, iLength);
    }

    int iResult = msgsnd(iQueueId, (void *)pcNewData, iMsgLength, IPC_NOWAIT);
    free(pcNewData);

    return iResult;
}

BnThread::BnThread(const char* pcThreadName, int iMsgQueueKey, int iPriority) :
            m_pThread(NULL),
            m_tMsgQueueKey(iMsgQueueKey),
            m_iThreadPriority(iPriority){
    int iLength = strlen(pcThreadName);
    m_pcThreadName = new char[iLength];
    m_pcThreadName[iLength] = '\0';
    memcpy((void *)m_pcThreadName, pcThreadName, iLength);//m_pcThreadName means the name of the thread(zhong)

    RegisterThread();

}

BnThread::~BnThread() {
    if(NULL != m_pcThreadName){
        delete m_pcThreadName;
    }

    DeleteAllTimer();

    ExitThread();
}

ERRCODE BnThread::RegisterThread(){
    if (NULL == m_pThread){
        m_pThread = new std::thread(&BnThread::Process, this);//new the dispatch-thread(zhong)
        if(NULL == m_pThread ){
            return ERROR_NOMEMORY;
        }

        // 获取唯一的消息队列ID
        m_iMsgQueueId = msgget(m_tMsgQueueKey, 0666 | IPC_CREAT);//new or get the msgQueue
        if(-1 == m_iMsgQueueId){
            return ERROR_MSGQUEUEERROR;
        }
    }
    return SUCCESS;
}

std::thread::id BnThread::GetThreadId(){
    std::thread::id tId;
    if(NULL == m_pThread){
        assert(0);
    }else{
        tId = m_pThread->get_id();
    }
    return tId;
}

void BnThread::ExitThread(){
    if (NULL == m_pThread){
        return;
    }

    // Send a new ThreadMsg and wait when queue is full
    PostMsg(BN_MSG_EXIT_THREAD, NULL, 0);
}

void BnThread::join(){
    if (NULL == m_pThread){
        return;
    }

    m_pThread->join();
}

void BnThread::SetPriority(int iPriority){
    if(BN_THREAD_PRIORITY_MIN > iPriority || BN_THREAD_PRIORITY_MAX < iPriority){
        return;
    }

    if(m_iThreadPriority != iPriority){
        if (NULL == m_pThread){
            return;
        }

        PostMsgNoWait(BN_MSG_CHANGE_PRIORITY, &iPriority, sizeof(int));
    }
}

ERRCODE BnThread::RegisterTimer(){
    return SUCCESS;
}

ERRCODE BnThread::CreateTimer(int iTimerId, int iIntervalMs){
    struct sigevent         tSigEvent;
    struct itimerspec       tTimerSpec;
    struct sigaction        tSigAction;
    int                     iSigNo = SIGRTMIN;

    timer_t                 *pTimer = new timer_t();
    if(NULL == pTimer){
        return ERROR_NOMEMORY;
    }

    struct BnThreadTimerMsg *pTimerMsg = new struct BnThreadTimerMsg();
    if(NULL == pTimerMsg){
        delete pTimer;
        return ERROR_NOMEMORY;
    }

    /* Set up signal handler. */
    tSigAction.sa_flags = SA_SIGINFO;
    tSigAction.sa_sigaction = TimerHandle;
    sigemptyset(&tSigAction.sa_mask);
    if (-1 == sigaction(iSigNo, &tSigAction, NULL)){
        printf("Failed to setup signal handling for %d.\n", iTimerId);
        return ERROR_TIMERCREATEERROR;
    }

    /* Set and enable alarm */
    pTimerMsg->m_iMsgQueueId = m_iMsgQueueId;
    pTimerMsg->iMsgId = iTimerId;
    pTimerMsg->tTimerId = pTimer;
    tSigEvent.sigev_notify = SIGEV_SIGNAL;
    tSigEvent.sigev_signo = iSigNo;
    tSigEvent.sigev_value.sival_ptr = pTimerMsg;
    int ret = timer_create(CLOCK_REALTIME, &tSigEvent, pTimer);
    m_mapTimerMsg.insert(std::make_pair(iTimerId, pTimerMsg));
    if(0 != ret){
        printf("timer_create return %d\n", ret);
    }

    tTimerSpec.it_interval.tv_sec = iIntervalMs / 1000;
    tTimerSpec.it_interval.tv_nsec = (iIntervalMs % 1000) * 1000000;
    tTimerSpec.it_value.tv_sec = tTimerSpec.it_interval.tv_sec;
    tTimerSpec.it_value.tv_nsec = tTimerSpec.it_interval.tv_nsec;
    ret = timer_settime(*pTimer, 0, &tTimerSpec, NULL);
    if(0 != ret){
        printf("timer_settime return %d\n", ret);
    }
    return SUCCESS;
}

ERRCODE BnThread::DeleteAllTimer(){
    std::map<int, struct BnThreadTimerMsg *>::iterator iterTimer = m_mapTimerMsg.begin();
    while(m_mapTimerMsg.end() != iterTimer){
        struct BnThreadTimerMsg *pTimerMsg = (struct BnThreadTimerMsg *)iterTimer->second;
        if (NULL != pTimerMsg) {
            timer_t *pTimerId = pTimerMsg->tTimerId;
            if (NULL != pTimerId) {
                timer_delete(*pTimerId);
                delete pTimerId;
            }
            delete pTimerMsg;
        }
        m_mapTimerMsg.erase(iterTimer++);
    }
    return SUCCESS;
}

ERRCODE BnThread::PostMsg(int iMsgId, const void* pvData, int iLength){
    return SendMsgToQueueId(GetMsgQueueId(), iMsgId, pvData, iLength);
}

ERRCODE BnThread::PostMsgNoWait(int iMsgId, const void* pvData, int iLength){
    return  SendMsgToQueueIdNoWait(GetMsgQueueId(), iMsgId, pvData, iLength);
}

void BnThread::DeleteMessageQueue(){
    // release message queue in kernel
    msgctl(m_iMsgQueueId, IPC_RMID, 0);
}

void BnThread::Process(){
    RenameThread(m_pcThreadName);//allow the name to thread(zhong)

    // Require root
    SetThreadPriority(m_iThreadPriority);//set the priority of the thread(zhong)

    // Register timer at here
    RegisterTimer();

    char *pcMsgBuffer = new char[MSGMAX];

    while (true){
        struct ThreadMsg *pThreadMsg = NULL;
        if(NULL == pcMsgBuffer){
            pcMsgBuffer = new char[MSGMAX];
            continue;
        }

        int iMsgLength = msgrcv(m_iMsgQueueId, pcMsgBuffer, MSGMAX, 0, MSG_NOERROR);//now always receive message from m_iMsgQueueId(zhong)
        if (-1 == iMsgLength)                                                       //and return the actual length of the message
        {
            // Wait for a message to be added to the queue
            std::cout << "Receive from message queue 0x" << std::hex << m_iMsgQueueId << " failed" << std::endl;
            continue;
        }

        pThreadMsg = (struct ThreadMsg *)pcMsgBuffer;

        switch (pThreadMsg->m_id){
            case BN_MSG_EXIT_THREAD:{
                DeleteMessageQueue();//delete the message queue(zhong)
                std::cout << "Exit thread on " << m_pcThreadName << std::endl;

                m_pThread->join();
                delete m_pThread;
                m_pThread = NULL;
                break;
            }
            case BN_MSG_CHANGE_PRIORITY: {
                int *piPriority = (int *)(pcMsgBuffer + sizeof(struct ThreadMsg));
                SetThreadPriority(*piPriority);
                break;
            }
            case BN_MSG_CHANGE_NAME: {
                const char *pcName = (const char *)(pcMsgBuffer + sizeof(struct ThreadMsg));
                RenameThread(pcName);
                if(NULL != m_pcThreadName){
                    delete m_pcThreadName;
                }
                int iLength = strlen(pcName);
                m_pcThreadName = new char[iLength];
                memcpy((void *)m_pcThreadName, pcName, iLength);
                break;
            }
            default: {
                OnMessage(pThreadMsg->m_id,
                        iMsgLength - sizeof(struct ThreadMsg),
                        pcMsgBuffer + sizeof(struct ThreadMsg));
                break;
            }
        }
    }

    delete pcMsgBuffer;
}

void BnThread::OnMessage(int iMsgId, int iLength, void *pvData){
    return;
}


void BnThread::RenameThread(const char* pcThreadName){
#if defined(PR_SET_NAME)
    // Only the first 15 characters are used (16 - NUL terminator)
    ::prctl(PR_SET_NAME, pcThreadName, 0, 0, 0);
#elif (defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__DragonFly__))
    pthread_set_name_np(pthread_self(), pcThreadName);

#elif defined(MAC_OSX)
    pthread_setname_np(pcThreadName);
#else
    // Prevent warnings for unused parameters...
    (void)pcThreadName;
#endif
}


void BnThread::SetThreadPriority(int iPriority){
#ifdef WIN32
    SetThreadPriority(GetCurrentThread(), iPriority);
#else
#ifdef PRIO_THREAD
    setpriority(PRIO_THREAD, 0, iPriority);
#else
    setpriority(PRIO_PROCESS, 0, iPriority);
#endif
#endif
    m_iThreadPriority = iPriority;
}




