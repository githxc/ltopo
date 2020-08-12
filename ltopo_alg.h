#ifndef LTOPO_ALG_H
#define LTOPO_ALG_H

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "ltopo_define.h"
#include "ltopo_list.h"

#define LTOPO_LOAD_TYPE_CURRENT 0x1
#define LTOPO_LOAD_TYPE_VOLTAGE 0x2
#define LTOPO_LOAD_TYPE_APOWER 0x4
#define LTOPO_LOAD_TYPE_RPOWER 0x8
#define LTOPO_LOAD_TYPE_PFACTOR 0x10

#define LTOPO_MAX_POINT_IN_ONE_RECORD 10

#define LTOPO_ALG_ACTION_CALCUL 0
#define LTOPO_ALG_ACTION_STATIS 1

#define LTOPO_RESULT_BY_UNKNOWN 0
#define LTOPO_RESULT_BY_CAL 1
#define LTOPO_RESULT_BY_STA 2

#define LTOPO_DATA_VALUE_MIDDLE -1
#define LTOPO_DATA_VALUE_MISSING -2

#define LTOPO_EV_VALIDATION_OK 0
#define LTOPO_EV_VALIDATION_ERR_IGNORE -1
#define LTOPO_EV_VALIDATION_ERR_CONFLICTS -2


typedef enum
{
    LTOPO_TYPE_PHASE_T=0,
    LTOPO_TYPE_PHASE_A,
    LTOPO_TYPE_PHASE_B,
    LTOPO_TYPE_PHASE_C,
    LTOPO_TYPE_PHASE_MAX
} LTOPO_TYPE_PHASE;

#define LTOPO_EVENT_TYPE_VOID 0
#define LTOPO_EVENT_TYPE_SMOOTH 1 // type, start, vcount, load[0] load[1] ...
#define LTOPO_EVENT_TYPE_JUMP   2   // type, start, vcount, load[0]...load[vcount]
#define LTOPO_EVENT_TYPE_MIDDLE 3
#define LTOPO_EVENT_NO_DATA LTOPO_EVENT_TYPE_VOID

typedef struct struct_LTOPO_JTIME
{
	LTOPO_LIST addr;
	int jump_time; 
}LTOPO_JTIME;


typedef struct struct_LTOPO_LOAD
{
    int i;      //current
    int v;      //voltage
    int ap;     //active power
    int rp;     //reactive power
    int pf;     //power factor
} LTOPO_LOAD;
typedef struct struct_LTOPO_EVENT
{
    int type;
    int start;
    int vcount;  //除了start以外的load的个数，即load的总个数是vcount+1
    LTOPO_LOAD load[LTOPO_MAX_POINT_IN_ONE_RECORD];
}LTOPO_EVENT;
typedef struct struct_METER_STATIS
{
    int middle_count;
    int missing_count;
}METER_STATIS;
typedef struct struct_METER_INFO
{
    char addr[LTOPO_ADDR_LEN]; //mbox/branch addr, 对branch不同分支有不同id, 000001-0 000001分支单元的0号分支
    char event_file[FILENAME_LEN]; //数据文件名，ba-000001-0， ma-000002
#if 0 //def M1    
    LTOPO_EVENT * event; //数据链表
    LTOPO_EVENT cur;      //扫描过程中，生成的事件
    LTOPO_EVENT * ptr;    //记录扫描到event的指针，用于快速遍历
#endif //M1
    int excluded;        //计算时是否排除在外
    LTOPO_LOAD * load;
	LTOPO_LIST * head;
    struct struct_LTOPO_MBOX_NODE * pm;  //记录g_ltopo.meters中的指针
    int owstatus;           //outer window status
    int iwstatus;           //inner window status
    int next_needed;          //是否需要扫描下一根扫描线，以替代本扫描线
    METER_STATIS s;
} METER_INFO;
    
