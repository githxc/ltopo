#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include <dirent.h>

#include "ltopo_xml.h"
#include "ltopo.h"
#include "ltopo_alg.h"
#include "ltopo_util.h"

LTOPO_INFO g_ltopo;


///////////////////////////////////////////////////////////////
//               meter list                                  //
///////////////////////////////////////////////////////////////
//从ltopo xml文件中读出的所有节点，记录在g_ltopo.meters中
//为解析出来的节点，设置参数
int ltopo_set_meter_para(XML_METER * meter, int index, char * key)
{
    switch(index)
    {
        case LTOPO_METER_PARA_TYPE:
            meter->mtype=atoi(key);
            break;
        case LTOPO_METER_PARA_ADDR:
            strncpy(meter->addr, key, LTOPO_ADDR_LEN-1);
            break;
        case LTOPO_METER_PARA_F_ADDR:
            strncpy(meter->f_addr, key, LTOPO_ADDR_LEN-1);
            break;
        case LTOPO_METER_PARA_RESULTBY:
            meter->resultby=atoi(key);
            break;
        case LTOPO_METER_PARA_ID:
            meter->id=atoi(key);
            break;
        case LTOPO_METER_PARA_F_ID:
            meter->f_id=atoi(key);
            break;
        case LTOPO_METER_PARA_LEVEL:
            meter->level=atoi(key);
            break;
        case LTOPO_METER_PARA_STATUS:
            meter->sm_state=atoi(key);
            break;
        case LTOPO_METER_PARA_COUNTER:
            meter->sm_counter=atoi(key);
            break;
        default:
            break;
   }
    return 0;
}
//在g_ltopo.meters尾部加入节点
int ltopo_add_meter_in_mlist(XML_METER * meter)
{
    LTOPO_METER_NODE * pnew;
    pnew=malloc(sizeof(LTOPO_METER_NODE));
    if(!pnew){
        printf(LTOPO_ERR_TAG "%s, malloc LTOPO_METER_NODE failed\n", __FUNCTION__);
        return -1;
        }
    memset(pnew, 0x0, sizeof(LTOPO_METER_NODE));
    pnew->minfo.mtype=meter->mtype;
    strncpy(pnew->minfo.addr, meter->addr, LTOPO_ADDR_LEN-1);
    strncpy(pnew->minfo.f_addr, meter->f_addr, LTOPO_ADDR_LEN-1);
    if(pnew->minfo.mtype==LTOPO_METER_TYPE_TTU){
        pnew->minfo.resultby=LTOPO_RESULT_BY_CAL;
        pnew->minfo.sm_state=LTOPO_SM_STATE_CONFIRMED;
        pnew->minfo.sm_counter=0;
        }
    else{
        pnew->minfo.resultby=meter->resultby;
        pnew->minfo.sm_state=meter->sm_state;
        pnew->minfo.sm_counter=meter->sm_counter;
        }
    pnew->minfo.id=meter->id;
    pnew->minfo.f_id=meter->f_id;
    pnew->minfo.level=meter->level;

    list_add(&g_ltopo.meters.list, &pnew->list);
    if(meter->mtype==LTOPO_METER_TYPE_MBOX)
        g_ltopo.mc++;
    else if(meter->mtype==LTOPO_METER_TYPE_BRANCH)
        g_ltopo.bc++;
    g_ltopo.mcount++;
    if(meter->mtype==LTOPO_METER_TYPE_TTU){
        g_ltopo.root=pnew;
        printf(LTOPO_TAG "got ttu %s\n", g_ltopo.root->minfo.addr);
        }
    return 0;
}
//通过addr获得链表中的节点指针
LTOPO_METER_NODE * ltopo_get_meter(char * addr, LTOPO_LIST * head)
{
    LTOPO_LIST *p;
    LTOPO_METER_NODE  *pmeter;
    list_for_each(p, head){
        pmeter=container_of(p, LTOPO_METER_NODE, list);
        if(!strncmp(pmeter->minfo.addr, addr, strlen(pmeter->minfo.addr)))
            return pmeter;
        }
    return NULL;
}
int ltopo_build_mlist_by_path()
{
    
    XML_METER meter;
    int id=0, phase=LTOPO_TYPE_PHASE_A, type, exists;
    DIR *dir = NULL;  
    struct dirent *entry;  
    LTOPO_LIST *p;
    LTOPO_METER_NODE  *pmeter;
    char addr[LTOPO_ADDR_LEN];

    printf(LTOPO_TAG "%s, X86 read %s\n", __FUNCTION__, g_ltopoalg.event_path);

    memset(&meter, 0x0, sizeof(meter));
    strcpy(meter.addr, "TTU");
    strcpy(meter.f_addr, "-1");
    meter.level=0;
    meter.id=id++;
    meter.mtype=LTOPO_METER_TYPE_TTU;
    meter.resultby=LTOPO_RESULT_BY_CAL;
    meter.sm_state=LTOPO_SM_STATE_CONFIRMED;
    meter.sm_counter=0;
    meter.f_id=-1;
    ltopo_add_meter_in_mlist(&meter);
    
    if((dir = opendir(g_ltopoalg.event_path))==NULL)   {  
        printf(LTOPO_ERR_TAG "opendir failed!");  
        return -1;  
        }
    while((entry=readdir(dir)))  {
        if(entry->d_type==8){
            if(!strncmp(entry->d_name, ltopo_bp_prefix[phase], 2) ||
                        !strncmp(entry->d_name, ltopo_mp_prefix[phase], 2)){
                exists=0;
                list_for_each(p, &g_ltopo.meters.list){
                    pmeter=container_of(p, LTOPO_METER_NODE, list);
                    strncpy(addr, &entry->d_name[3], 12);
                    if(!strcmp(addr, pmeter->minfo.addr)){
                        exists=1;
                        break;
                        }
                    }
                if(exists==1)
                    continue;
                
                if(entry->d_name[0]=='b')
                    type=LTOPO_METER_TYPE_BRANCH;
                else if(entry->d_name[0]=='m')
                    type=LTOPO_METER_TYPE_MBOX;
                else{
                    printf(LTOPO_TAG "%s, invalid file %s!\n", __FUNCTION__, entry->d_name);
                    return -1;
                    }
                memset(&meter, 0x0, sizeof(meter));
                strncpy(meter.addr, addr, LTOPO_ADDR_LEN);
                strcpy(meter.f_addr, "-1");
                meter.level=-1;
                meter.id=id++;
                meter.mtype=type;
                meter.resultby=-1;
                meter.sm_state=LTOPO_SM_STATE_UNKNOWN;
                meter.sm_counter=0;
                meter.f_id=-1;
                ltopo_add_meter_in_mlist(&meter);
               }
            }
        }
    closedir(dir);
    return 0;
}

