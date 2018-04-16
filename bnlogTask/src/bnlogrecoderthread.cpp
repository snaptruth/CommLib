/*
 * bnlogthread.cpp
 *
 *  Created on: 2017年8月22日
 */

#include <fstream>
#include <stdio.h>
#include <iostream>
#include "bnconst.h"
#include "bnosutils.h"
#include "bnlogif.h"
#include "bnlogrecoderthread.h"
#include <sstream>

// For file operation
#ifdef WIN32
#include <io.h>
#include <direct.h>
#else
#include <unistd.h>
#include <sys/stat.h>
#endif

#ifdef WIN32
#define ACCESS(fileName,accessMode) _access(fileName,accessMode)
#define MKDIR(path) _mkdir(path)
#else
#define ACCESS(fileName,accessMode) access(fileName,accessMode)
#define MKDIR(path) mkdir(path,S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)
#endif
// End for file operation



namespace std
{
    template < typename T > std::string to_string( const T& n )
    {
        std::ostringstream stm ;
        stm << n ;
        return stm.str() ;
    }
} 

void createDirectory(const std::string &directoryPath){
    int dirPathLen = directoryPath.length();

    char tmpDirPath[256] = { 0 };
    for (int i = 0; i < dirPathLen; ++i){
        tmpDirPath[i] = directoryPath[i];
        if (tmpDirPath[i] == '\\' || tmpDirPath[i] == '/'){
            if (ACCESS(tmpDirPath, 0) != 0){
                int iRetValue = MKDIR(tmpDirPath);
                if(0 != iRetValue){
                    std::cout<< "mkdir " << tmpDirPath << " return " << iRetValue \
                            <<", Make sure you got Authorization "<< std::endl;
                }
            }
        }
    }
}

int BnLogRecoderThread::m_iLastLogSecond = 0;
std::string BnLogRecoderThread::m_strLastLogSecond = "";
int BnLogRecoderThread::m_iLastLogMicroSecond = 0;
std::string BnLogRecoderThread::m_strLastLogMicroSecond = "";

BnLogRecoderThread::BnLogRecoderThread(const char *pcThreadName, int iMsgQueueKey, int iPriority)
    : BnThread(pcThreadName, iMsgQueueKey, iPriority){
}

BnLogRecoderThread::~BnLogRecoderThread() {
}

void BnLogRecoderThread::OnMessage(int iMsgId, int iLength, void *pvData){
    if(!m_tLogParams.m_bInitialed){
        return;
    }

    switch(iMsgId){//recorder the log according to the iMsgId
    case BN_MSG_LOG_ENABLE: {
        if(4 == iLength && NULL != pvData){
            int iLogEnable = ((int *)pvData)[0];
            m_tLogParams.m_iLogLevel = iLogEnable;
        }
        break;
    }
    case BN_MSG_LOG_DEBUG: {
        std::string strDebugInfo = std::string((char *)pvData);
        BnLogDebug(strDebugInfo);
        break;
    }
    case BN_MSG_LOG_INTERFACE: {
        std::string strIfaceInfo = std::string((char *)pvData);
        BnLogInterface(strIfaceInfo);
        break;
    }
    case BN_MSG_LOG_ERROR: {
        std::string strErrorInfo = std::string((char *)pvData);
        BnLogError(strErrorInfo);
        break;
    }
    case BN_MSG_LOG_CLEAR: {
        ClearLogFile();
        break;
    }
    default:
        break;
    }
}

void BnLogRecoderThread::InitialLogVar(const char *pcParamFiles){

    m_tLogParams.InitialParams(pcParamFiles);

    std::fstream fLogDirection;
    m_strIndexFile = getLogDirection() + "index";
    m_strLogFile = getLogDirection() + m_tLogParams.m_strLogFilePrefix + ".log";

    createDirectory(getLogDirection());

    m_iLogFileBackNo = 0;
    std::ifstream ifIndexFile;
    ifIndexFile.open(m_strIndexFile);
    if(ifIndexFile.is_open()){
        ifIndexFile >> m_iLogFileBackNo;
        ifIndexFile.close();
        m_iLogFileBackNo ++;    // Next NO will plus one
    }

    std::ifstream ifCurLogFile;
    ifCurLogFile.open(m_strLogFile);
    if(ifCurLogFile.is_open()){
        // If last log file exist, then we should rename to backup file
        ifCurLogFile.close();
        BackupCurLogFile();
    }

    // Open or reopen current log file
    m_ofLogFile.open(m_strLogFile);
}

