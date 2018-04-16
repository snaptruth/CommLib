/*
 * BnLogDispatchThread.cpp
 *
 *  Created on: 2017年9月19日
 */
#include "bnlogdispatchthread.h"
#include "bnlogrecoderthread.h"
#include <iostream>
#include <sys/time.h>

int BnLogDispatchThread::m_siMsgQueueIdNum = 1;

BnLogDispatchThread::BnLogDispatchThread() : BnThread("BnLogDispatch",
        BM_MSG_QUEUE_LOGDISTRIBUTE,
        BN_THREAD_PRIORITY_25) {
    // TODO Auto-generated constructor stub
    m_iLogIndex = 0;
}

BnLogDispatchThread::~BnLogDispatchThread() {
    m_mapModuleLogEnable.clear();
    std::map<int, BnThread *>::iterator iterModuleThread = m_mapModuleThread.begin();
    for(; m_mapModuleThread.end() != iterModuleThread;){
        BnThread *pLogThread = (BnThread *)iterModuleThread->second;
        if(NULL != pLogThread){
            delete pLogThread;
        }
        m_mapModuleThread.erase(iterModuleThread++);
    }
}

void BnLogDispatchThread::OnMessage(int iMsgId, int iLength, void *pvData){
    int iModuleMark = iMsgId >> 16;//the mark of the module(zhong)
    iMsgId = iMsgId & 0xFFFF;//decide the type of the message(zhong)

    switch (iMsgId) {
    case BN_MSG_LOG_CREATE: {
        int iCurQueueNum = m_siMsgQueueIdNum;
        m_siMsgQueueIdNum++;
        SetMsgQueueKey((key_t) (BM_MSG_QUEUE_STARTID + iCurQueueNum));//and create different message queue for the different module(zhong)
        BnThread *pLogRecoder = GetLogRecoder(iModuleMark);
        if (NULL == pLogRecoder && NULL != pvData) {
            CreateLogRecoder(iModuleMark, GetMsgQueueKey(), iLength, pvData);//create the recorder for the corresponding module(zhong)
        }
        break;
    }
    case BN_MSG_LOG_ENABLE: {
        if(4 == iLength && NULL != pvData){
            int iLogEnable = ((int *)pvData)[0];
            m_mapModuleLogEnable[iModuleMark] = iLogEnable;
        }
        BnThread *pLogRecoder = GetLogRecoder(iModuleMark);
        if (NULL != pLogRecoder) {
            int iMsgQueueId = pLogRecoder->GetMsgQueueId();
            SendMsgToQueueIdNoWait(iMsgQueueId, iMsgId, pvData, iLength);
        }
        break;
    }

    case BN_MSG_LOG_DEBUG: {
        BnThread *pLogRecoder = GetLogRecoder(iModuleMark);
        if (NULL != pLogRecoder
                && 0 != (m_mapModuleLogEnable[iModuleMark] & BN_LOGLEVEL_DEBUG)) {
            int iMsgQueueId = pLogRecoder->GetMsgQueueId();
            SendMsgToQueueIdNoWait(iMsgQueueId, iMsgId, pvData, iLength);
        }
        break;
    }
    case BN_MSG_LOG_INTERFACE:{
        BnThread *pLogRecoder = GetLogRecoder(iModuleMark);
        if (NULL != pLogRecoder
                && 0 != (m_mapModuleLogEnable[iModuleMark] & BN_LOGLEVEL_INTERFACE)) {
            int iMsgQueueId = pLogRecoder->GetMsgQueueId();
            SendMsgToQueueIdNoWait(iMsgQueueId, iMsgId, pvData, iLength);
        }
        break;
    }
    case BN_MSG_LOG_ERROR: {
        BnThread *pLogRecoder = GetLogRecoder(iModuleMark);
        if (NULL != pLogRecoder) {
            int iMsgQueueId = pLogRecoder->GetMsgQueueId();
            SendMsgToQueueIdNoWait(iMsgQueueId, iMsgId, pvData, iLength);
        }
        break;
    }
//    case BN_MSG_TIMER_0: {
//        struct timeval tTimeOfDay;
//        gettimeofday(&tTimeOfDay, NULL);
//        struct tm* pTime;
//        pTime = localtime(&tTimeOfDay.tv_sec);
//
//        char arrcTime[24] = {0};
//        snprintf(arrcTime, sizeof(arrcTime), "%04d-%02d-%02d %02d:%02d:%02d-%03d",
//                pTime->tm_year+1900,  pTime->tm_mon+1, pTime->tm_mday,
//                pTime->tm_hour, pTime->tm_min, pTime->tm_sec,
//                (int)tTimeOfDay.tv_usec/1000);
//        std::string strTime = arrcTime;
//        std::cout<< strTime << std::endl;
//        break;
//    }
    default: {
        BnThread *pLogRecoder = GetLogRecoder(iModuleMark);
        if (NULL != pLogRecoder) {
            int iMsgQueueId = pLogRecoder->GetMsgQueueId();
            SendMsgToQueueIdNoWait(iMsgQueueId, iMsgId, pvData, iLength);
        }
        break;
    }
    }

}

ERRCODE BnLogDispatchThread::RegisterTimer(){
//    return CreateTimer(BN_MSG_TIMER_0, 3001);
    return SUCCESS;
}

void BnLogDispatchThread::CreateLogRecoder(int iModuleMark,
        int iMsgQueueKey,
        int iLength,
        void *pvData){
    BnLogRecoderThread *pLogThread = new BnLogRecoderThread((char *)pvData, iMsgQueueKey);//add a new thread for the corresponding module
    if(NULL == pLogThread){
        return;
    }

    // May be change another way to get the property xml
    //pLogThread->InitialLogVar("/home/bn/gitHouse/COMMON/bnlogTask/bnlog.properties.xml");
    pLogThread->InitialLogVar("./bnlog.properties.xml");
    m_mapModuleThread[iModuleMark] = pLogThread;
    m_mapModuleLogEnable[iModuleMark] = BN_LOGLEVEL_DEBUG
                                        | BN_LOGLEVEL_INTERFACE
                                        | BN_LOGLEVEL_ERROR
                                        | BN_LOGLEVEL_FATAL;

    //SendMsgToQueueId(pLogThread->GetMsgQueueId(), BN_MSG_CHANGE_NAME, pvData, iLength);
}

BnThread *BnLogDispatchThread::GetLogRecoder(int iModuleMark){
    std::map<int, BnThread *>::iterator iterModuleThread = m_mapModuleThread.find(iModuleMark);
    BnThread *pLogThread = NULL;
    if(m_mapModuleThread.end() != iterModuleThread ){
        pLogThread = (BnThread *)iterModuleThread->second;
        if(NULL == pLogThread){
            m_mapModuleThread.erase(iterModuleThread);
        }
    }

    return pLogThread;
}

