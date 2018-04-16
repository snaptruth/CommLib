/*
 * bnlogparams.h
 *
 *  Created on: 2017年8月22日
 */
#ifndef BNLOGPARAMS_H_
#define BNLOGPARAMS_H_

#include <string>

// 检查指针值是否为空，为空则返回，DEBUG版本主动挂起
#define BN_LOG_CHECK_POINTER_VOID(ptr) {\
                if(NULL == ptr){\
                    std::cout<<__FILE__ << __FUNCTION__ << __LINE__ << "Pointer is NULL" << std::endl;\
                    assert(true);\
                    return;\
                }\
            }

class BnLogParams {
public:
    BnLogParams();
    virtual ~BnLogParams();

    void InitialParams(const char *pcParamFiles);

    bool isPrintToConsole(){return m_bPrintToConsole;}

private:
    void SetLogLevel(std::string strLogLevel);

public:
    std::string m_strLogFilePath;
    std::string m_strLogFilePrefix;
    int m_iMaxFileSize;
    int m_iMaxFileBackup;
    int m_iLogLevel;
    bool m_bPrintToConsole;
    bool m_bInitialed;
};

#endif /* BNLOGPARAMS_H_ */