//打印g_ltopo.meters
void ltopo_print_mlist(LTOPO_LIST * head)
{
    LTOPO_LIST *p;
    LTOPO_METER_NODE  *pmeter;
    printf(LTOPO_TAG "**************meters***************\n");
    list_for_each(p, head){
        pmeter=container_of(p, LTOPO_METER_NODE, list);
        printf(LTOPO_TAG "[meter]\n");
        printf(LTOPO_TAG "type %d, result by %d, addr %s, f_addr %s, id %d, f_id %d, level %d, status %d, counter %d\n",
            pmeter->minfo.mtype, pmeter->minfo.resultby, pmeter->minfo.addr, pmeter->minfo.f_addr,
            pmeter->minfo.id, pmeter->minfo.f_id, pmeter->minfo.level, pmeter->minfo.sm_state, pmeter->minfo.sm_counter);
        }
}

int ltopo_clean_mlist()
{
    list_fini(&g_ltopo.meters.list, meter_fini);
    g_ltopo.meters.list.next=NULL;
    g_ltopo.mcount=0;
    g_ltopo.mc=0;
    g_ltopo.bc=0;
    g_ltopo.root=NULL;
    return 0;
}
void ltopo_reset_mlist(LTOPO_LIST * head)
{
    LTOPO_LIST *p;
    LTOPO_METER_NODE  *pmeter;
    list_for_each(p, head){
        pmeter=container_of(p, LTOPO_METER_NODE, list);
        if(pmeter->minfo.mtype==LTOPO_METER_TYPE_TTU)
            continue;
        strcpy(pmeter->minfo.f_addr, "-1");
        pmeter->minfo.f_id=-1;
        pmeter->minfo.level=-1;
        pmeter->minfo.resultby=-1;
        pmeter->minfo.sm_state=LTOPO_SM_STATE_UNKNOWN;
        pmeter->minfo.sm_counter=0;
        }
    ltopo_xml_save_file(&g_ltopo.meters.list, LTOPO_XML_FILE);
}

void ltopo_reset_all_meters()
{
    printf(LTOPO_TAG "enter %s\n", __FUNCTION__);
    return ltopo_reset_mlist(&g_ltopo.meters.list);
}

void ltopo_print_all_meters()
{
    return ltopo_print_mlist(&g_ltopo.meters.list);
}
void meter_fini(void *arg)
{
        LTOPO_LIST *q = (LTOPO_LIST *)arg;
        struct struct_LTOPO_MBOX_NODE  *p = container_of(q, struct struct_LTOPO_MBOX_NODE, list);
/*
        LTOPO_MBOX_NODE *next_nodep = NULL;
        if (p->list.next != NULL)
                next_nodep = container_of(p->list.next, struct struct_LTOPO_MBOX_NODE, list);

        printf(LTOPO_TAG "%s, free (node) %p next (node) %p\n", __FUNCTION__, p, next_nodep);
*/
        p->list.next = NULL;
        free(p);
}
int ltopo_meters_dup(LTOPO_LIST *dhead, LTOPO_LIST *shead)
{
    LTOPO_LIST *p;
    LTOPO_METER_NODE  *pmeter, * pnew;
    list_for_each(p, shead){
        pmeter=container_of(p, LTOPO_METER_NODE, list);
        pnew=(LTOPO_METER_NODE*)malloc(sizeof(LTOPO_METER_NODE));
        if(!pnew){
            printf(LTOPO_ERR_TAG "%s, malloc meters failed\n", __FUNCTION__);
            goto err;
            }
        memcpy(pnew, pmeter, sizeof(LTOPO_METER_NODE));
        pnew->list.next=NULL;
        list_add(dhead, &pnew->list);
        }
    return 0;
err:
    list_fini(dhead, meter_fini);
    dhead->next=NULL;
    return -1;
}

///////////////////////////////////////////////////////////////
//               整条分支  list                                  //
///////////////////////////////////////////////////////////////
void branch_list_show(void *arg)
{
    LTOPO_LIST *q = (LTOPO_LIST *)arg;
    BRANCH_LIST  *p = container_of(q, BRANCH_LIST, list);
    METER_INFO * r=p->p;
    
    printf(LTOPO_TAG "[meter]\n");
    printf(LTOPO_TAG "type %d, addr %s, f_addr %s, evtfile %s\n",
        p->minfo.mtype, p->minfo.addr, p->minfo.f_addr, r->event_file);
}
void branch_list_fini(void *arg)
{
        LTOPO_LIST *q = (LTOPO_LIST *)arg;
        BRANCH_LIST  *p = container_of(q, BRANCH_LIST, list);
/*
        LTOPO_MBOX_NODE *next_nodep = NULL;
        if (p->list.next != NULL)
                next_nodep = container_of(p->list.next, struct struct_LTOPO_MBOX_NODE, list);

        printf(LTOPO_TAG "%s, free (node) %p next (node) %p\n", __FUNCTION__, p, next_nodep);
*/
        p->list.next = NULL;
        free(p);
}

