#include <unistd.h>
#include <dirent.h>
#include <string.h>

#include "ltopo_alg.h"
//#include "ltopo_xml.h"
//#include "ltopo.h"
#include "ltopo_util.h"

#define LTOPO_TEST_INPUT_PATH "../test/meter/data/100-999"
//#define LTOPO_TEST_INPUT_PATH "../test/ltopo/data/input/1586932016-1586932616"

int run_path(char * path)
{
    int ret;
    DIR *dir = NULL;  
    struct dirent *entry;  
    char inner_path[256];

    if((dir = opendir(path))==NULL)   {  
        printf(LTOPO_ERR_TAG "opendir failed!");  
        return -1;  
        }
    while((entry=readdir(dir)))  {
        if(entry->d_type==4){
            if(!strcmp(".",entry->d_name) || !strcmp("..",entry->d_name) )
                continue;
            snprintf(inner_path, sizeof(inner_path)-1, "%s/%s", path, entry->d_name);
            printf(LTOPO_TAG "got path %s\n", inner_path);
            ret=ltopo_job_set_path(inner_path);
            if(ret!=0){
                printf(LTOPO_ERR_TAG "%s, ltopo_job_set_path failed, system abort!\n", __FUNCTION__);
                return -1;
                }
            ret=ltopo_job_start(LTOPO_ALG_ACTION_CALCUL);
            if(ret!=0){
                printf(LTOPO_ERR_TAG "%s, ltopo_job_start failed, system abort!\n", __FUNCTION__);
                return -1;
                }
            }
        }
    closedir(dir);
    dir=NULL;
    return 0;
}
void cb_finish(char * path, int result)
{
    printf(LTOPO_TAG "Enter %s\n", __FUNCTION__);
    printf(LTOPO_TAG "******************************\n");
    printf(LTOPO_TAG "Path: %s, result %d\n", g_ltopoalg.event_path, result);
}
void help()
{
	printf(LTOPO_TAG "help:\n");
	printf(LTOPO_TAG "./ltopo [<path>]\n");
	printf(LTOPO_TAG "./ltopo ../test/meter/data/100-999\n");
}

#ifndef LTOPO_ALG_ACTION_CALCUL
#define LTOPO_ALG_ACTION_CALCUL 0
#define LTOPO_ALG_ACTION_STATIS 1
#endif

inline void THREAD_SLEEP(int s)
{
#ifdef LTOPO_ENABLE_THREAD
    sleep(s);
#endif
}
#if 0
LTOPO_ARCHIVE a[]={{0, 0, "000000000522"}, 
                   {1, 1, "000000000001"},
                   {2, 2, "000000000003"},
//                   {4, 1, "000000000004"},
                   {3, 2, "000000000057"}};
#endif

LTOPO_ARCHIVE a[]={{0, 0, "000000000522"},
                   {1, 2, "000000000021"},
                   {2, 2, "000000000022"},
                   {3, 2, "000000000023"},
                   {4, 2, "000000000025"},
                   {5, 2, "000000000026"},
                   {6, 2, "000000000027"},
                   {7, 2, "000000000028"},
                   {8, 2, "000000000029"},
                   {9, 2, "000000000030"},
                   {10, 1, "000000000031"},
                   {11, 2, "000000000032"},
                   {12, 1, "000000000033"},
                   {13, 2, "000000000034"},
                   {14, 2, "000000000035"},
                   {15, 2, "000000000036"},
                   {16, 1, "000000000037"},
                   {17, 1, "000000000038"},
                   {18, 2, "000000000039"},
                   {19, 2, "000000000053"},
                   {20, 1, "000000000051"},
                   {21, 2, "000000000052"},
                   {22, 1, "000000000054"},
                   {23, 2, "000000000056"},
                   {24, 1, "000000000057"},
                   {25, 2, "000000000073"},
                   {26, 1, "000000000074"},
                   {27, 2, "000000000055"},
                   {28, 1, "000000000061"},
                   {29, 2, "000000000040"},
                   {30, 1, "000000000041"},
                   {31, 2, "000000000060"},
                   {32, 2, "000000000069"},
                   {33, 2, "000000000070"},
                   {34, 1, "000000000058"},
                   {35, 2, "000000000059"},
                   {36, 2, "000000000071"},
                   {37, 1, "000000000063"},
                   {38, 2, "000000000064"},
                   {39, 2, "000000000065"},
                   {40, 2, "000000000066"},
                   {41, 2, "000000000067"},
                   {42, 2, "000000000068"},
                   {43, 1, "000000000072"},
                   {44, 2, "000000000075"},
                   {45, 2, "000000000077"},
                   {46, 2, "000000000078"},
                   {47, 2, "000000000079"},
                   {48, 1, "000000000062"},
                   {49, 2, "000000000076"}};