typedef struct struct_LTOPO_ALG
{
    int mcycle;                         //计量周期，一般为1秒, meter cycle
    int start;
    int end;
    int vh;
    int vl;
    int wsize;
    int flag;
    int action;
    char event_path[FILENAME_LEN];      //待处理的抄读数据路径， 如1583975621-1583979221
    int bcount[LTOPO_TYPE_PHASE_MAX];   //每个相位的branch个数，每个分支算一个branch
    int mcount[LTOPO_TYPE_PHASE_MAX];   //每个相位的meter box个数
    METER_INFO * branch[LTOPO_TYPE_PHASE_MAX];  //每个相位的branch数据
    METER_INFO * mbox[LTOPO_TYPE_PHASE_MAX];    //每个相位的mbox数据
} LTOPO_ALG;

extern LTOPO_ALG g_ltopoalg;

#define LTOPO_DIRECTION_UP 1
#define LTOPO_DIRECTION_DOWN -1
typedef struct struct_LTOPO_JUMP_EV_INFO 
{
    int phase;
    int type;
    int index;
    int start;
    int direction;
    int value;
    int period;
    int minv;
    int maxv;
    char addr[LTOPO_ADDR_LEN];
    char father[LTOPO_ADDR_LEN];
}LTOPO_JUMP_EV_INFO;
typedef struct struct_LTOPO_JUMP_EV_NODE
{
    LTOPO_JUMP_EV_INFO ev;
    LTOPO_LIST list;
}LTOPO_JUMP_EV_NODE;
/*
#define BEYOND_CONTINUE(x, a, b)  \
                if(x<a || x>=b) { \
                        printf(LTOPO_ERR_TAG "%s, warning: index %d\n", __FUNCTION__, x);\
                        continue; \
                        }
*/
#if 1
#define BEYOND_CONTINUE(offset, i, a, b)  \
                                if((offset+i)<a || (offset+i)>=b) { \
                                        printf(LTOPO_ERR_TAG "%s, warning: index %d [%d %d], offset %d, i %d, \n", __FUNCTION__, (offset+i), a, b-1, offset, i);\
                                        continue; \
                                        }
#else
#define BEYOND_CONTINUE(offset, i, a, b)  \
                                if((offset+i)<a || (offset+i)>=b) { \
                                        continue; \
                                        }
#endif
extern char * ltopo_bp_prefix[];
extern char * ltopo_mp_prefix[];

#if 0 //def M1
int ltopo_print_event(LTOPO_EVENT *head);
int ltopo_print_event_list(FILE * fp, LTOPO_EVENT *head);
LTOPO_EVENT * build_event(char * filename);
int ltopo_read_dir(int phase);
int print_ltopo_info();
int write_ltopo_info();
int is_sline_in_the_future(int sline, LTOPO_EVENT* e);
int minvalue_index_in_event(LTOPO_EVENT *e);
int maxvalue_index_in_event(LTOPO_EVENT *e);
int generate_jump_event(LTOPO_EVENT * ori, LTOPO_EVENT * gen, int vh, int vl, int wsize);
int ltopo_scan_meter_event(METER_INFO * meter, int sline, int vh, int vl, int wsize);
int ltopo_scan(int phase, int wsize);
#endif
int ltopo_alg_scan2(int phase);
void ltopo_alg_config(int cycle, int vh, int vl, int wsize, int flag);
int ltopo_alg_init();
int ltopo_alg_set_path(char * path);
int ltopo_alg_clean(int phase);

int verify_mcount_bcount(int mcount, int bcount); //待实现
int ltopo_alg_read_path(int phase);
int ltopo_alg_debug_save_load(int phase);

int ltopo_alg_scan(int phase, int action,FILE *fpo);
int ltopo_alg_statistics(char * path);
int ltopo_alg_proc_s1_jump(int phase, int sline, int mbindex, int vl, char * f_addr, int action);
float ltopo_alg_matching(LTOPO_LIST *m_head,LTOPO_LIST *b_head);
#endif