//通过表箱终端的地址，创建整个branch的链表
int ltopo_build_whole_branch_by_mboxaddr(int phase, LTOPO_LIST * head, char *addr)
{
    int i, j;
    char father[LTOPO_ADDR_LEN];
    LTOPO_METER_NODE  *pmeter;
    BRANCH_LIST *pnew;

    for(i=0;i<g_ltopoalg.mcount[phase];i++){
        if(!strcmp(g_ltopoalg.mbox[phase][i].addr, addr))
            break;
        }
    if(i==g_ltopoalg.mcount[phase]){
        printf(LTOPO_ERR_TAG "%s, can't find the meter box %s\n", __FUNCTION__, addr);
        return -1;
        }
    pmeter=g_ltopoalg.mbox[phase][i].pm;
    if(!pmeter){
        printf(LTOPO_ERR_TAG "%s, g_ltopoalg.mbox[phase][i].pm NULL\n", __FUNCTION__);
        return -1;
        }
    //创建节点
    pnew=(BRANCH_LIST* )malloc(sizeof(BRANCH_LIST));
    if(!pnew){
        printf(LTOPO_ERR_TAG "%s, can't malloc for mbox\n", __FUNCTION__);
        goto err;
        }
    memcpy(&pnew->minfo, &pmeter->minfo, sizeof(LTOPO_METER_INFO));
    pnew->p=&g_ltopoalg.mbox[phase][i];
    pnew->type=LTOPO_METER_TYPE_MBOX;
    pnew->index=i;
    pnew->list.next=NULL;
    list_add(head, &pnew->list);
    
    strncpy(father, pnew->minfo.f_addr, LTOPO_ADDR_LEN-1);
    if(!strcmp(father, "-1")){
        printf(LTOPO_ERR_TAG "%s, mbox's father: -1\n", __FUNCTION__);
        goto err;
        }
   for(j=0;j<LTOPO_MAX_METERS;j++){
       for(i=0;i<g_ltopoalg.bcount[phase];i++){
           if(!strcmp(g_ltopoalg.branch[phase][i].addr, father))
                break;
           }
           if(i==g_ltopoalg.bcount[phase]){
                printf(LTOPO_ERR_TAG "%s, can't find the branch %s\n", __FUNCTION__, father);
                goto err;
                }
           pmeter=g_ltopoalg.branch[phase][i].pm;
           if(!pmeter){
               printf(LTOPO_ERR_TAG "%s, g_ltopoalg.branch[phase][i].pm NULL\n", __FUNCTION__);
               return -1;
               }
           //创建节点
           pnew=(BRANCH_LIST* )malloc(sizeof(BRANCH_LIST));
           if(!pnew){
               printf(LTOPO_ERR_TAG "%s, can't malloc for mbox\n", __FUNCTION__);
               goto err;
               }
           memcpy(&pnew->minfo, &pmeter->minfo, sizeof(LTOPO_METER_INFO));
           pnew->p=&g_ltopoalg.branch[phase][i];
           pnew->type=LTOPO_METER_TYPE_BRANCH;
           pnew->index=i;
           pnew->list.next=NULL;
           list_add_head(head, &pnew->list);
           
           strncpy(father, pnew->minfo.f_addr, LTOPO_ADDR_LEN-1);
           if(!strcmp(father, "-1") || !strcmp(father, g_ltopo.root->minfo.addr))
               break;
        }
       if(j==800){
           printf(LTOPO_ERR_TAG "%s, loop failed by father\n", __FUNCTION__);
           goto err; 
           }
       return 0;
   
err:
     list_fini(head, branch_list_fini);
     head->next=NULL;
     return -1;
}
//对整条分支的的节点，找到在g_ltopoalg.branch相应的指针，并设置他的excluded值
int ltopo_branch_list_set_exclude(LTOPO_LIST * head, int v)
{
    LTOPO_LIST * p;
    BRANCH_LIST * pbranchlist;
    METER_INFO *pminfo;
    
    list_for_each(p, head){
        pbranchlist=container_of(p, BRANCH_LIST, list);
        pminfo=pbranchlist->p;
        if(!pminfo){
            printf(LTOPO_ERR_TAG "%s, pbranchlist->p NULL\n", __FUNCTION__);
            return -1;
            }
        pminfo->excluded=v;
        }
    return 0;
}
#if 0
int ltopo_build_whole_branch_by_mboxaddr2(int phase, LTOPO_LIST * head, char *addr)
{
    int i, j;
    char father[LTOPO_ADDR_LEN];
    LTOPO_LIST *p;
    LTOPO_METER_NODE  *pmeter;
    BRANCH_LIST *pnew;
    //在meters中找到表箱终端
    list_for_each(p, &g_ltopo.meters.list){
        pmeter=container_of(p, LTOPO_MBOX_NODE, list);
        if(!strcmp(pmeter->minfo.addr, addr))
            break;
        }
    if(!p){
        printf(LTOPO_ERR_TAG "%s, can't find the known mbox\n", __FUNCTION__);
        return -1;
        }
    //创建节点
    pnew=(BRANCH_LIST* )malloc(sizeof(BRANCH_LIST));
    if(!pnew){
        printf(LTOPO_ERR_TAG "%s, can't malloc for mbox\n", __FUNCTION__);
        goto err;
        }
    memcpy(&pnew->minfo, &pmeter->minfo, sizeof(LTOPO_METER_INFO));
    pnew->p=NULL;
    for(i=0;i<g_ltopoalg.mcount[phase];i++){
        if(!strcmp(g_ltopoalg.mbox[phase][i].addr, addr)){
            pnew->p=&g_ltopoalg.mbox[phase][i];
            pnew->type=LTOPO_METER_TYPE_MBOX;
            pnew->index=i;
            break;
            }
        }
    if(pnew->p==NULL){
        printf(LTOPO_TAG "%s, can't find the meter box %s\n", __FUNCTION__, addr);
        free(pnew);
        return -1;
        }
    pnew->list.next=NULL;
    list_add(head, &pnew->list);
    
    strncpy(father, pnew->minfo.f_addr, LTOPO_ADDR_LEN-1);
    if(!strcmp(father, "-1")){
        printf(LTOPO_ERR_TAG "%s, mbox's father: -1\n", __FUNCTION__);
        goto err;
        }
   for(j=0;j<LTOPO_MAX_METERS;j++){
        list_for_each(p, &g_ltopo.meters.list){
            pmeter=container_of(p, LTOPO_MBOX_NODE, list);
            if(!strcmp(pmeter->minfo.addr, father))
                break;
            }
        if(!p){
            printf(LTOPO_ERR_TAG "%s, can't find meter's father\n", __FUNCTION__);
            goto err;
            }
        pnew=(BRANCH_LIST* )malloc(sizeof(BRANCH_LIST));
        if(!pnew){
            printf(LTOPO_ERR_TAG "%s, can't malloc for mbox\n", __FUNCTION__);
            goto err;
            }
        memcpy(&pnew->minfo, &pmeter->minfo, sizeof(LTOPO_METER_INFO));
        pnew->list.next=NULL;
        pnew->p=NULL;
        for(i=0;i<g_ltopoalg.bcount[phase];i++){
            if(!strcmp(g_ltopoalg.branch[phase][i].addr, pnew->minfo.addr)){
                pnew->p=&g_ltopoalg.branch[phase][i];
                pnew->type=LTOPO_METER_TYPE_BRANCH;
                pnew->index=i;
                break;
                }
            }
        list_add_head(head, &pnew->list);

        strncpy(father, pnew->minfo.f_addr, LTOPO_ADDR_LEN-1);
        if(!strcmp(father, "-1"))
            break;
        }
   
    if(j==800){
        printf(LTOPO_ERR_TAG "%s, loop failed by father\n", __FUNCTION__);
        goto err; 
        }
    return 0;

err:
    list_fini(head, meter_fini);
    head->next=NULL;
    return -1;
}
#endif //0
///////////////////////////////////////////////////////////////
//               btable的操作                                   //
///////////////////////////////////////////////////////////////
//g_ltopo.btable，记录已经确定的节点，
//是一个链表数组，按分支的level存储
//向g_ltopo.btable中插入branch节点
int ltopo_insert_bnode_in_btable(XML_METER * node, LTOPO_BRANCH_NODE * father, int level)
{
    LTOPO_BRANCH_NODE * pnew;
    pnew=malloc(sizeof(LTOPO_BRANCH_NODE));
    if(!pnew){
        printf(LTOPO_ERR_TAG "%s, malloc LTOPO_BRANCH_NODE failed\n", __FUNCTION__);
        return -1;
        }
    memset(pnew, 0x0, sizeof(LTOPO_BRANCH_NODE));
/*    
    pnew->binfo.mtype=node->mtype;
    strncpy(pnew->binfo.addr, node->addr, LTOPO_ADDR_LEN-1);
    strncpy(pnew->binfo.f_addr, node->f_addr, LTOPO_ADDR_LEN-1);
*/
    memcpy(&pnew->binfo, node, sizeof(XML_METER));
    pnew->father=father;

    list_add(&g_ltopo.btable[level].list, &pnew->list);

    return 0;
    
}
//向g_ltopo.btable中插入mbox节点
int ltopo_insert_mnode_in_btable(XML_METER * node, LTOPO_BRANCH_NODE * pbranch)
{
    LTOPO_MBOX_NODE  * pnew;
    pnew=malloc(sizeof(LTOPO_MBOX_NODE));
    if(!pnew){
        printf(LTOPO_ERR_TAG "%s, malloc LTOPO_MBOX_NODE failed\n", __FUNCTION__);
        return -1;
        }
    memset(pnew, 0x0, sizeof(LTOPO_MBOX_NODE));
    /*
    pnew->minfo.mtype=node->mtype;
    strncpy(pnew->minfo.addr, node->addr, LTOPO_ADDR_LEN-1);
    strncpy(pnew->minfo.f_addr, node->f_addr, LTOPO_ADDR_LEN-1);
    */
    memcpy(&pnew->minfo, node, sizeof(XML_METER));

    list_add(&pbranch->mbox.list, &pnew->list);
    return 0;
}
//构建btable，
//如果react为1，则反作用head链表，赋值level，id，f_id等
static int ltopo_build_btable(LTOPO_LIST * head, int react)
{
    int i, ret;
    
    LTOPO_LIST * pm, * pb;
    LTOPO_METER_NODE * pmeter=NULL;
    LTOPO_BRANCH_NODE * pnode=NULL;
    printf(LTOPO_TAG "Enter %s\n", __FUNCTION__);
    
    list_for_each(pm, head){
        pmeter=container_of(pm, LTOPO_METER_NODE, list);
        if(pmeter->minfo.mtype==LTOPO_METER_TYPE_TTU)
            break;
        }
    if(!pmeter){
        printf(LTOPO_ERR_TAG "can't find root meter\n");
        return -1;
    }
    if(react){
        pmeter->minfo.f_id=-1;
        pmeter->minfo.level=0;
        }
    ret=ltopo_insert_bnode_in_btable((XML_METER *)&pmeter->minfo, NULL, 0);    
    if(ret!=0){
        printf(LTOPO_ERR_TAG "%s, ltopo_insert_bnode_in_btable root failed\n", __FUNCTION__);
        goto err;
        }
        
    for(i=1;i<LTOPO_BRANCH_LEVEL_MAX;i++){
        list_for_each(pb, &g_ltopo.btable[i-1].list){
            pnode=container_of(pb, LTOPO_BRANCH_NODE, list);
            list_for_each(pm, head){
                pmeter=container_of(pm, LTOPO_METER_NODE, list);
                if(pmeter->minfo.mtype==LTOPO_METER_TYPE_BRANCH 
                                    && !strcmp(pmeter->minfo.f_addr, pnode->binfo.addr)){
                    if(react){
                        pmeter->minfo.f_id=pnode->binfo.id;
                        pmeter->minfo.level=i;
                        printf(LTOPO_TAG "**set branch %s f_id %d, id %d\n", pmeter->minfo.addr, pmeter->minfo.f_id, pmeter->minfo.id);
                        }
                    ret=ltopo_insert_bnode_in_btable((XML_METER *)&pmeter->minfo, pnode, i);
                    if(ret!=0){
                        printf(LTOPO_ERR_TAG "%s, ltopo_insert_bnode_in_btable failed\n", __FUNCTION__);
                        goto err;
                        }
                    }
                }
            }
        }
    for(i=0;i<LTOPO_BRANCH_LEVEL_MAX;i++){
        list_for_each(pb, &g_ltopo.btable[i].list){
            pnode=container_of(pb, LTOPO_BRANCH_NODE, list);
            list_for_each(pm, head){
                pmeter=container_of(pm, LTOPO_METER_NODE, list);
                if(pmeter->minfo.mtype==LTOPO_METER_TYPE_MBOX 
                                    && !strcmp(pmeter->minfo.f_addr, pnode->binfo.addr)){
                    
                    if(react){
                        pmeter->minfo.f_id=pnode->binfo.id;
                        pmeter->minfo.level=i+1;
                        printf(LTOPO_TAG "**set mbox %s f_id %d, id %d\n", pmeter->minfo.addr, pmeter->minfo.f_id, pmeter->minfo.id);
                        }
                    ret=ltopo_insert_mnode_in_btable((XML_METER *)&pmeter->minfo, pnode);
                    if(ret!=0){
                        printf(LTOPO_ERR_TAG "%s, ltopo_insert_mnode_in_btable failed\n", __FUNCTION__);
                        goto err;
                        }
                    }
                }
            }
        }
    return 0;
err:
    ltopo_clean_btable();
    return -1;
}

