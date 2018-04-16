/*
 * bnsyscommand.cpp
 *
 *  Created on: 2017年10月19日
 */
#include <sys/wait.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include "bnosutils.h"

#ifndef WIN32
bool ExecuteCmd(const char * pcCommand){
    if(NULL == pcCommand) {
        return false;
    }

    int iResult = system(pcCommand);
    if(iResult < 0) {
        printf("cmd: %s execute error: %s", pcCommand, strerror(errno));
        return false;
    }

    if (WIFEXITED(iResult)){
        if (0 == WEXITSTATUS(iResult)){
            return true;
        }
        else{
            printf("cmd: %s execute fail, exit code: %d\n", pcCommand, WEXITSTATUS(iResult));
        }
    }
    else{
        printf("cmd: %s exit status = [%d]\n", pcCommand, WEXITSTATUS(iResult));
    }
    return false;
}
#endif


