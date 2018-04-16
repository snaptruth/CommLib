/*
 * BnLogDispatchThread.h
 *
 *  Created on: 2017年9月19日
 */

#ifndef BNLOGDISPATCHTHREAD_H_
#define BNLOGDISPATCHTHREAD_H_
#include <map>
#include "bnthread.h"


class BnLogDispatchThread: public BnThread {
public:
    BnLogDispatchThread();
    virtual ~BnLogDispatchThread();

    virtual void OnMessage(int iMsgId, int iLength, void *pvData);

    virtual ERRCODE RegisterTimer();

private:
    void CreateLogRecoder(int iModuleMark, int iMsgQueueKey, int iLength, void *pvData);
    BnThread *GetLogRecoder(int iModuleMark);

private:
    std::map<int, BnThread *> m_mapModuleThread;
    std::map<int, int> m_mapModuleLogEnable;

    int m_iLogIndex;

    static int m_siMsgQueueIdNum;
};

#endif /* BNLOGDISPATCHTHREAD_H_ */
