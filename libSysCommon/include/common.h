#ifndef _COMMON_H_
#define _COMMON_H_

/*屏蔽未用变量告警*/
#define UNWARING(x) (x=x)

/******************TYPE DEFINE*************************/

typedef int                 BOOL;
typedef signed char         INT8;
typedef unsigned char       UINT8;
typedef unsigned char       UCHAR;
typedef char                CHAR;
typedef short               INT16;
typedef short               SHORT;
typedef unsigned short      UINT16;
typedef unsigned short      USHORT;
typedef int                 INT32;
typedef unsigned int        UINT32;
typedef long                LINT32;
typedef unsigned long       ULINT32;
typedef long long           INT64;
typedef unsigned long long  UINT64;
typedef void *              PVOID;
typedef unsigned char *     PUCHAR;
typedef char *              PCHAR;
typedef short *             PSHORT;
typedef unsigned short*     PUSHORT;
typedef int   *             PINT32;
typedef unsigned int  *     PUINT32;
typedef long int      *     PLINT32;
typedef unsigned long *     PULINT32;
typedef const char *        CPCHAR;

#define DRV_DEV_FILE                        "/dev/fpga_dev" //driver file for app, need clarify later
#define SYSTEM_MONITOR_RCV_MSG_QUEUE_KEY    0X5AA5

/******************MACRO DEFINE*************************/

typedef enum _MSG_SRC_
{
    MSG_SRC_NONE    = 0,
    MSG_SRC_BSDRSVR  = 1,//20171020
	MSG_SRC_BNLOG = 2,//20171108
    MSG_SRC_MONITOR = 3,
    MSG_SRC_MAX

} MSG_SRC;


typedef enum _MSG_DST_
{
    MSG_DST_NONE        = 0,
    MSG_DST_VOICE       = 1,
    MSG_DST_MAX

} MSG_DST;

#endif