void BnLogRecoderThread::BackupCurLogFile(){
    std::string strCurBackFile = getLogDirection() +
            m_tLogParams.m_strLogFilePrefix + "_" +
            std::to_string(m_iLogFileBackNo) + ".log";
    rename(m_strLogFile.c_str(), strCurBackFile.c_str());

    std::ofstream ofIndexFile;
    ofIndexFile.open(m_strIndexFile);
    ofIndexFile << m_iLogFileBackNo << std::endl;
    ofIndexFile.close();
    m_iLogFileBackNo++;
    if(m_iLogFileBackNo >= m_tLogParams.m_iMaxFileBackup){
        // Then we write back
        m_iLogFileBackNo = 0;
    }

    m_ofLogFile.open(m_strLogFile);
    m_ofLogFile << std::endl;
    m_ofLogFile.close();
    m_iLogFileSize = 0;
}

void BnLogRecoderThread::ClearLogFile(){
    m_ofLogFile.close();
    m_ofLogFile.open(m_strLogFile);
    m_ofLogFile << std::endl;
    m_iLogFileSize = 0;
    m_ofLogFile.close();
    m_iLogFileBackNo = 0;

    std::string strRmCommand = "rm -rf " + getLogDirection() + "/*";
    ExecuteCmd(strRmCommand.c_str());
}

void BnLogRecoderThread::BnLogDebug(const std::string &strInfo){
    if(m_tLogParams.m_iLogLevel & BN_LOGLEVEL_DEBUG){
        BnLogInfo("DEBUG", strInfo);
    }

    // Print to console according to user's chooses
    if(m_tLogParams.m_bPrintToConsole){
        std::cout<< "[DEBUG] " << strInfo << std::endl;
    }
}

void BnLogRecoderThread::BnLogInterface(const std::string &strInfo){
    if(m_tLogParams.m_iLogLevel & BN_LOGLEVEL_INTERFACE){
        BnLogInfo("IFACE", strInfo);
    }

    // Print to console according to user's chooses
    if(m_tLogParams.m_bPrintToConsole){
        std::cout<< "[IFACE] " << strInfo << std::endl;
    }
}

void BnLogRecoderThread::BnLogError(const std::string &strInfo){
    if(m_tLogParams.m_iLogLevel & BN_LOGLEVEL_ERROR){
        BnLogInfo("ERROR", strInfo);
    }

    // Print to console according to user's chooses
    if(m_tLogParams.m_bPrintToConsole){
        std::cout<< "[ERROR] " << strInfo << std::endl;
    }
}

void BnLogRecoderThread::BnLogFatal(const std::string &strInfo){
    if(m_tLogParams.m_iLogLevel & BN_LOGLEVEL_FATAL){
        BnLogInfo("FATAL", strInfo);
    }

    // Print to console according to user's chooses
    if(m_tLogParams.m_bPrintToConsole){
        std::cout<< "[FATAL] " << strInfo << std::endl;
    }
}

void BnLogRecoderThread::BnLogInfo(const std::string &strLevel, const std::string &strLogInfo){
    std::string strAllInfo = "[" + strLevel + "] " + strLogInfo;
    int iLength = strAllInfo.length();

    // Write to log file
    if(0 >= iLength){
        return;
    }
    try{
        m_iLogFileSize += iLength;
        if(!m_ofLogFile.is_open()){
            // Something may not happen
            m_ofLogFile.open(m_strLogFile);
        }
        m_ofLogFile << strAllInfo << std::endl;
        if(m_iLogFileSize >= m_tLogParams.m_iMaxFileSize){
            BackupCurLogFile();
            // After current log file backuped, we should reopen current log file
            m_ofLogFile.open(m_strLogFile);
        }
    }catch(...){
    }

    strAllInfo.clear();

    // need c++11 for doing this
    strAllInfo.shrink_to_fit();
}

