#ifndef ERRCODE_H
#define ERRCODE_H

#ifdef  __cplusplus
extern "C" {
#endif


/*----------------------------------------------*
 * 系统返回值定义   范围(0~-10000)              *
 *----------------------------------------------*/

#define SYS_RC_OK                   (0)     /*成功*/
#define SYS_RC_INV_PARAM            (-1)    /*无效参数*/
#define SYS_RC_ERR                  (-2)    /*错误*/
#define SYS_RC_CREAT_FAILED         (-3)    /*创建失败*/
#define SYS_RC_OPEN_FAILED          (-4)    /*打开失败*/
#define SYS_RC_INIT_FAILED          (-5)    /*初始化失败*/
#define SYS_RC_FIND_ERR             (-6)    /*查找失败*/
#define SYS_RC_GET_ERR              (-7)    /*获取失败*/
#define SYS_RC_INSERT_ERR           (-8)    /*插入失败*/
#define SYS_RC_UPDATE_ERR           (-9)    /*更新失败*/
#define SYS_RC_DELETE_ERR           (-10)   /*删除失败*/
#define SYS_RC_REGISTE_ERR          (-11)   /*注册失败*/
#define SYS_RC_MALLOC_ERR           (-12)   /*malloc失败*/
#define SYS_RC_READ_ERR             (-13)   /*读失败*/
#define SYS_RC_WRITE_ERR            (-14)   /*写失败*/
#define SYS_RC_FILE_NOT_EXIST       (-15)   /*文件不存在*/
#define SYS_RC_FILE_STAT_ERR        (-16)   /*文件stat错误*/
#define SYS_RC_FILE_RM_ERR          (-17)   /*删除文件错误*/
#define SYS_RC_FILE_TYPE_NOT_MATCH  (-18)   /*文件类型不匹配*/
#define SYS_RC_RPC_CLNT_CREATE_ERR  (-19)   /*RPC client创建失败*/
#define SYS_RC_RPC_CLNT_DESTROY_ERR (-20)   /*RPC client销毁失败*/
#define SYS_RC_CURL_EASY_INIT_ERR   (-21)   /*curl esay init 失败*/
#define SYS_RC_MD5_CHKSUM_ERR       (-22)   /*MD5校验失败*/
#define SYS_RC_LOCK_TEST_ERR        (-23)   /*锁测试失败*/
#define SYS_RC_LOCK_ERR             (-24)   /*锁lock失败*/
#define SYS_RC_PKG_INVALID           (-25)   /*包类型不匹配 */

/*----------------------------------------------*
 * 自定义返回值定义   范围(-10001~-20000)       *
 *----------------------------------------------*/


/*----------------------------------------------*
 * 系统返回值定义   范围(0~-10000)              *
 *----------------------------------------------*/

#define SYS_RC_MSG_OK                   "No error"
#define SYS_RC_MSG_INV_PARAM            "Invaid param"
#define SYS_RC_MSG_ERR                  "General error"
#define SYS_RC_MSG_CREAT_FAILED         "Create file error"
#define SYS_RC_MSG_OPEN_FAILED          "Open file error"
#define SYS_RC_MSG_INIT_FAILED          "Init error"
#define SYS_RC_MSG_FIND_ERR             "Find error"
#define SYS_RC_MSG_GET_ERR              "Get error"
#define SYS_RC_MSG_INSERT_ERR           "Insert error"
#define SYS_RC_MSG_UPDATE_ERR           "Update error"
#define SYS_RC_MSG_DELETE_ERR           "Delete error"
#define SYS_RC_MSG_REGISTE_ERR          "Registe error"
#define SYS_RC_MSG_MALLOC_ERR           "Malloc error"
#define SYS_RC_MSG_READ_ERR             "File read error"
#define SYS_RC_MSG_WRITE_ERR            "File write error"
#define SYS_RC_MSG_FILE_NOT_EXIST       "File not exist"
#define SYS_RC_MSG_FILE_STAT_ERR        "File stat error"
#define SYS_RC_MSG_FILE_RM_ERR          "File rm error"
#define SYS_RC_MSG_FILE_TYPE_NOT_MATCH  "File type not match error"
#define SYS_RC_MSG_RPC_CLNT_CREATE_ERR  "RPC client create error"
#define SYS_RC_MSG_RPC_CLNT_DESTROY_ERR "RPC client destroy error"
#define SYS_RC_MSG_CURL_EASY_INIT_ERR   "CURL easy init error"
#define SYS_RC_MSG_MD5_CHKSUM_ERR       "Md5 checksum error"
#define SYS_RC_MSG_LOCK_TEST_ERR        "lock test error"
#define SYS_RC_MSG_LOCK_ERR             "lock error"
#define SYS_RC_MSG_PKG_INVALID          "package invalid"


/*----------------------------------------------*
 * 自定义返回值定义   范围(-10001~-20000)       *
 *----------------------------------------------*/
typedef struct
{
    int errCode;
    char* errMsg;
} sErrToMsg;

extern char* RC_GetResultMsg_ByCode(int nResultCode);

#ifdef __cplusplus
}
#endif

#endif /* __RESULTCODE_H__ */