void ltopo_print_btable()
{
    int i;
    LTOPO_LIST *pb, * pm;
    LTOPO_BRANCH_NODE * pnode;
    LTOPO_MBOX_NODE * pmnode;
    printf(LTOPO_TAG "Enter %s\n", __FUNCTION__);
    for(i=0;i<LTOPO_BRANCH_LEVEL_MAX;i++){
        printf(LTOPO_TAG "####level %d\n", i);
        list_for_each(pb, &g_ltopo.btable[i].list){
            pnode=container_of(pb, LTOPO_BRANCH_NODE, list);
            printf(LTOPO_TAG "ADDR %s, F_ADDR %s, RESULTBY %d, ID %d, F_ID %d, LEVEL %d, status %d, counter %d\n", pnode->binfo.addr, pnode->binfo.f_addr,
                                pnode->binfo.resultby, pnode->binfo.id, pnode->binfo.f_id, pnode->binfo.level, pnode->binfo.sm_state, pnode->binfo.sm_counter);
            list_for_each(pm, &pnode->mbox.list){
                pmnode=container_of(pm, LTOPO_MBOX_NODE, list);
                printf(LTOPO_TAG "\tmbox: ADDR %s, F_ADDR %s, RESULTBY %d, ID %d, F_ID %d, LEVEL %d, status %d, counter %d\n", pmnode->minfo.addr, pmnode->minfo.f_addr,
                                pmnode->minfo.resultby, pmnode->minfo.id, pmnode->minfo.f_id, pmnode->minfo.level, pmnode->minfo.sm_state, pmnode->minfo.sm_counter);
                }
            }
        }
}