#define CALCU_PATH
int main(int argc, char ** argv)
{
    int ret=0;
    char path[128];
	if(argc!=1 && argc!=2)
	{
		help();
		return 0;
	}
    if(argc==1)
        strncpy(path, LTOPO_TEST_INPUT_PATH, sizeof(path)-1);
    if(argc==2)
        strncpy(path, argv[1], sizeof(path)-1);
    printf(LTOPO_TAG "path %s\n", path);

#ifdef ARCHIVE_TEST    
    ret=ltopo_init(sizeof(a)/sizeof(LTOPO_ARCHIVE), a, cb_finish);
printf(LTOPO_ERR_TAG "OK\n");
return 0;
#else
    ret=ltopo_init(0, NULL, cb_finish);
#endif
    if(ret!=0){
        printf(LTOPO_ERR_TAG "%s, ltopo_init failed, system abort!\n", __FUNCTION__);
        return -1;
        }
    // 自动生成的数据，对应的参数设置
    //ret=ltopo_config(LTOPO_DEFAULT_MCYCLE, LTOPO_DEFAULT_VH, LTOPO_DEFAULT_VL, LTOPO_DEFAULT_WSIZE, LTOPO_LOAD_TYPE_APOWER);
    // 测试出来的数据，对应的参数设置
    ret=ltopo_config(LTOPO_DEFAULT_MCYCLE, 100, 10, 3, LTOPO_LOAD_TYPE_APOWER);
    if(ret!=0){
        printf(LTOPO_ERR_TAG "%s, ltopo_config failed, system abort!\n", __FUNCTION__);
        return -1;
        }
#if defined ARCHIVE_TEST 
    ret=ltopo_job_set_path("/home/heguang/work/ltopo/test/ltopo/data/input/topology/1589472000-1589475599");
    if(ret!=0){
        printf(LTOPO_ERR_TAG "%s, ltopo_job_set_path failed, system abort!\n", __FUNCTION__);
        return -1;
        }
    sleep(1);
    ret=ltopo_job_start(LTOPO_ALG_ACTION_CALCUL);
    if(ret!=0){
        printf(LTOPO_ERR_TAG "%s, ltopo_job_start failed, system abort!\n", __FUNCTION__);
        return -1;
        }
    sleep(1);
#elif defined CALCU_TEST 
    ret=ltopo_job_set_path("/home/user/work/ltopo/test/ltopo/data/input/highfreq/topology/1589136000-1589139599");
    if(ret!=0){
        printf(LTOPO_ERR_TAG "%s, ltopo_job_set_path failed, system abort!\n", __FUNCTION__);
        return -1;
        }
    sleep(1);
    ret=ltopo_job_start(LTOPO_ALG_ACTION_CALCUL);
    if(ret!=0){
        printf(LTOPO_ERR_TAG "%s, ltopo_job_start failed, system abort!\n", __FUNCTION__);
        return -1;
        }
    sleep(1);
#elif defined CALCU_PATH 
    run_path("/home2/huangxc/git/ltopo/test/tangshan/0712");
#elif defined STATIS_TEST
    ltopo_job_set_path(path);
    sleep(1);
    ltopo_job_start(LTOPO_ALG_ACTION_STATIS);
    sleep(1);
#elif defined STATIS_CALCU
    ltopo_statistics();
#elif defined CONTINUES_CALCU
    ltopo_job_set_path(LTOPO_TEST_PATH "data/input/continues/1587005276-1587005876");
    ltopo_job_start(LTOPO_ALG_ACTION_CALCUL);
    sleep(1);

    ltopo_job_set_path(LTOPO_TEST_PATH "data/input/continues/1587005636-1587006236");
    ltopo_job_start(LTOPO_ALG_ACTION_CALCUL);
    sleep(1);
    
    ltopo_job_set_path(LTOPO_TEST_PATH "data/input/continues/1587005996-1587006596");
    ltopo_job_start(LTOPO_ALG_ACTION_CALCUL);
    sleep(1);

    ltopo_job_set_path(LTOPO_TEST_PATH "data/input/continues/1587006356-1587006956");
    ltopo_job_start(LTOPO_ALG_ACTION_CALCUL);
    sleep(1);
    ltopo_job_set_path(LTOPO_TEST_PATH "data/input/continues/1587006715-1587007315");
    ltopo_job_start(LTOPO_ALG_ACTION_CALCUL);
    sleep(1);
    
#elif defined GROUP_TEST
    ret=ltopo_config(LTOPO_DEFAULT_MCYCLE, LTOPO_DEFAULT_VH, LTOPO_DEFAULT_VL, LTOPO_DEFAULT_WSIZE, LTOPO_LOAD_TYPE_APOWER);
    system("rm -f data/output/ev/*");
    printf(LTOPO_TAG "**************STEP1:calculate 100-999****************\n");
    ltopo_job_set_path("../test/meter/data/100-999");
    ltopo_job_start(LTOPO_ALG_ACTION_CALCUL);
    THREAD_SLEEP(2);

    printf(LTOPO_TAG "**************STEP2.1:statistics 1100-1999****************\n");
    ltopo_job_set_path("../test/meter/data/1100-1999");
    ltopo_job_start(LTOPO_ALG_ACTION_CALCUL);
    THREAD_SLEEP(2);

    printf(LTOPO_TAG "**************STEP2.2:statistics 2100-2999****************\n");
    ltopo_job_set_path("../test/meter/data/2100-2999");
    ltopo_job_start(LTOPO_ALG_ACTION_CALCUL);
    THREAD_SLEEP(2);
    
    printf(LTOPO_TAG "**************STEP2.3:statistics 3100-3999****************\n");
    ltopo_job_set_path("../test/meter/data/3100-3999");
    ltopo_job_start(LTOPO_ALG_ACTION_CALCUL);
    THREAD_SLEEP(2);

    printf(LTOPO_TAG "**************STEP2.4:statistics 4100-4999****************\n");
    ltopo_job_set_path("../test/meter/data/4100-4999");
    ltopo_job_start(LTOPO_ALG_ACTION_CALCUL);
    THREAD_SLEEP(2);
    
    printf(LTOPO_TAG "**************STEP2.5:statistics 5100-5999****************\n");
    ltopo_job_set_path("../test/meter/data/5100-5999");
    ltopo_job_start(LTOPO_ALG_ACTION_CALCUL);
    THREAD_SLEEP(2);
return 0;    
    printf(LTOPO_TAG "**************STEP3:statistics calculate****************\n");
    printf(LTOPO_TAG "cp -f ../test/ltopo/data/output/ev.modified/* ../test/ltopo/data/output/ev/.\n");
    system("cp -f ../test/ltopo/data/output/ev.modified/* ../test/ltopo/data/output/ev/.");
        
    ltopo_statistics();    
#endif
    ltopo_alg_quit();
    sleep(2);
    ret++;
    return 0;    
}
