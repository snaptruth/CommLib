#define __OPENWRT__

#ifdef __X86__
#include <sys/io.h>
#endif

#include <fcntl.h>
#include<linux/watchdog.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "monitor.h"
#include "taskLib.h"
//#include "errCode.h"


/**************************************************************************
** 函数名:enDog
** 输  入:
** 输  出:
** 描  述: 使能硬件看门狗
** 作  者: xiaodong
** 日  期:
** 修  改:
** 备  注:
**************************************************************************/
#ifdef __X86__
void enDog()
{
    if( iopl(3) < 0 )//为3时可以读写端口
    {
      perror("iopl access error/n");
      exit(1);
    }

    outb(0x87, 0x4E );
    outb(0x87, 0x4E ); //Enter configuration mode

    outb(0x07, 0x4E ); //LDN select index
    outb(0x07, 0x4F ); //select LDN to 0x07(Select WDT device configuration registers.)

    outb(0xF0, 0x4E ); //Watchdog output enable index
    outb(0x80, 0x4F ); //enable Watchdog time out output via WDTRST#.

    outb(0xF6, 0x4E ); //Timer Configuration index
    outb(0x05, 0x4F ); //5 Unit 设置看门狗周期为5秒

    outb(0xF5, 0x4E ); //Control Configuration index
    outb(0x32, 0x4F ); //Unit = 1sec, Pulse mode, Pulse Width = 125ms, Enable!

    outb(0xAA, 0x4E ); //Exit configuration mode
}
#endif

#ifdef __NO_OPENWRT__
int wdtFd = 0;
void enDog()
{
    int wdtFd = open("/dev/watchdog", O_WRONLY);
    if (wdtFd == -1)
    {
       SYSMONITOR_DEBUG_INFO("open /dev/watchdog fail! %s!\n", strerror(errno));
       exit(EXIT_FAILURE);
    }
    else
    {
        SYSMONITOR_DEBUG_INFO("open /dev/watchdog good\n");
    }

    int timeout = 0;
    ioctl(wdtFd, WDIOC_GETTIMEOUT, &timeout);
    SYSMONITOR_DEBUG_INFO("Default timeout was is %d seconds\n", timeout);

    timeout=5;
    ioctl(wdtFd, WDIOC_SETTIMEOUT, 5); //设置看门狗周期为5秒

    timeout=0;
    ioctl(wdtFd, WDIOC_GETTIMEOUT, &timeout);
    SYSMONITOR_DEBUG_INFO("Set timeout was is %d seconds\n", timeout);

}
#endif


/**************************************************************************
** 函数名:feedDog
** 输  入:
** 输  出:
** 描  述: 去使能硬件看门狗
** 作  者: xiaodong
** 日  期:
** 修  改:
** 备  注:
**************************************************************************/
#ifdef __X86__
void disDog()
{
    outb(0x87, 0x4E );
    outb(0x87, 0x4E ); //Enter configuration mode

    outb(0x07, 0x4E ); //LDN select index
    outb(0x07, 0x4F ); //select LDN to 0x07(Select WDT device configuration registers.)

    outb(0xF0, 0x4E ); //Watchdog output enable index
    outb(0x00, 0x4F ); //disable Watchdog time out output via WDTRST#.

    outb(0xF5, 0x4E ); //Timer Configuration index
    outb(0x52, 0x4F ); //clear WDTMOUT_STS,Unit = 1sec, Pulse mode, Pulse Width = 125ms, Disable!

    outb(0xAA, 0x4E ); //Control Configuration index

}
#endif

#ifdef __NO_OPENWRT__
void disDog()
{
    //mt7621 wdt driver said : Watchdog cannot be stopped once started
    if( wdtFd != 0 )
    {
        close(wdtFd);
        wdtFd = 0;
    }
}
#endif

/**************************************************************************
** 函数名:feedDog
** 输  入:
** 输  出:
** 描  述: 喂硬件看门狗
** 作  者: xiaodong
** 日  期:
** 修  改:
** 备  注:
**************************************************************************/
#ifdef __X86__
void feedDog()
{
    outb(0x87, 0x4E );
    outb(0x87, 0x4E ); //Enter configuration mode
    outb(0xF6, 0x4E ); //Control Configuration index
    outb(0x05, 0x4F ); //Unit = 1sec, Pulse mode, Pulse Width = 125ms, Enable!
    outb(0xAA, 0x4E ); //Control Configuration index
}
#endif

#ifdef __NO_OPENWRT__
void feedDog()
{
    if( wdtFd != 0 )
    {
        ioctl(wdtFd, WDIOC_KEEPALIVE, 0);
    }
}
#endif



///*start modified by Haijing 20171108*/
//
//volatile bool shouldrun = true;
//
///*end modified by Haijing 20171108*/

int main()
{
/*added by haijing20171027start*/

	int iMsgId = BN_LOG_MODULE_MONITOR;
	if(InitBnLog("MONITOR", iMsgId) != SUCCESS)
	{
		BN_LOG_ERROR("%s: taskLibInit error: %s.\n",  __func__, strerror(errno));//modified by haijing20171110
		return -1;
	}

/*added by haijing20171027end*/



//由于openwrt 的看门狗被ubus接管，所以这里就不需要编写喂狗程序
//ubus call system watchdog 查看看门狗状态
#ifndef __OPENWRT__
    //注册看门狗
    registerWatchDog(feedDog);

    //使能硬件看门狗
    enDog();
#endif

    //注册200毫秒定时器用于喂狗定时器任务，如果失败，返回错误，定时产生SIGALRM信号
    if(systemMonitorInit() != 0)
    {
    	BN_LOG_ERROR("%s: systemMonitorInit: %s.\n", __func__, strerror(errno));//modified by haijing20171110
        return -1;
    }


    //初始化线程记录表
    if(taskLibInit(1) == TASK_RT_ERROR)
    {
    	BN_LOG_ERROR("%s: taskLibInit error: %s.\n", __func__, strerror(errno));//modified by haijing20171027
        return errno;
    }

    if(taskSpawn("monitorRcvMessageThread", TASK_DEFAULT_PRIORITY, TASK_DEFAULT_OPTION, TASK_DEFAULT_STACK_SIZE, 1, monitor_rcv_queue_msg_sevice, NULL) < 0)
    {//回调函数，函数内部new一个线程去执行monitor_rcv_queue_msg_sevice函数commented by haijing
    	BN_LOG_ERROR("%s: taskSpawn err: %s.\n", __func__, strerror(errno));//modified by haijing20171027
        return -1;
    }



    char cmd[512] = {0};
    sprintf(cmd, "logger monitor: Pid %d Start Running ...... \n", getpid());
    system(cmd);

//    UCHAR msgSrc = MSG_SRC_MONITOR;//by haijing 20171108
//    while(shouldrun)
//    {
//    	taskWatch(&msgSrc);//by haijing 20171108
//        taskDelay(100 * 1000);
//    }

    while(1)
   {
	   taskDelay(100 * 1000);
   }

    return 0;
}