static void
branch_fini(void *arg)
{
        LTOPO_LIST *q = (LTOPO_LIST *)arg;
        LTOPO_BRANCH_NODE  *p = container_of(q, LTOPO_BRANCH_NODE, list);
/*
        LTOPO_BRANCH_NODE *next_nodep = NULL;
        if (p->list.next != NULL)
                next_nodep = container_of(p->list.next, LTOPO_BRANCH_NODE, list);
        printf(LTOPO_TAG "*free (node) %p next (node) %p\n", p, next_nodep);
*/
        list_fini(&p->mbox.list, meter_fini);
        p->mbox.list.next=NULL;
        p->list.next = NULL;
        free(p);
}
int ltopo_clean_btable()
{
    int i;

    for(i=0;i<LTOPO_BRANCH_LEVEL_MAX;i++){
        list_fini(&g_ltopo.btable[i].list, branch_fini);
        g_ltopo.btable[i].list.next=NULL;
        }
    return 0;
}

void ltopo_clean()
{
    ltopo_clean_mlist();
    ltopo_clean_btable();
#ifdef LTOPO_ENABLE_THREAD    
    sem_destroy(&g_ltopo.sem);
    pthread_mutex_destroy(&g_ltopo.mutex);
#endif
}
int ltopo_refresh_mlist(LTOPO_LIST * head, char * filename)
{
    ltopo_clean_btable();
    ltopo_build_btable(head, 1);
    ltopo_print_btable();
    ltopo_clean_btable();
    
    ltopo_xml_save_file(head, filename);
    return 0;

}
int ltopo_print_topo(LTOPO_LIST * head)
{
    ltopo_clean_btable();
    ltopo_build_btable(head, 0);
    ltopo_print_btable();
    ltopo_clean_btable();
    return 0;
}
int ltopo_build_mlist_by_archive(int acount, LTOPO_ARCHIVE * a)
{
    int i;
    XML_METER meter;
    for(i=0;i<acount;i++){
        memset(&meter, 0x0, sizeof(meter));
        strncpy(meter.addr, a[i].addr, LTOPO_ADDR_LEN);
        meter.id=a[i].id;
        meter.mtype=a[i].type;
        strcpy(meter.f_addr, "-1");
        meter.f_id=-1;
        meter.level=-1;
        meter.resultby=-1;
        meter.sm_state=LTOPO_SM_STATE_UNKNOWN;
        meter.sm_counter=0;
        ltopo_add_meter_in_mlist(&meter);
        }
    ltopo_xml_save_file(&g_ltopo.meters.list, LTOPO_XML_FILE);
    return 0;  
}

