#ifndef LTOPO_H
#define LTOPO_H
#include <semaphore.h> 
#include <pthread.h>

#include "ltopo_define.h"
#include "ltopo_list.h"

typedef enum
{
    LTOPO_SM_STATE_UNKNOWN=0,
    LTOPO_SM_STATE_CONFIRMING,
    LTOPO_SM_STATE_CONFIRMED
} LTOPO_SM_STATUS;

//XML_METER: xml节点信息
//LTOPO_METER_INFO: 所有设备的信息，表箱终端、分支单元、ttu共用
typedef struct struct_XML_METER {
    int mtype; //表类型
    char addr[LTOPO_ADDR_LEN]; //表地址
    char f_addr[LTOPO_ADDR_LEN]; //父地址
    int resultby;
    int id;
    int f_id;
    int level;
    int sm_state;
    int sm_counter;
}XML_METER, LTOPO_METER_INFO;

//LTOPO_METER_NODE: 所有设备的链表节点，用于g_ltopo下
//LTOPO_MBOX_NODE: 表箱终端的链表节点，用于branch节点下
typedef struct struct_LTOPO_MBOX_NODE
{
    LTOPO_METER_INFO minfo;
    LTOPO_LIST list;
} LTOPO_METER_NODE, LTOPO_MBOX_NODE;

#define LTOPO_BRANCH_LEVEL_MAX 10
typedef struct struct_LTOPO_BRANCH_NODE
{
    LTOPO_METER_INFO binfo; //分支信息
    LTOPO_MBOX_NODE mbox; //分支下挂的表箱终端的链表

    struct struct_LTOPO_BRANCH_NODE * father;
    LTOPO_LIST list;
} LTOPO_BRANCH_NODE;

typedef enum
{
    LTOPO_STATUS_NOT_INITED=0,
    LTOPO_STATUS_INITED,
    LTOPO_STATUS_JOB_STARTED,
    LTOPO_STATUS_JOB_FINISHED
} LTOPO_STATUS;

//#define LTOPO_METER_MAX 400
typedef struct struct_LTOPO_INFO
{
    int status; //是否已经初始化
    int running;
#ifdef LTOPO_ENABLE_THREAD    
    sem_t sem;
	pthread_mutex_t mutex;
#endif
    LTOPO_METER_NODE meters;
    int mcount; //meters的个数=mc+bc+ttu
    int mc; //mbox的个数
    int bc; //branch的个数
    int conflicts;
    LTOPO_METER_NODE * root;
    LTOPO_BRANCH_NODE btable[LTOPO_BRANCH_LEVEL_MAX];
	void (*cb_fini)(char * path, int result);
    
} LTOPO_INFO;

extern LTOPO_INFO g_ltopo;


//整条分支，从第一级分支单元到末端的表箱终端
typedef struct struct_BRANCH_LIST
{
    LTOPO_METER_INFO minfo; //信息
    int type;               //类型
    int index;
    struct struct_METER_INFO * p;
    LTOPO_LIST list;
}BRANCH_LIST;
#define THREAD_MODE_NORMAL 			0			
#define THREAD_MODE_REALTIME		1			


int ltopo_set_meter_para(XML_METER * meter, int index, char * key);
int ltopo_add_meter_in_mlist(XML_METER * meter);
LTOPO_METER_NODE * ltopo_get_meter(char * addr, LTOPO_LIST * head);
void meter_fini(void *arg);
void ltopo_print_mlist(LTOPO_LIST * head);
int ltopo_meters_dup(LTOPO_LIST *dhead, LTOPO_LIST *shead);


void branch_list_show(void *arg);
int ltopo_build_whole_branch_by_mboxaddr(int phase, LTOPO_LIST * head, char *addr);
void branch_list_fini(void *arg);
int ltopo_branch_list_set_exclude(LTOPO_LIST * head, int v);


int ltopo_get_shell_result(char * cmd, char * rbuf, int rlen);
int ltopo_print_topo(LTOPO_LIST * head);
int ltopo_refresh_mlist(LTOPO_LIST * head, char * filename);
int ltopo_clean_btable();
void ltopo_clean();



#endif
