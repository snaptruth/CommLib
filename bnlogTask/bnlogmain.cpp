/*
 * bnlogmain.cpp
 *
 *  Created on: 2017年9月27日
 */
#include <iostream>
#include "bnosutils.h"
#include "bnlogdispatchthread.h"
#include "bnlogif.h"


/*begin modified by Haijing 20171108*/

#include "taskLib.h"
volatile bool shouldrun = true;

/*end modified by Haijing 20171108*/






int main(int ac, char** av){
    // Modify message queue's max buffer, Need root authorization
    ExecuteCmd("echo 16384000 > /proc/sys/kernel/msgmnb");
    // /proc/sys/fs/mqueue/msg_max

    BnLogDispatchThread *pLogDispatchThread = new BnLogDispatchThread();
    if(NULL == pLogDispatchThread){
        std::cout<<"Dispatch Thread create failed, check you memory"<<std::endl;
        return -1;
    }

//    pLogDispatchThread->join();


    /*start modified by Haijing 20171108*/

    UCHAR msgSrc = MSG_SRC_BNLOG;
    while(shouldrun)
	{
		//sleep(1);
		taskWatch(&msgSrc);
	}

    /*end modified by Haijing 20171108*/

    pLogDispatchThread->join();
}