int ltopo_read_archive(int acount, LTOPO_ARCHIVE * a)
{
    int i, ret;
    LTOPO_LIST *p;
    LTOPO_METER_NODE  *pmeter;
    XML_METER meter;
    printf(LTOPO_TAG "Enter %s, archive count %d\n", __FUNCTION__, acount);
    
    if(g_ltopo.mcount==0){
        printf(LTOPO_WARN_TAG "%s, no data in xml! Build from archive.\n", __FUNCTION__);
        return ltopo_build_mlist_by_archive(acount, a);
        }
#ifdef X86_LINUX
    if(acount==0){
        printf(LTOPO_TAG "Linux X86, no archive\n");
        return 0;
        }
#endif 

    /*
    档案发生变化
    1）对每个线路拓扑中的每个节点，遍历档案，如果节点多出来，
        则有新节点被移除出档案，
        如果是表箱终端，则将该节点移除出线路拓扑，
        如果是分支单元则清空线路拓扑数据，重建线路拓扑。
    2）对每个档案，遍历线路拓扑的每个节点，
        如果档案多出来，则有新的档案加入，将该档案加入到线路拓扑中。
    */
    //先检查TTU
    list_for_each(p, &g_ltopo.meters.list){
        pmeter=container_of(p, LTOPO_METER_NODE, list);
        if(pmeter->minfo.mtype==LTOPO_METER_TYPE_TTU){
            for(i=0;i<acount;i++){
                if( a[i].type==LTOPO_METER_TYPE_TTU){
                    if(strcmp(pmeter->minfo.addr, a[i].addr)){
                        printf(LTOPO_TAG "TTU addr changed, %s->%s\n", pmeter->minfo.addr, a[i].addr);
                        strncpy(pmeter->minfo.addr, a[i].addr, LTOPO_ADDR_LEN);
                        ltopo_xml_save_file(&g_ltopo.meters.list, LTOPO_XML_FILE);
                        }
                    else{
                        printf(LTOPO_TAG "TTU addr unchanged, %s\n", pmeter->minfo.addr);
                        }
                    break;
                    }
                }
            break;
            }
        }
    
    //对于链表上的每个meter，遍历全部档案，检出被移除档案的meter
    list_for_each(p, &g_ltopo.meters.list){
        //exists=0;
        pmeter=container_of(p, LTOPO_METER_NODE, list);
        for(i=0;i<acount;i++){
            if(!strcmp(pmeter->minfo.addr, a[i].addr)){
                //exists=1;
                if(pmeter->minfo.mtype!=a[i].type || pmeter->minfo.id!=a[i].id){
                    printf(LTOPO_TAG "%s, orig type %d id %d, archive type %d id %d\n",
                            pmeter->minfo.addr, pmeter->minfo.mtype, pmeter->minfo.id, a[i].type, a[i].id);
                    printf(LTOPO_TAG "Rebuild mlist\n");
                    ltopo_clean_mlist();
                    return ltopo_build_mlist_by_archive(acount, a);
                    }
                break;
                }
            }
        //如果链表节点多出来，则有链表节点被移除出档案
        if(i==acount){
            if(pmeter->minfo.mtype==LTOPO_METER_TYPE_MBOX){
                //从链表中删除该节点，重新存储ltopo.xml
                printf(LTOPO_TAG "%s, no archive, remove mbox %s\n", __FUNCTION__, pmeter->minfo.addr);
                ret=list_remove(&g_ltopo.meters.list, &pmeter->list);
                if(ret==0){
                    g_ltopo.mcount--;
                    g_ltopo.mc--;
                    }
                ltopo_xml_save_file(&g_ltopo.meters.list, LTOPO_XML_FILE);
                //continue;
                }
            else if(pmeter->minfo.mtype==LTOPO_METER_TYPE_BRANCH){
                //清除整个链表，按表档案构建新的链表
                printf(LTOPO_TAG "%s, no branch archive %s, remove all meters\n", __FUNCTION__, pmeter->minfo.addr);
                ltopo_clean_mlist();
                return ltopo_build_mlist_by_archive(acount, a);
                }
            }
        }
    //对于每个档案，遍历链表上的每个meter，检出新增的档案
    for(i=0;i<acount;i++){
        list_for_each(p, &g_ltopo.meters.list){
            //exists=0;
            pmeter=container_of(p, LTOPO_METER_NODE, list);
            if(!strcmp(pmeter->minfo.addr, a[i].addr)){
                //exists=1;
                break;
                }
            }
        if(p==NULL){
            //增加了新的表箱终端或分支单元的档案，加入链表，重新存储ltopo.xml
            printf(LTOPO_TAG "%s, new archive, add %s, type %d\n", __FUNCTION__, a[i].addr, a[i].type);
            memset(&meter, 0x0, sizeof(meter));
            strncpy(meter.addr, a[i].addr, LTOPO_ADDR_LEN);
            meter.id=a[i].id;
            meter.mtype=a[i].type;
            strcpy(meter.f_addr, "-1");
            meter.f_id=-1;
            meter.level=-1;
            meter.resultby=-1;
            meter.sm_state=LTOPO_SM_STATE_UNKNOWN;
            meter.sm_counter=0;
            ltopo_add_meter_in_mlist(&meter);
            ltopo_xml_save_file(&g_ltopo.meters.list, LTOPO_XML_FILE);
            }
        }
    return 0;
}

int ltopo_load_xmldata()
{
    int ret=0;
    ret=ltopo_xml_load_file(LTOPO_XML_FILE);
    if(ret!=0){
        printf(LTOPO_WARN_TAG "%s, load xml failed\n", __FUNCTION__);
        }

    return 0;
}
int ltopo_alg_run_phase(int phase,FILE *fpo)
{
    int ret;
#ifdef X86_LINUX
    //没有初始配置文件和档案，从目录自动生成
    if(g_ltopo.mcount==0){
        ltopo_build_mlist_by_path();
        ltopo_print_all_meters();
        ltopo_xml_save_file(&g_ltopo.meters.list, LTOPO_XML_FILE);
        }
#endif
    //按相位读事件数据，并恢复成负荷数据
    ret=ltopo_alg_read_path(phase);
    if(ret!=0){
        printf(LTOPO_ERR_TAG "%s, Warning, ltopo_read_dir failed\n", __FUNCTION__);
        return -1;
        }
    //调试对照用
    ltopo_alg_debug_save_load(phase);
    //扫描处理
    ret=ltopo_alg_scan(phase, g_ltopoalg.action,fpo);
    if(ret!=0){
        printf(LTOPO_ERR_TAG "%s, Error, ltopo_alg_scan failed\n", __FUNCTION__);
        }
    ltopo_alg_clean(phase);
    return ret;
}

