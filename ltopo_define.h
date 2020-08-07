#ifndef LTOPO_DEFINE_H
#define LTOPO_DEFINE_H

#ifdef X86_LINUX
//#define LTOPO_ENABLE_THREAD
#else
#define LTOPO_ENABLE_THREAD
#endif

//#define ALG_DEBUG 

#define LTOPO_ADDR_LEN 16
#define FILENAME_LEN 128

#define LTOPO_MAX_METERS 1000

#define LTOPO_TAG "LTOPOALG: "
#define LTOPO_ERR_TAG "LTOPOALG-ERR: "
#define LTOPO_WARN_TAG "LTOPOALG-WARN: "

//缺省计量周期：1秒
#define LTOPO_DEFAULT_MCYCLE 1
//缺省容错尺寸
#define LTOPO_WINDOW_MARGIN_SIZE 2 //容错尺寸， ow=iw+2*LTOPO_WINDOW_MARGIN_SIZE

#define LTOPO_DEFAULT_VH 10000 //1000w
#define LTOPO_DEFAULT_VL 100   //10w
#define LTOPO_DEFAULT_WSIZE 3
#define LTOPO_NODE_SM_COUNTER_MAX 2 //状态变迁对应的计数器阈值


//比较ev时，ev value相差小于LTOPO_EV_COMP_VL_TIMES*Vl，则认为是等幅值
#define LTOPO_EV_COMP_VL_TIMES 5 
//比较ev时，start相差小于LTOPO_EV_COMP_START_DIFF，则认为是等时
#define LTOPO_EV_COMP_START_DIFF 2
//判断branch时，窗口扩大以便容错
//计算ev时，同时扩大
//由于LTOPO_WINDOW_MARGIN_SIZE 定义为2，已有较强容错，本处定义为0
#define LTOPO_BRANCH_WINDOW_ADD 0

//衡量开始跳变的倍数，这个倍数*Vl，表示开始跳变
#define LTOPO_MAYBE_JUMP_COUNT 2 


#ifdef X86_LINUX
#define LTOPO_TEST_PATH "../test/ltopo/"
#define LTOPO_XML_FILE LTOPO_TEST_PATH "data/output/ltopo.xml"
#define LTOPO_XML_STATIS_PATH LTOPO_TEST_PATH "data/output/ev"
#define LTOPO_RESULT_RECORD_PATH LTOPO_TEST_PATH "data/output/rec"
#define LTOPO_RESTORE_LOAD_PATH LTOPO_TEST_PATH "tmp2"
#else
#define LTOPO_DATA_HOME        "/data/ltopo/"
#define LTOPO_XML_FILE LTOPO_DATA_HOME "ltopo.xml" 
#define LTOPO_XML_STATIS_PATH LTOPO_DATA_HOME "alg/ev"
#define LTOPO_RESULT_RECORD_PATH LTOPO_DATA_HOME "alg/rec"
#define LTOPO_RESTORE_LOAD_PATH "/tmp"
#endif

typedef enum
{
    LTOPO_METER_PARA_TYPE=0,
    LTOPO_METER_PARA_ADDR,
    LTOPO_METER_PARA_F_ADDR,
    LTOPO_METER_PARA_RESULTBY,
    LTOPO_METER_PARA_ID,
    LTOPO_METER_PARA_F_ID,
    LTOPO_METER_PARA_LEVEL,
    LTOPO_METER_PARA_STATUS,
    LTOPO_METER_PARA_COUNTER,
    LTOPO_METER_PARA_MAX
}LTOPO_METER_PARA; //与char * ltopo_meter_para[]={"TYPE", "ADDR", "F_ADDR", "ID", "F_ID", "LEVEL"};一起使用

typedef enum
{
    LTOPO_METER_TYPE_TTU=0,     //TTU
    LTOPO_METER_TYPE_BRANCH,
    LTOPO_METER_TYPE_MBOX,       //meter box
    LTOPO_METER_TYPE_METER,
    LTOPO_METER_TYPE_MAX
}METER_TYPE;

#define LTOPO_OK 0
#define LTOPO_ERR_COMMON -1
#define LTOPO_ERR_CONFLICTS -100
#endif
