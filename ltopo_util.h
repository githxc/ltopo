#ifndef _LTOPO_UTIL_H
#define _LTOPO_UTIL_H

#include "ltopo_define.h"

typedef struct struct_LTOPO_ARCHIVE
{
    int id;
    int type;
    char addr[LTOPO_ADDR_LEN];
}LTOPO_ARCHIVE;

int ltopo_init(int acount, LTOPO_ARCHIVE * a, void (*callback)(char * path, int result));
//int ltopo_init(void (*callback)(char * path, int result));
int ltopo_config(int cycle, int vh, int vl, int wsize, int flag);


int ltopo_job_set_path(char * path);
//#define LTOPO_ALG_ACTION_CALCUL 0
//#define LTOPO_ALG_ACTION_STATIS 1
int ltopo_job_start(int action);
//如果返回1，则表示已经退出，如果返回0，表示正在退出
int ltopo_alg_quit();
int ltopo_statistics();
void ltopo_print_all_meters();

#endif