int ltopo_alg_run()
{
    int i, j, k, ret;
	FILE* fpo;
	fpo=fopen("./shishi.txt", "wb");
    printf(LTOPO_TAG "%s, cycle %d, vl %d, vh %d, window %d, path %s\n", __FUNCTION__, 
        g_ltopoalg.mcycle, g_ltopoalg.vl, g_ltopoalg.vh, g_ltopoalg.wsize, g_ltopoalg.event_path);
#ifdef X86_LINUX        
    for(i=LTOPO_TYPE_PHASE_A;i<LTOPO_TYPE_PHASE_MAX;i++){
#else        
    for(i=LTOPO_TYPE_PHASE_T;i<LTOPO_TYPE_PHASE_A;i++){
#endif        

		
        ret=ltopo_alg_run_phase(i,fpo);
        if(ret!=0){
            printf(LTOPO_WARN_TAG "%s, ltopo_alg_run_phase failed, phase %d!\n", __FUNCTION__, i);
            if(g_ltopo.conflicts==1){
                printf(LTOPO_ERR_TAG "%s, ltopo_alg_run_phase conflicts, phase %d!\n", __FUNCTION__, i);
                g_ltopo.conflicts=0;
                return LTOPO_ERR_CONFLICTS;
                }
            }

		float matching;
		for(j=0;j<g_ltopoalg.mcount[i];j++)
		{
			for(k=0;j<g_ltopoalg.bcount[i];k++)
			{
				matching = ltopo_alg_matching(g_ltopoalg.mbox[i][j]->head,g_ltopoalg.branch[i][k]->head);
				if(matching > 0.8)
					fprintf(fpo,"%s %s matching",g_ltopoalg.mbox[i][j].addr,g_ltopoalg.branch[i][k].addr);
			}
				
		}
		
        }
	
		
		
    return 0;
}
#ifdef LTOPO_ENABLE_THREAD
void* ltopo_thread_alg(void *arg)
{
    int ret;
    printf(LTOPO_TAG "Enter %s\n", __FUNCTION__);

    g_ltopo.running=1;
    while(g_ltopo.running){
        struct timespec to;

        to.tv_sec = time(NULL) + 2;
        to.tv_nsec = 0;

        if (sem_timedwait (&g_ltopo.sem, &to)) {
            //printf(LTOPO_TAG "%s, sem_timedwait timeout\n", __FUNCTION__);
            continue;
            }
        printf(LTOPO_TAG "*****************************************\n");
        printf(LTOPO_TAG "sem wait ok\n");
        ret=ltopo_alg_run();
        if(ret==LTOPO_ERR_CONFLICTS){
            printf(LTOPO_ERR_TAG "ltopo_alg_run conflicts\n");
            ltopo_reset_all_meters();
            }

        if(g_ltopo.cb_fini)
		    g_ltopo.cb_fini(g_ltopoalg.event_path, ret);
        }
    ltopo_clean();
    printf(LTOPO_TAG "Leave %s\n", __FUNCTION__);
    g_ltopo.running=-1;
    return NULL;
}
#endif
int ltopo_thread_create(pthread_t *tid, void *(*function)(void *), void *arg, unsigned char mode, unsigned char prio)
{
    int ret = 0;
    pthread_attr_t attr;
    struct sched_param param;

printf(LTOPO_TAG "%s, mode %d, prio %d\n", __FUNCTION__, mode, prio);
//初始化线程属性
    ret = pthread_attr_init(&attr);
    if (ret) {
        ret = -1;
        goto error;
    }
//设置线程为分离状态
    ret = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    if (ret) {
        ret = -2;
        goto error;
    }
//设置优先级
    if (THREAD_MODE_NORMAL == mode) {
        if (0 != prio) {
            ret = -3;
            goto error;
        }
    }
    else if (THREAD_MODE_REALTIME == mode) {
        if ((prio < 1) || (prio > 99)) {			//1是实时线程最低优先级，99是最高优先级
            ret = -4;
            goto error;
        }
        ret = pthread_attr_setschedpolicy(&attr, SCHED_RR);
        if (ret) {
            ret = -5;
            goto error;
        }
        param.sched_priority = prio;
        ret = pthread_attr_setschedparam(&attr, &param);
        if (ret) {
            ret = -6;
            goto error;
        }
    }
    else {
        ret = -7;
        goto error;
    }
//建立线程
    ret = pthread_create (tid, &attr, function, arg);
    if (ret) {
        ret = -8;
        goto error;
    }
	return 0;
error:
	printf(LTOPO_TAG "Err %d\n", ret);
    return ret;
}
void ltopo_mkdir()
{
    char cmd[256];
    snprintf(cmd, sizeof(cmd)-1, "mkdir -p %s", LTOPO_XML_STATIS_PATH);
    system(cmd);
    snprintf(cmd, sizeof(cmd)-1, "mkdir -p %s", LTOPO_RESULT_RECORD_PATH);
    system(cmd);
    snprintf(cmd, sizeof(cmd)-1, "mkdir -p %s", LTOPO_RESTORE_LOAD_PATH);
    system(cmd);
    return ;
}

char * ltopo_alg_version="0.1.8"; 

#define _MACROSTR(x) #x
#define MACROSTR(x) _MACROSTR(x)
#define BUILDTIME MACROSTR(COMP_TIME)

int ltopo_init(int acount, LTOPO_ARCHIVE * a, void (*callback)(char * path, int result))
{
    int ret;
    printf(LTOPO_TAG "%s, alg version %s\n", __FUNCTION__, ltopo_alg_version);
#ifdef COMP_TIME
    printf(LTOPO_TAG "%s, building at %s\n", __FUNCTION__, BUILDTIME);
#endif
    if(g_ltopo.status!=LTOPO_STATUS_NOT_INITED){
        printf(LTOPO_ERR_TAG "%s, already initialized, ignore !\n", __FUNCTION__);
        return -1;
        }
    memset(&g_ltopo, 0x0, sizeof(LTOPO_INFO));
    g_ltopo.cb_fini=callback;
    ltopo_alg_init();
    ltopo_mkdir();
    ret=ltopo_load_xmldata();
    if(ret!=0){
        printf(LTOPO_ERR_TAG "%s, ltopo_load_xmldata failed\n", __FUNCTION__);
        return -1;
        }
printf(LTOPO_TAG "%s, ltopo_print_mlist after ltopo_load_xmldata\n", __FUNCTION__);
printf(LTOPO_TAG "%s, mcount %d, bcount %d\n", __FUNCTION__, g_ltopo.mc, g_ltopo.bc);
ltopo_print_mlist(&g_ltopo.meters.list);
    
    ltopo_read_archive(acount, a);
    if(g_ltopo.mcount==0){
#ifdef X86_LINUX
        printf(LTOPO_WARN_TAG "%s, no meter in xml and archive, will build in the future\n", __FUNCTION__);
#else
        printf(LTOPO_ERR_TAG "%s, no meter in xml and archive\n", __FUNCTION__);
        return -1;
#endif        
        }
    if(g_ltopo.root==NULL){
#ifdef X86_LINUX
        printf(LTOPO_WARN_TAG "%s, no root in xml and archive, will build in the future\n", __FUNCTION__);
#else
        printf(LTOPO_ERR_TAG "%s, no root in xml and archive\n", __FUNCTION__);
        return -1;
#endif        
        }
    printf(LTOPO_TAG "%s, ltopo_print_mlist after ltopo_read_archive\n", __FUNCTION__);
    printf(LTOPO_TAG "%s, mcount %d, bcount %d\n", __FUNCTION__, g_ltopo.mc, g_ltopo.bc);
    ltopo_print_mlist(&g_ltopo.meters.list);
    
#ifdef LTOPO_ENABLE_THREAD    
	pthread_t tid;
    //创建信号量
	if (sem_init(&g_ltopo.sem, 0, 0)) 
	{
		printf(LTOPO_ERR_TAG "%s, sem_init failed\n", __FUNCTION__);
        ltopo_clean_mlist();
        return -1;
	}
	if (pthread_mutex_init(&g_ltopo.mutex, NULL)) 
	{
		printf(LTOPO_TAG "%s, pthread_mutex_init failed\n", __FUNCTION__);
        ltopo_clean_mlist();
        sem_destroy(&g_ltopo.sem);
        return -1;
	}
    
    
    ret = ltopo_thread_create (&tid, ltopo_thread_alg, NULL, THREAD_MODE_NORMAL, 0);
    if (ret) {
        printf(LTOPO_TAG "%s, ttuapp_thread_create failed\n", __FUNCTION__);
        ltopo_clean_mlist();
        sem_destroy(&g_ltopo.sem);
        pthread_mutex_destroy(&g_ltopo.mutex);
        return -1;
    }
#endif    
    g_ltopo.status=LTOPO_STATUS_INITED;
    return 0;
}
int ltopo_config(int cycle, int vh, int vl, int wsize, int flag)
{
    if(g_ltopo.status!=LTOPO_STATUS_INITED && g_ltopo.status!=LTOPO_STATUS_JOB_FINISHED ){
        printf(LTOPO_ERR_TAG "%s, invalid status %d!\n", __FUNCTION__, g_ltopo.status);
        return -1;
        }
    ltopo_alg_config(cycle, vh, vl, wsize, flag);
    return 0;
}
int ltopo_get_shell_result(char * cmd, char * rbuf, int rlen)
{
    FILE *fpr;
    fpr = popen(cmd, "r");
    if (NULL == fpr)
    {
            printf("popen %s failed\n", cmd);
            return -1;
    }

    memset(rbuf,0,rlen);
    if(NULL == fgets(rbuf,rlen,fpr))
    {
            pclose(fpr);
            return -1;
    }
    printf("%s", rbuf);
    pclose(fpr);
    return 0;
}

int ltopo_job_set_path(char * path)
{
    if(g_ltopo.status!=LTOPO_STATUS_INITED && g_ltopo.status!=LTOPO_STATUS_JOB_FINISHED ){
        printf(LTOPO_ERR_TAG "%s, invalid status %d!\n", __FUNCTION__, g_ltopo.status);
        return -1;
        }
    return ltopo_alg_set_path(path);
}

int ltopo_job_start(int action)
{
    int ret;
    printf(LTOPO_TAG "Enter %s\n", __FUNCTION__);
    g_ltopoalg.action=action;
#ifdef LTOPO_ENABLE_THREAD    
    ret=sem_post (&g_ltopo.sem); 
#else
    ret=ltopo_alg_run();
    if(ret==LTOPO_ERR_CONFLICTS){
        printf(LTOPO_ERR_TAG "ltopo_alg_run conflicts\n");
        //ltopo_reset_all_meters();
        //ltopo_print_topo(&g_ltopo.meters.list);
        }

    if(g_ltopo.cb_fini)
        g_ltopo.cb_fini(g_ltopoalg.event_path, ret);
#endif
    return ret;
}
int ltopo_alg_quit()
{
    printf(LTOPO_TAG "Enter %s\n", __FUNCTION__);
#ifdef LTOPO_ENABLE_THREAD    
    g_ltopo.running=0;
    sleep(2);
    if(g_ltopo.running==-1)
        return 1;
    else
        return 0;
#else
    ltopo_clean();
#endif
    return 1;

}
int ltopo_statistics()
{
    return ltopo_alg_statistics(LTOPO_XML_STATIS_PATH);
}

