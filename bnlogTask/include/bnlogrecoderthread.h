/*
 * bnlogthread.h
 *
 *  Created on: 2017年8月22日
 */

#ifndef BNLOGRECODERTHREAD_H_
#define BNLOGRECODERTHREAD_H_

#include <string.h>
#include <fstream>
#include "bnlogparams.h"
#include "bnthread.h"

void createDirectory(const std::string &directoryPath);
void BnLogInfo(const std::string &strLevel, const std::string &strLogInfo);

class BnLogRecoderThread: public BnThread {
public:
    BnLogRecoderThread(const char *pcThreadName, int iMsgQueueKey, int iPriority = BN_THREAD_PRIORITY_10);
    virtual ~BnLogRecoderThread();

    virtual void OnMessage(int iMsgId, int iLength, void *pvData);

    void InitialLogVar(const char *pcParamFiles);

    std::string getLogDirection() {return m_tLogParams.m_strLogFilePath
            + GetThreadName()
            + PATHINTERVAL;};

    void BnLogDebug(const std::string &strInfo);
    void BnLogInterface(const std::string &strInfo);
    void BnLogError(const std::string &strInfo);
    void BnLogFatal(const std::string &strInfo);
    void BnLogInfo(const std::string &strLevel, const std::string &strLogInfo);
    void ClearLogFile();

private:
    void BackupCurLogFile();

private:
    int m_iLogFileBackNo;
    std::string m_strIndexFile;
    std::string m_strLogFile;
    std::ofstream m_ofLogFile;
    int m_iLogFileSize;

    BnLogParams m_tLogParams;

    static int m_iLastLogSecond;
    static std::string m_strLastLogSecond;
    static int m_iLastLogMicroSecond;
    static std::string m_strLastLogMicroSecond;
};

#endif /* BNLOGRECODERTHREAD_H_ */
