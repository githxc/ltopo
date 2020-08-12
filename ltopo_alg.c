#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <dirent.h>
#include "ltopo_xml.h"
#include "ltopo_alg.h"
#include "ltopo.h"

char * ltopo_phase_name[]={"t", "a", "b", "c"};
char * ltopo_bp_prefix[]={"bt", "ba", "bb", "bc"}; // branch and phase prefix
char * ltopo_mp_prefix[]={"mt", "ma", "mb", "mc"}; // mbox and phase prefix
#if 0 //def M1
int ltopo_print_event(LTOPO_EVENT *head)
{
    int i;
    LTOPO_EVENT * p;
    p=head;
    printf(LTOPO_TAG "**type %d, s %d e %d, d %d\n", p->type, (int)p->start, (int)p->start+p->vcount, p->vcount);
    if(p->type==LTOPO_EVENT_TYPE_JUMP){
        for(i=0;i<=p->vcount;i++)
            printf(LTOPO_TAG "\t%d ", p->load[i].ap);
        printf(LTOPO_TAG "\n");
        }
    else if(p->type==LTOPO_EVENT_TYPE_SMOOTH)
    printf(LTOPO_TAG "\t%d\n", p->load[0].ap);

    return 0;
}
int ltopo_print_event_list(FILE * fp, LTOPO_EVENT *head)
{
    int i;
    LTOPO_EVENT * p;
    p=head;
    while(p){
        fprintf(fp, "\ttype %d, s %d e %d, d %d\n", p->type, (int)p->start, (int)p->start+p->vcount, p->vcount);
        if(p->type==LTOPO_EVENT_TYPE_JUMP){
            for(i=0;i<=p->vcount;i++)
                fprintf(fp, "\t%d ", p->load[i].ap);
            fprintf(fp, "\n");
            }
        else if(p->type==LTOPO_EVENT_TYPE_SMOOTH)
        fprintf(fp, "\t%d %d\n", p->load[0].ap, p->load[1].ap);
        p=p->next;
        }
    return 0;
}
LTOPO_EVENT * build_event(char * filename)
{
//    int ret;
    char fullname[128];
    LTOPO_EVENT event, *pnew=NULL, * phead=NULL, *prear=NULL;
    FILE * fp=NULL;

    printf(LTOPO_TAG "enter %s\n", __FUNCTION__);
    snprintf(fullname, sizeof(fullname)-1, "%s/%s", g_ltopoalg.event_path, filename);

    fp=fopen(fullname, "rb");
    if(!fp){
        printf(LTOPO_ERR_TAG "%s failed, open %s!!!\n", __FUNCTION__, fullname);
        return NULL;
        }
    while(fread (&event, sizeof (LTOPO_EVENT), 1, fp)>0)
    {
        pnew=(LTOPO_EVENT *)malloc(sizeof(LTOPO_EVENT));
        if(!pnew){
            printf(LTOPO_ERR_TAG "%s failed, malloc\n", __FUNCTION__);
            goto err;
            }
        memcpy(pnew, &event, sizeof(LTOPO_EVENT));
        pnew->next=NULL;
        if(!phead){
            phead=pnew;
            prear=pnew;
            }
        else{
            prear->next=pnew;
            prear=pnew;
            }
    }
    fclose(fp);
    return phead;
err:
    pnew=phead;
    while(pnew){
        phead=pnew->next;
        free(pnew);
        pnew=phead;
        }
    if(fp)
        fclose(fp);
    return NULL;
}
int ltopo_read_dir(int phase)
{
    int bcount=0, mcount=0; // bcount: phase 相位的branch个数, mcount: phase相位的meter box个数
    DIR *dir = NULL;  
    struct dirent *entry;  

    if(phase<0 || phase>=LTOPO_TYPE_PHASE_MAX){
        printf(LTOPO_ERR_TAG "__FUNCTION__: invalid phase %d\n", phase);
        return -1;
        }

    printf(LTOPO_TAG "%s, phase %s\n", __FUNCTION__, ltopo_phase_name[phase]);

    if((dir = opendir(g_ltopoalg.event_path))==NULL)   {  
        printf(LTOPO_ERR_TAG "opendir failed!");  
        return -1;  
        }
    while((entry=readdir(dir)))  {
        if(entry->d_type==8){
            if(!strncmp(entry->d_name, ltopo_bp_prefix[phase], 2))
                bcount++;
            else if(!strncmp(entry->d_name, ltopo_mp_prefix[phase], 2))
                mcount++;
            }
        }
    closedir(dir);
    dir=NULL;
    if(mcount==0 || bcount==0){
        printf(LTOPO_ERR_TAG "%s failed: no data file\n", __FUNCTION__);
        return -1;
        }
    if(verify_mcount_bcount(mcount, bcount)!=0){
        printf(LTOPO_ERR_TAG "mcount & bcount verification failed!\n"); 
        printf(LTOPO_ERR_TAG "mcount %d, bcount %d\n", mcount, bcount); 
        return -1;
        }
    g_ltopoalg.mbox[phase]=(METER_INFO *)malloc(mcount*sizeof(METER_INFO));
    g_ltopoalg.branch[phase]=(METER_INFO *)malloc(bcount*sizeof(METER_INFO));
    if(!g_ltopoalg.mbox[phase] || !g_ltopoalg.branch[phase]){
        printf(LTOPO_ERR_TAG "g_ltopoalg malloc pmbox(%d) or pbranch(%d) for %s failed\n", 
                            mcount, bcount, ltopo_phase_name[phase]);
        goto err;
        }
    memset(g_ltopoalg.mbox[phase], 0x0, mcount*sizeof(METER_INFO));
    memset(g_ltopoalg.branch[phase], 0x0, bcount*sizeof(METER_INFO));
    g_ltopoalg.bcount[phase]=bcount;
    g_ltopoalg.mcount[phase]=mcount;
    if((dir = opendir(g_ltopoalg.event_path))==NULL)   {  
        printf(LTOPO_TAG "opendir failed!");  
        goto err;  
        }
    bcount=0; 
    mcount=0; 
    while((entry=readdir(dir)))  {
        if(entry->d_type==8){
            if(!strncmp(entry->d_name, ltopo_bp_prefix[phase], 2)){
                if(bcount>=g_ltopoalg.bcount[phase]){
                    printf(LTOPO_ERR_TAG "%s failed, more branch files\n", __FUNCTION__);
                    goto err;
                    }
                strncpy(g_ltopoalg.branch[phase][bcount].event_file, entry->d_name, FILENAME_LEN-1);
                g_ltopoalg.branch[phase][bcount].event=build_event(entry->d_name);
                if(!g_ltopoalg.branch[phase][bcount].event){
                    printf(LTOPO_ERR_TAG "%s build event failed, %s\n", __FUNCTION__, 
                                    g_ltopoalg.branch[phase][bcount].event_file);
                    goto err;
                    }
                bcount++;
                }
            else if(!strncmp(entry->d_name, ltopo_mp_prefix[phase], 2)){
                if(mcount>=g_ltopoalg.mcount[phase]){
                    printf(LTOPO_ERR_TAG "%s failed, more branch files\n", __FUNCTION__);
                    goto err;
                    }
                strncpy(g_ltopoalg.mbox[phase][mcount].event_file, entry->d_name, FILENAME_LEN-1);
                g_ltopoalg.mbox[phase][mcount].event=build_event(entry->d_name);
                if(!g_ltopoalg.mbox[phase][mcount].event){
                    printf(LTOPO_ERR_TAG "%s build event failed, %s\n", __FUNCTION__, 
                                    g_ltopoalg.mbox[phase][mcount].event_file);
                    goto err;
                    }
                mcount++;
                }
            }
    } 
    
    closedir(dir);
    return 0;
err:
    if(dir)
        closedir(dir);
    ltopo_alg_clean(phase);
   return -1;
}
int print_ltopo_info()
{
    int i, j;
    printf(LTOPO_TAG "Enter %s\n", __FUNCTION__);
    printf(LTOPO_TAG "g_ltopoalg: path: %s\n", g_ltopoalg.event_path);
    for(i=0;i<LTOPO_TYPE_PHASE_MAX;i++){
        printf(LTOPO_TAG "branch phase %s, count %d\n", ltopo_phase_name[i], g_ltopoalg.bcount[i]);
        for(j=0;j<g_ltopoalg.bcount[i];j++){
            printf(LTOPO_TAG "####file:%s\n", g_ltopoalg.branch[i][j].event_file);
            ltopo_print_event_list(stdout, g_ltopoalg.branch[i][j].event);
            }
        printf(LTOPO_TAG "mbox phase %s, count %d\n", ltopo_phase_name[i], g_ltopoalg.mcount[i]);
        for(j=0;j<g_ltopoalg.mcount[i];j++){
            printf(LTOPO_TAG "####file:%s\n", g_ltopoalg.mbox[i][j].event_file);
            ltopo_print_event_list(stdout, g_ltopoalg.mbox[i][j].event);
            }
        }
    return 0;
}
int write_ltopo_info()
{
    int i, j;
    char fullname[128];
    FILE * fp;
    
    printf(LTOPO_TAG "Enter %s\n", __FUNCTION__);
    printf(LTOPO_TAG "g_ltopoalg: path: %s\n", g_ltopoalg.event_path);
    for(i=0;i<LTOPO_TYPE_PHASE_MAX;i++){
        printf(LTOPO_TAG "branch phase %s, count %d\n", ltopo_phase_name[i], g_ltopoalg.bcount[i]);
        for(j=0;j<g_ltopoalg.bcount[i];j++){
            printf(LTOPO_TAG "####file:%s\n", g_ltopoalg.branch[i][j].event_file);

            snprintf(fullname, sizeof(fullname)-1, "./tmp/%s", g_ltopoalg.branch[i][j].event_file);
            fp=fopen(fullname, "wb");
            if(!fp){
                printf(LTOPO_ERR_TAG "%s failed, open %s!!!\n", __FUNCTION__, fullname);
                return -1;;
                }
            ltopo_print_event_list(fp, g_ltopoalg.branch[i][j].event);
            fclose(fp);
            }
        printf(LTOPO_TAG "mbox phase %s, count %d\n", ltopo_phase_name[i], g_ltopoalg.mcount[i]);
        for(j=0;j<g_ltopoalg.mcount[i];j++){
            printf(LTOPO_TAG "####file:%s\n", g_ltopoalg.mbox[i][j].event_file);
            snprintf(fullname, sizeof(fullname)-1, "./tmp/%s", g_ltopoalg.mbox[i][j].event_file);
            fp=fopen(fullname, "wb");
            if(!fp){
                printf(LTOPO_ERR_TAG "%s failed, open %s!!!\n", __FUNCTION__, fullname);
                return -1;;
                }
            ltopo_print_event_list(fp, g_ltopoalg.mbox[i][j].event);
            fclose(fp);
            }
        }
    return 0;
}

int is_sline_in_the_future(int sline, LTOPO_EVENT* e)
{
    if(sline >= e->start && sline <= e->start+e->vcount) //sline in the event
        return 0;
    else if (sline < e->start) //event in the future
        return -1;
    else 
        return 1; //sline in the future
    
}
int minvalue_index_in_event(LTOPO_EVENT *e)
{
    int i, index=0;
    for(i=1;i<=e->vcount;i++){
        if(e->load[i].ap<e->load[index].ap)
            index=i;
        }
    return index;
}
int maxvalue_index_in_event(LTOPO_EVENT *e)
{
    int i, index=0;
    for(i=1;i<=e->vcount;i++){
        if(e->load[i].ap>e->load[index].ap)
            index=i;
        }
    return index;
}
int generate_jump_event(LTOPO_EVENT * ori, LTOPO_EVENT * gen, int vh, int vl, int wsize)
{
    int i, minindex, maxindex, /*direction, */offset;
    minindex=minvalue_index_in_event(ori);
    maxindex=maxvalue_index_in_event(ori);
    if(ori->load[maxindex].ap-ori->load[minindex].ap<vh){
        printf(LTOPO_ERR_TAG "jump event error: %d(max)-%d(min)=%d\n", ori->load[maxindex].ap, ori->load[minindex].ap,
                            ori->load[maxindex].ap-ori->load[minindex].ap);
        return -1;
        }
    if(minindex<maxindex){
        //direction=1;
        for(i=minindex;i<maxindex;i++)
            //找到第一个正向较大变化，开始记录
            if(ori->load[i+1].ap-ori->load[i].ap>vl)
                break;
        if(i==maxindex){//没找到
            printf(LTOPO_ERR_TAG "jump event error: all value smooth\n");
            return -1;
            }
        offset=i; //从offset开始记录
        /*
        1000, 1001, 10002, 1200, 2000, 2000, 
        */
        gen->type=LTOPO_EVENT_TYPE_JUMP;
        gen->start=ori->start+i;
        
        for(i=minindex;i<maxindex;i++)
            //已经是有效跳变了，并且下一个值减去当前值开始平滑或者转向
            //这个点就是记录的结束点
            if(ori->load[i+1].ap-ori->load[minindex].ap>vh && 
                        ori->load[i+1].ap-ori->load[i].ap<vl)
                break;
        gen->vcount=i-offset;
        for(i=0;i<=gen->vcount;i++){
            gen->load[i].ap=ori->load[i+offset].ap;
            }
        printf(LTOPO_TAG "got jump event\n");
        }
    if(minindex>maxindex){
        //direction=-1;
        for(i=maxindex;i<minindex;i++)
            //找到第一个正向较大变化，开始记录
            //23000, 21550, 20100, 10100, 100
            if(ori->load[i].ap-ori->load[i+1].ap>vl)
                break;
        if(i==minindex){//没找到
            printf(LTOPO_ERR_TAG "jump event error: all value smooth\n");
            return -1;
            }
        offset=i; //从offset开始记录
        /*
        1000, 1001, 10002, 1200, 2000, 2000, 
        */
        gen->type=LTOPO_EVENT_TYPE_JUMP;
        gen->start=ori->start+i;
        
        for(i=maxindex;i<minindex;i++)
            //已经是有效跳变了，并且下一个值减去当前值开始平滑或者转向
            //这个点就是记录的结束点
            if(ori->load[maxindex].ap-ori->load[i+1].ap>vh && 
                        ori->load[i].ap-ori->load[i+1].ap<vl)
                break;
        gen->vcount=i-offset;
        for(i=0;i<=gen->vcount;i++){
            gen->load[i].ap=ori->load[i+offset].ap;
            }
        printf(LTOPO_TAG "got jump event\n");
        }

    return 0;
    
}

int ltopo_scan_meter_event(METER_INFO * meter, int sline, int vh, int vl, int wsize)
{
    int ret;
    LTOPO_EVENT * p;
    if(meter->cur.start==0 && meter->cur.vcount==0){
        printf(LTOPO_TAG "First time\n");
        }
    else{
        //扫描线在当前计算出的event中，不做操作
        if(is_sline_in_the_future(sline, &meter->cur)==0) 
            return 0;
        //已经生成了当前event，但还未扫到该event，不做操作，下次扫描再处理
        if(is_sline_in_the_future(sline, &meter->cur)<0)
            return 0;
        }
    //当前event过时了
    if(meter->ptr==NULL)
        p=meter->event;
    else
        p=meter->ptr;
    //event遍历，找到扫描线在其中的event
    while(p){
        ret=is_sline_in_the_future(sline, p);
        if(ret==0) //找到扫描线在其中的event，break后处理
            break;
        else if(ret>0) //扫描线在本event的将来，event遍历到下一个
            p=p->next;
        else //扫描线在本event的过去，即未找到本扫描线在其中的event，跳过本扫描线
            return -1;
        }
    if(!p){
        printf(LTOPO_TAG "scan finished\n");
        return -1;
        }
        
    meter->ptr=p; //记录本event，以便于下次遍历
    if(p->type==LTOPO_EVENT_TYPE_SMOOTH){ //平滑波动事件，直接复制
        meter->cur.start=p->start;
        meter->cur.vcount=p->vcount;
        meter->cur.type=p->type;
        meter->cur.load[0].ap=p->load[0].ap;
        printf(LTOPO_TAG "got smooth event\n");
        }
    else if(p->type==LTOPO_EVENT_TYPE_JUMP){//跳变，需要生成跳变event
        //找到最大值
        //找到最小值，
        //最大值减去最小值 > VH
        //构建新的event，从最小值开始，到最大值结束
        generate_jump_event(p, &meter->cur, vh, vl, wsize);
        
        
        }
        
    return 0;
}
int ltopo_scan(int phase, int wsize)
{
    int i, j, start, end, mcycle, vh, vl;
    int mjumptimes=0, msmoothtimes=0;
    
    start=g_ltopoalg.start;
    end=g_ltopoalg.end;
    mcycle=g_ltopoalg.mcycle;
    vh=g_ltopoalg.vh;
    vl=g_ltopoalg.vl;

    for(i=start;i<=end;i+=mcycle){
        for(j=0;j<g_ltopoalg.mcount[phase];j++){
            printf(LTOPO_TAG "scan %s, line %d\n", g_ltopoalg.mbox[phase][j].event_file, i);
            ltopo_scan_meter_event(&g_ltopoalg.mbox[phase][j], i, vh, vl, wsize);
            }
        
        for(j=0;j<g_ltopoalg.bcount[phase];j++){
            printf(LTOPO_TAG "scan %s, line %d\n", g_ltopoalg.branch[phase][j].event_file, i);
            ltopo_scan_meter_event(&g_ltopoalg.branch[phase][j], i, vh, vl, wsize);
            }
        mjumptimes=0;
        msmoothtimes=0;
        for(j=0;j<g_ltopoalg.mcount[phase];j++){
            printf(LTOPO_TAG "scan %s, line %d\n", g_ltopoalg.mbox[phase][j].event_file, i);
            ltopo_print_event(&g_ltopoalg.mbox[phase][j].cur);
            if(is_sline_in_the_future(i, &g_ltopoalg.mbox[phase][j].cur)==0 && 
                                    g_ltopoalg.mbox[phase][j].cur.type==LTOPO_EVENT_TYPE_JUMP)
                mjumptimes++;
        
            if(is_sline_in_the_future(i, &g_ltopoalg.mbox[phase][j].cur)==0 && 
                                    g_ltopoalg.mbox[phase][j].cur.type==LTOPO_EVENT_TYPE_SMOOTH)
                msmoothtimes++;
            }
        printf(LTOPO_TAG "jump times %d, smooth times %d\n", mjumptimes, msmoothtimes);
        printf(LTOPO_TAG "\n");
        //sleep(10);
        }
    return 0;
}
#endif //M1

///////////////////////////////////
//从TTU的事件记录文件中，恢复负荷数据，只恢复有功功率
LTOPO_LOAD * ltopo_alg_restore_load(char * filename)
{
    int i, count=0, offset, delta; //ret;
    char fullname[128];
    LTOPO_LOAD * l;
    LTOPO_EVENT event;
    FILE * fp=NULL;
    printf(LTOPO_TAG "enter %s, %s\n", __FUNCTION__, filename);
    snprintf(fullname, sizeof(fullname)-1, "%s/%s", g_ltopoalg.event_path, filename);
    count=(g_ltopoalg.end-g_ltopoalg.start)/g_ltopoalg.mcycle+1;
    l=(LTOPO_LOAD * )malloc(count*sizeof(LTOPO_LOAD));
    if(!l){
        printf(LTOPO_TAG "%s, malloc load failed, %d\n", __FUNCTION__, count);
        return NULL;
        }
    //全部初始化为中间值
    for(i=0;i<count;i++){
        l[i].ap=LTOPO_DATA_VALUE_MIDDLE;
        l[i].v=LTOPO_DATA_VALUE_MIDDLE;
        }
    fp=fopen(fullname, "rb");
    if(!fp){
        printf(LTOPO_ERR_TAG "%s failed, open %s!!!\n", __FUNCTION__, fullname);
        goto err;
        }
    while(fread (&event, sizeof (LTOPO_EVENT), 1, fp)>0)
    {
        if(event.vcount==0){
            printf(LTOPO_ERR_TAG "%s, got event value count 0\n", __FUNCTION__);
            continue;
            }
        //##需要检查offset合法性，所有数组都要检查下标合法性
        offset=(event.start-g_ltopoalg.start)/g_ltopoalg.mcycle;
        if(event.type==LTOPO_EVENT_TYPE_SMOOTH){
            //smooth数据按照线性恢复
            delta=(event.load[1].ap-event.load[0].ap)/event.vcount;
            for(i=0;i<=event.vcount;i++){
                //if(offset+i<0 || offset+i>count)
                    //printf(LTOPO_TAG "1\n");
                //BEYOND_CONTINUE(offset+i, 0, count);
                BEYOND_CONTINUE(offset, i, 0, count);
                l[offset+i].ap=event.load[0].ap+i*delta;
                }
            }
        else if(event.type==LTOPO_EVENT_TYPE_JUMP){
            //jump数据直接赋值
            for(i=0;i<=event.vcount;i++){
                //BEYOND_CONTINUE(offset+i, 0, count);
                BEYOND_CONTINUE(offset, i, 0, count);
                l[offset+i].ap=event.load[i].ap;
                l[offset+i].v=event.load[i].v;
                }
            }
        else if(event.type==LTOPO_EVENT_NO_DATA){
            printf(LTOPO_TAG "%s, missing data, from %d to %d\n", __FUNCTION__, event.start, event.start+event.vcount*g_ltopoalg.mcycle);
            for(i=0;i<=event.vcount;i++){
                //BEYOND_CONTINUE(offset+i, 0, count);
                BEYOND_CONTINUE(offset, i, 0, count);
                l[offset+i].ap=LTOPO_DATA_VALUE_MISSING;
                }
            }
    }
    fclose(fp);
    return l;
err:
    if(l)
        free(l);
    if(fp)
        fclose(fp);
    return NULL;
}

//把负荷按相位恢复到文本文件中，以便与表箱终端的原始数据进行对比
//只是调试用
int ltopo_alg_debug_save_load(int phase)
{
    int i, j, k, count, total_middle=0, total_missing=0;
    int missing_s=-1, missing_e=-1, middle_s=-1, middle_e=-1;
    char fullname[128];
    FILE * fp;
    printf(LTOPO_TAG "Enter %s, path: %s\n", __FUNCTION__, g_ltopoalg.event_path);
    i=phase;
    printf(LTOPO_TAG "branch phase %s, count %d\n", ltopo_phase_name[i], g_ltopoalg.bcount[i]);
    count=(g_ltopoalg.end-g_ltopoalg.start)/g_ltopoalg.mcycle+1;
    for(j=0;j<g_ltopoalg.bcount[i];j++){
        missing_s=missing_e=middle_s=middle_e=-1;
        //printf(LTOPO_TAG "####file:%s\n", g_ltopoalg.branch[i][j].event_file);
        snprintf(fullname, sizeof(fullname)-1, "%s/%s", LTOPO_RESTORE_LOAD_PATH, g_ltopoalg.branch[i][j].event_file);
        fp=fopen(fullname, "wb");
        if(!fp){
            printf(LTOPO_ERR_TAG "%s failed, open %s!!!\n", __FUNCTION__, fullname);
            return -1;
            }
        //ltopo_print_event_list(stdout, g_ltopoalg.branch[i][j].event);
        for(k=0;k<count;k++){
            fprintf(fp, "%d:%d\n", (int)g_ltopoalg.start+k*g_ltopoalg.mcycle,
                g_ltopoalg.branch[i][j].load[k].ap);

            //打印丢失的数据
            if(g_ltopoalg.branch[i][j].load[k].ap==LTOPO_DATA_VALUE_MISSING){
                g_ltopoalg.branch[i][j].s.missing_count++; 
                if(missing_s==-1){
                    missing_s=g_ltopoalg.start+k*g_ltopoalg.mcycle;
                    missing_e=missing_s;
                    }
                else
                    missing_e=g_ltopoalg.start+k*g_ltopoalg.mcycle;
                }
            else if(missing_s!=-1){
                printf(LTOPO_TAG "%s, missing from %d to %d\n", g_ltopoalg.branch[i][j].event_file, missing_s, missing_e);
                missing_s=-1;
                missing_e=-1;
                }
            //打印中间数据
            if(g_ltopoalg.branch[i][j].load[k].ap==LTOPO_DATA_VALUE_MIDDLE){
                g_ltopoalg.branch[i][j].s.middle_count++;
                if(middle_s==-1){
                    middle_s=g_ltopoalg.start+k*g_ltopoalg.mcycle;
                    middle_e=middle_s;
                    }
                else
                    middle_e=g_ltopoalg.start+k*g_ltopoalg.mcycle;
                }
            else  if(middle_s!=-1){
                printf(LTOPO_TAG "%s, middle from %d to %d\n", g_ltopoalg.branch[i][j].event_file, middle_s, middle_e);
                middle_s=-1;
                middle_e=-1;
                }
            }
        fclose(fp);
        if(middle_s!=-1){
            printf(LTOPO_TAG "%s, middle from %d to %d\n", g_ltopoalg.branch[i][j].event_file,  middle_s, middle_e);
            }
        if(missing_s!=-1){
            printf(LTOPO_TAG "%s, missing from %d to %d\n", g_ltopoalg.branch[i][j].event_file, missing_s, missing_e);
            }
        total_middle+=g_ltopoalg.branch[i][j].s.middle_count;
        total_missing+=g_ltopoalg.branch[i][j].s.missing_count;
        }
    printf(LTOPO_TAG "mbox phase %s, count %d\n", ltopo_phase_name[i], g_ltopoalg.mcount[i]);
    for(j=0;j<g_ltopoalg.mcount[i];j++){
        missing_s=missing_e=middle_s=middle_e=-1;
        //printf(LTOPO_TAG "####file:%s\n", g_ltopoalg.mbox[i][j].event_file);
        //ltopo_print_event_list(stdout, g_ltopoalg.mbox[i][j].event);
        snprintf(fullname, sizeof(fullname)-1, "%s/%s", LTOPO_RESTORE_LOAD_PATH, g_ltopoalg.mbox[i][j].event_file);
        fp=fopen(fullname, "wb");
        if(!fp){
            printf(LTOPO_ERR_TAG "%s failed, open %s!!!\n", __FUNCTION__, fullname);
            return -1;;
            }
        for(k=0;k<count;k++){
            fprintf(fp, "%d:%d\n", (int)g_ltopoalg.start+k*g_ltopoalg.mcycle,
                g_ltopoalg.mbox[i][j].load[k].ap);

            //打印丢失的数据
            if(g_ltopoalg.mbox[i][j].load[k].ap==LTOPO_DATA_VALUE_MISSING){
                g_ltopoalg.mbox[i][j].s.missing_count++; 
                if(missing_s==-1){
                    missing_s=g_ltopoalg.start+k*g_ltopoalg.mcycle;
                    missing_e=missing_s;
                    }
                else
                    missing_e=g_ltopoalg.start+k*g_ltopoalg.mcycle;
                }
            else  if(missing_s!=-1){
                printf(LTOPO_TAG "%s, missing from %d to %d\n", g_ltopoalg.mbox[i][j].event_file, missing_s, missing_e);
                missing_s=-1;
                missing_e=-1;
                }
            //打印中间数据
            if(g_ltopoalg.mbox[i][j].load[k].ap==LTOPO_DATA_VALUE_MIDDLE){
                g_ltopoalg.mbox[i][j].s.middle_count++;
                if(middle_s==-1){
                    middle_s=g_ltopoalg.start+k*g_ltopoalg.mcycle;
                    middle_e=middle_s;
                    }
                else
                    middle_e=g_ltopoalg.start+k*g_ltopoalg.mcycle;
                }
            else  if(middle_s!=-1){
                printf(LTOPO_TAG "%s, middle from %d to %d\n", g_ltopoalg.mbox[i][j].event_file, middle_s, middle_e);
                middle_s=-1;
                middle_e=-1;
                }
            }
        fclose(fp);
        if(middle_s!=-1)
            printf(LTOPO_TAG "%s, middle from %d to %d\n", g_ltopoalg.mbox[i][j].event_file, middle_s, middle_e);
        if(missing_s!=-1)
            printf(LTOPO_TAG "%s, missing from %d to %d\n", g_ltopoalg.mbox[i][j].event_file, missing_s, missing_e);
        total_middle+=g_ltopoalg.mbox[i][j].s.middle_count;
        total_missing+=g_ltopoalg.mbox[i][j].s.missing_count;
        }

    printf(LTOPO_TAG "%s, bcount %d, mcount %d, count %d\n", __FUNCTION__, 
                    g_ltopoalg.bcount[i], g_ltopoalg.mcount[i], count);
    printf(LTOPO_TAG "%s, total data %d, missing %d, middle %d\n", __FUNCTION__, 
                    (g_ltopoalg.bcount[i]+g_ltopoalg.mcount[i])*count,
                    total_missing, total_middle);
    return 0;
}

//从ttu配置过来的目录中，读取事件数据
int ltopo_alg_read_path(int phase)
{
    int bcount=0, mcount=0; // bcount: phase 相位的branch个数, mcount: phase相位的meter box个数
    DIR *dir = NULL;  
    struct dirent *entry;  

    if(phase<0 || phase>=LTOPO_TYPE_PHASE_MAX){
        printf(LTOPO_ERR_TAG "__FUNCTION__: invalid phase %d\n", phase);
        return -1;
        }

    printf(LTOPO_TAG "%s, phase %s, path %s\n", __FUNCTION__, ltopo_phase_name[phase], g_ltopoalg.event_path);

    if((dir = opendir(g_ltopoalg.event_path))==NULL)   {  
        printf(LTOPO_ERR_TAG "opendir failed!");  
        return -1;  
        }
    while((entry=readdir(dir)))  {
        if(entry->d_type==8){
            if(!strncmp(entry->d_name, ltopo_bp_prefix[phase], 2))
                bcount++;
            else if(!strncmp(entry->d_name, ltopo_mp_prefix[phase], 2))
                mcount++;
            }
        }
    closedir(dir);
    dir=NULL;
    if(mcount==0 || bcount==0){
        printf(LTOPO_WARN_TAG "%s failed: no data file, phase %s\n", __FUNCTION__, ltopo_phase_name[phase]);
        return -1;
        }
    if(verify_mcount_bcount(mcount, bcount)!=0){
        printf(LTOPO_ERR_TAG "mcount & bcount verification failed!\n"); 
        printf(LTOPO_ERR_TAG "mcount %d, bcount %d\n", mcount, bcount); 
        return -1;
        }
    g_ltopoalg.mbox[phase]=(METER_INFO *)malloc(mcount*sizeof(METER_INFO));
    g_ltopoalg.branch[phase]=(METER_INFO *)malloc(bcount*sizeof(METER_INFO));
    if(!g_ltopoalg.mbox[phase] || !g_ltopoalg.branch[phase]){
        printf(LTOPO_ERR_TAG "g_ltopoalg malloc pmbox(%d) or pbranch(%d) for %s failed\n", 
                            mcount, bcount, ltopo_phase_name[phase]);
        goto err;
        }
    memset(g_ltopoalg.mbox[phase], 0x0, mcount*sizeof(METER_INFO));
    memset(g_ltopoalg.branch[phase], 0x0, bcount*sizeof(METER_INFO));
    g_ltopoalg.bcount[phase]=bcount;
    g_ltopoalg.mcount[phase]=mcount;
    if((dir = opendir(g_ltopoalg.event_path))==NULL)   {  
        printf(LTOPO_TAG "opendir failed!");  
        goto err;  
        }
    bcount=0; 
    mcount=0; 
    while((entry=readdir(dir)))  {
        if(entry->d_type==8){
            if(!strncmp(entry->d_name, ltopo_bp_prefix[phase], 2)){
                if(bcount>=g_ltopoalg.bcount[phase]){
                    printf(LTOPO_ERR_TAG "%s failed, more branch files\n", __FUNCTION__);
                    goto err;
                    }
                strncpy(g_ltopoalg.branch[phase][bcount].event_file, entry->d_name, FILENAME_LEN-1);
//                strncpy(g_ltopoalg.branch[phase][bcount].addr, &entry->d_name[3], LTOPO_ADDR_LEN-1);
                strncpy(g_ltopoalg.branch[phase][bcount].addr, &entry->d_name[3], 12);

                g_ltopoalg.branch[phase][bcount].load=ltopo_alg_restore_load(entry->d_name);//
                if(!g_ltopoalg.branch[phase][bcount].load){
                    printf(LTOPO_ERR_TAG "%s ltopo_alg_restore_load failed, %s\n", __FUNCTION__, 
                                    g_ltopoalg.branch[phase][bcount].event_file);
                    goto err;
                    }
                g_ltopoalg.branch[phase][bcount].pm=ltopo_get_meter(g_ltopoalg.branch[phase][bcount].addr, &g_ltopo.meters.list);
                if(!g_ltopoalg.branch[phase][bcount].pm){
                    printf(LTOPO_ERR_TAG "%s ltopo_get_meter failed, %s\n", __FUNCTION__, 
                                    g_ltopoalg.branch[phase][bcount].addr);
                    goto err;
                    }
                bcount++;
                }
            else if(!strncmp(entry->d_name, ltopo_mp_prefix[phase], 2)){
                if(mcount>=g_ltopoalg.mcount[phase]){
                    printf(LTOPO_ERR_TAG "%s failed, more branch files\n", __FUNCTION__);
                    goto err;
                    }
                strncpy(g_ltopoalg.mbox[phase][mcount].event_file, entry->d_name, FILENAME_LEN-1);
//                strncpy(g_ltopoalg.mbox[phase][mcount].addr, &entry->d_name[3], LTOPO_ADDR_LEN-1);
                strncpy(g_ltopoalg.mbox[phase][mcount].addr, &entry->d_name[3], 12);

                g_ltopoalg.mbox[phase][mcount].load=ltopo_alg_restore_load(entry->d_name);
                if(!g_ltopoalg.mbox[phase][mcount].load){
                    printf(LTOPO_ERR_TAG "%s ltopo_alg_restore_load failed, %s\n", __FUNCTION__, 
                                    g_ltopoalg.mbox[phase][mcount].event_file);
                    goto err;
                    }
                g_ltopoalg.mbox[phase][mcount].pm=ltopo_get_meter(g_ltopoalg.mbox[phase][mcount].addr, &g_ltopo.meters.list);
                if(!g_ltopoalg.mbox[phase][mcount].pm){
                    printf(LTOPO_ERR_TAG "%s ltopo_get_meter failed, %s\n", __FUNCTION__, 
                                    g_ltopoalg.mbox[phase][mcount].addr);
                    goto err;
                    }
                    
                mcount++;
                }
            }
    } 
    
    closedir(dir);
    return 0;
err:
    if(dir)
        closedir(dir);
    ltopo_alg_clean(phase);
   return -1;
}
//窗口是否平滑
int is_window_smooth(LTOPO_LOAD * w, int wsize, int vl)
{
    int i;
    for(i=1;i<=wsize;i++){
        if(w[i].ap-w[i-1].ap>vl || w[i-1].ap-w[i].ap>vl)
            return 0;
        }
    return 1;
}
//最小值的index
int minvalue_index_in_load(LTOPO_LOAD * e, int count)
{
    int i, index=0;
    for(i=1;i<=count;i++){
        if(e[i].ap<e[index].ap)
            index=i;
        }
    return index;
}
//最小正值的index
int minvalue_positive_index_in_load(LTOPO_LOAD * e, int count)
{
    int i, index;
    for(i=0;i<=count;i++){ //找一个大于零的做初值，考虑到功率为零的情况，增加等于0
        if(e[i].ap>=0){
            index=i;
            break;
            }
        }
    for(i=0;i<=count;i++){
        if(e[i].ap>=0 && e[i].ap<=e[index].ap)
            index=i;
        
        }
    printf(LTOPO_TAG "index %d, value %d\n", index, e[index].ap);
    return index;
}
//最大值的index
int maxvalue_index_in_load(LTOPO_LOAD * e, int count)
{
    int i, index=0;
    for(i=1;i<=count;i++){
        if(e[i].ap>e[index].ap)
            index=i;
        }
    return index;
}
//窗口是否有脉冲
int is_window_pulse(LTOPO_LOAD *w, int wsize)
{
	int i;
	int n=0;
	int d_value;
	for(i=0;i<wsize;i++)
		{
			d_value=w[i].ap-w[i-1].ap;
			
			if(d_value>0)
				{
					if(d_value>LTOPO_UP_THRESHOLD_VALUE)
						{
							while(d_value>LTOPO_UPE_THRESHOLD_VALUE)
								{
									n++;
									d_value=w[i+n].ap-w[i+n-1].ap;
									if(n+1==wsize)
										return 2;
								}
							return 1;
						}
				}
			else
				{
					if(d_value<LTOPO_DOWN_THRESHOLD_VALUE)
						{
							while(d_value<LTOPO_DOWNE_THRESHOLD_VALUE)
								{
									n++;
									d_value=w[i+n].ap-w[i+n-1].ap;
									if(n+1==wsize)
										return 2;
								}
							return 1;
						}
				}
		}
	return 0;
}
//窗口是否有跳变
int is_window_jump(LTOPO_LOAD * w, int wsize, int vh)
{
    int i, count=0, minindex, maxindex;
    for(i=0;i<=wsize;i++)
        if(w[i].ap<0)
            count++;
    if(count>1) //窗口里有超过两个小于零的值，则不做jump判断
        return 0;
    if(count==0)//窗口里全是大于零的值，正常处理
        minindex=minvalue_index_in_load(w, wsize);
    else if(w[0].ap<0) //窗口第一个值小于零
        minindex=minvalue_index_in_load(&w[1], wsize-1)+1;
    else if(w[wsize].ap<0) //窗口最后一个值小于零
        minindex=minvalue_index_in_load(w, wsize-1);
    else //窗口中间有一个小于零的值，不做jump判断
        return 0;
    
    maxindex=maxvalue_index_in_load(w, wsize);
        
    if(w[maxindex].ap-w[minindex].ap>=vh)
        return 1;
    return 0;
}
//窗口是否丢数据
int is_window_nodata(LTOPO_LOAD * w, int wsize)
{
    int i;
    for(i=0;i<=wsize;i++){
        if(w[i].ap==LTOPO_DATA_VALUE_MISSING)
            return 1;
        }
    return 0;
}

int is_window_middle(LTOPO_LOAD * w, int wsize)
{
    int i;
    for(i=0;i<=wsize;i++){
        if(w[i].ap==LTOPO_DATA_VALUE_MIDDLE)
            return 1;
        }
    return 0;
}
int ltopo_alg_print_meter_window(METER_INFO * meter, int sline, int wsize, int msize, int vh, int vl)
{
    LTOPO_LOAD *m;
    m=&meter->load[sline];
    
    int i;
    if(meter->pm && meter->pm->minfo.mtype==LTOPO_METER_TYPE_MBOX)
        printf(LTOPO_TAG "%s [%d %d]: ", meter->event_file, meter->excluded, meter->iwstatus==LTOPO_EVENT_TYPE_JUMP?1:0);
    else
        printf(LTOPO_TAG "%s [%d %d]: ", meter->event_file, meter->excluded, 0);
        
    for(i=0;i<=msize;i++){
        if(i==LTOPO_WINDOW_MARGIN_SIZE)
            printf("[ ");
        else if(i==msize-(LTOPO_WINDOW_MARGIN_SIZE-1))
            printf("] ");
        printf("%d ", m[i].ap);
        }
    printf("\n");
    return 0;
}
int print_alg_print_all_window(int phase, int sline, int wsize, int msize, int vh, int vl)
{
    METER_INFO * meter;
    int j;
    
    for(j=0;j<g_ltopoalg.mcount[phase];j++){
        meter=&g_ltopoalg.mbox[phase][j];
        ltopo_alg_print_meter_window(meter, sline, wsize, msize, vh, vl);
        }
    for(j=0;j<g_ltopoalg.bcount[phase];j++){
        meter=&g_ltopoalg.branch[phase][j];
        ltopo_alg_print_meter_window(meter, sline, wsize, msize, vh, vl);
        }
    return 0;
}
//扫描窗口
int ltopo_alg_scan_window(METER_INFO * meter, int sline, int wsize, int msize, int vh, int vl)
{
    int wleft;
    LTOPO_LOAD * w, *m;
    m=&meter->load[sline];
    wleft=sline+((msize-wsize)>>1);
    w=&meter->load[wleft];
    
#ifdef ALG_DEBUG 
    int i;
    for(i=0;i<=msize;i++)
        printf(LTOPO_TAG "%d ", m[i].ap);
    printf(LTOPO_TAG " -->");
#endif
    //如果本窗口有丢数据，放弃
    if(is_window_nodata(w,wsize)){
#ifdef ALG_DEBUG            
        printf(LTOPO_TAG "no data ");
#endif
        meter->iwstatus=LTOPO_EVENT_NO_DATA;
        }
    //如果本窗口有跳变，先处理，判断jump时需要先剔除LTOPO_DATA_MIDDLE -1的值
    else if(is_window_jump(w, wsize, vh)){
#ifdef ALG_DEBUG            
        printf(LTOPO_TAG "jump ");
#endif
        meter->iwstatus=LTOPO_EVENT_TYPE_JUMP;
        }
    else if(is_window_middle(w,wsize)){
#ifdef ALG_DEBUG            
        printf(LTOPO_TAG "middle ");
#endif
        meter->iwstatus=LTOPO_EVENT_TYPE_MIDDLE;
        }
    else if(is_window_smooth(w, wsize, vl)){
#ifdef ALG_DEBUG            
        printf(LTOPO_TAG "smooth ");
#endif
        meter->iwstatus=LTOPO_EVENT_TYPE_SMOOTH;
        }
    else{
#ifdef ALG_DEBUG            
        printf(LTOPO_TAG "middle2 ");
#endif
        meter->iwstatus=LTOPO_EVENT_TYPE_MIDDLE;
        }

    
    if(is_window_nodata(m,msize)){
#ifdef ALG_DEBUG            
        printf(LTOPO_TAG "no data \n");
#endif
        meter->owstatus=LTOPO_EVENT_NO_DATA;
        }
    else if(is_window_jump(m, msize, vh)){
#ifdef ALG_DEBUG            
        printf(LTOPO_TAG "jump \n");
#endif
        meter->owstatus=LTOPO_EVENT_TYPE_JUMP;
        }
    else if(is_window_middle(m,msize)){
#ifdef ALG_DEBUG            
        printf(LTOPO_TAG "middle \n");
#endif
        meter->owstatus=LTOPO_EVENT_TYPE_MIDDLE;
        }
    else if(is_window_smooth(m, msize, vl)){
#ifdef ALG_DEBUG            
        printf(LTOPO_TAG "smooth \n");
#endif
        meter->owstatus=LTOPO_EVENT_TYPE_SMOOTH;
        }
    else{
#ifdef ALG_DEBUG            
        printf(LTOPO_TAG "middle2 \n");
#endif
        meter->owstatus=LTOPO_EVENT_TYPE_MIDDLE;
        }
    meter->next_needed=0;
    if(meter->iwstatus==LTOPO_EVENT_TYPE_JUMP) {//内窗跳变
        if(w[1].ap-w[0].ap<LTOPO_MAYBE_JUMP_COUNT*vl || w[0].ap-w[1].ap<LTOPO_MAYBE_JUMP_COUNT*vl){//内窗口初始无跳变
            if(w[wsize+1].ap-w[wsize].ap>LTOPO_MAYBE_JUMP_COUNT*vl || w[wsize].ap-w[wsize+1].ap>LTOPO_MAYBE_JUMP_COUNT*vl)//内窗口外有跳变
            meter->next_needed=1;
            }
        }
    
    return 0;
}

//生成跳变的特征值，包括方向，跳变值，启动时点， 跳变时长，
//特征值: eigenvalue-->ev
int ltopo_alg_gen_ev(int sline, LTOPO_LOAD * w, int w_size, LTOPO_JUMP_EV_INFO * mbev, int vl)
{
    int i, minindex, maxindex;
    int start=-1, end=-1;
    int wsize=w_size+LTOPO_BRANCH_WINDOW_ADD;
    int pulse;
    /*
    1确定方向，+还是-
    2找到启动时间点
    3找到结束点，计算跳变值
    4计算时间差
    5判断是否脉冲
    */
    
printf(LTOPO_TAG "%s, ", __FUNCTION__);
for(i=0;i<=wsize;i++)
    printf("%d ", w[i].ap);
printf("\n");
    minindex=minvalue_positive_index_in_load(w, wsize);
    maxindex=maxvalue_index_in_load(w, wsize);
    
    mbev->direction=maxindex>minindex?LTOPO_DIRECTION_UP:LTOPO_DIRECTION_DOWN;
    mbev->minv=w[minindex].ap;
    mbev->maxv=w[maxindex].ap;
    mbev->value=mbev->maxv-mbev->minv;
	mbev->pulse=is_window_pulse(w, wsize);
	
    if(mbev->direction==LTOPO_DIRECTION_UP){
        for(i=minindex;i<maxindex;i++){
//            if(w[i].ap>=0 && w[i+1].ap-w[i].ap>g_ltopoalg.vh){
            if(w[i].ap>=0 && w[i+1].ap-w[i].ap>3*vl){

                start=i;
                break;
                }
            if(w[i].ap>=0 && w[i+1].ap-w[i].ap>vl){
                /*0,250,254,254,268*/
                if(i+2<=maxindex && w[i+2].ap-w[i+1].ap<vl)
                    continue;
                start=i;
                break;
                }
            }
        for(i=maxindex;i>minindex;i--){
            if(w[i-1].ap>=0 && w[i].ap-w[i-1].ap>vl){
                end=i;
                break;
                }
            }
        if(start==-1 || end==-1 || start>=end){
            printf(LTOPO_ERR_TAG "%s, start end failure %d %d\n", __FUNCTION__, start, end);
            return -1;
            }
        mbev->start=g_ltopoalg.start+sline+start; // 总起点+窗口起点+窗口内的位置
        mbev->period=end-start;
        mbev->value=w[end].ap-w[start].ap;
        }
    else{
        for(i=maxindex;i<minindex;i++){
//            if(w[i+1].ap>=0 && w[i].ap-w[i+1].ap>g_ltopoalg.vh){
            if(w[i+1].ap>=0 && w[i].ap-w[i+1].ap>3*vl){
                start=i;
                break;
                }
            if(w[i+1].ap>=0 && w[i].ap-w[i+1].ap>vl){
                if(i+2<=minindex && w[i+1].ap-w[i+2].ap<vl)
                    continue;
                start=i;
                break;
                }
            }
        for(i=minindex;i>maxindex;i--){
            if(w[i].ap>=0 && w[i-1].ap-w[i].ap>vl){
                end=i;
                break;
                }
            }
        if(start>=end){
            printf(LTOPO_ERR_TAG "%s, down start end failure %d %d\n", __FUNCTION__, start, end);
            return -1;
            }
        mbev->start=g_ltopoalg.start+sline+start; // 总起点+窗口起点+窗口内的位置
        mbev->period=end-start;
        mbev->value=w[start].ap-w[end].ap;
        }
    printf(LTOPO_TAG "direction %d, start %d, value %d, period %d,pulse %d\n", mbev->direction, mbev->start, mbev->value, mbev->period,mbev->pulse);
    printf(LTOPO_TAG "max %d, min %d\n", mbev->maxv, mbev->minv);
    if(mbev->value>50000){
        printf("##** %s, %s, %d\n##** power: ", mbev->type==LTOPO_METER_TYPE_MBOX?"mbox":"branch", mbev->addr, mbev->start);
        for(i=0;i<=wsize;i++)
            printf("%d ", w[i].ap);
        printf("\n");
        printf("##** voltage: ", mbev->value);
        for(i=0;i<=wsize;i++)
            printf("%d ", w[i].v);
        printf("\n##**\n");
        }
        
        
    return 0;
}
static void ltopo_alg_ev_list_show(void *arg)
{
    METER_INFO * pmeter;
    LTOPO_LIST *q = (LTOPO_LIST *)arg;
    LTOPO_JUMP_EV_NODE  *p = container_of(q, LTOPO_JUMP_EV_NODE, list);
    if(p->ev.type==LTOPO_METER_TYPE_MBOX)
        pmeter=&g_ltopoalg.mbox[p->ev.phase][p->ev.index];
    else
        pmeter=&g_ltopoalg.branch[p->ev.phase][p->ev.index];
    
    printf(LTOPO_TAG "[EV list]\n");
    printf(LTOPO_TAG "addr %s, phase %s, type %d, index %d\n",
            pmeter->event_file, ltopo_phase_name[  p->ev.phase], p->ev.type, p->ev.index);
    printf(LTOPO_TAG "start %d, direction %d, value %d, minv %d, max %d, period %d,pulse %d\n", 
            p->ev.start, p->ev.direction, p->ev.value, p->ev.minv, p->ev.maxv, p->ev.period,p->ev.pulse);
    printf(LTOPO_TAG "addr %s, father %s\n", p->ev.addr, p->ev.father);
}

void ltopo_alg_ev_list_fini(void *arg)
{
        LTOPO_LIST *q = (LTOPO_LIST *)arg;
        LTOPO_JUMP_EV_NODE  *p = container_of(q, LTOPO_JUMP_EV_NODE, list);
/*
        LTOPO_MBOX_NODE *next_nodep = NULL;
        if (p->list.next != NULL)
                next_nodep = container_of(p->list.next, struct struct_LTOPO_MBOX_NODE, list);

        printf(LTOPO_TAG "%s, free (node) %p next (node) %p\n", __FUNCTION__, p, next_nodep);
*/
        p->list.next = NULL;
        free(p);
}

int ltopo_alg_merge_node(LTOPO_LIST * meter_head, LTOPO_JUMP_EV_NODE * node)
{
    LTOPO_LIST * p;
    LTOPO_METER_NODE * pmeter=NULL;
    printf(LTOPO_TAG "Enter %s\n", __FUNCTION__);
    
    list_for_each(p, meter_head){
        pmeter=container_of(p, LTOPO_METER_NODE, list);
        if(!strcmp(pmeter->minfo.addr, node->ev.addr)){ //找到跳变对应的表
            if(pmeter->minfo.mtype==LTOPO_METER_TYPE_TTU){ //如果对应的表是TTU，不处理
                printf(LTOPO_TAG "find root\n");
                return 0;
                }
            else if(!strcmp(pmeter->minfo.f_addr, "-1")){//如果表父节点未知，拷贝表的父节点
                printf(LTOPO_TAG "find it! merges\n");
                if(!strcmp(node->ev.father, "-1")){ //跳变的父节点是root，则复制root给表
                    printf(LTOPO_TAG "should attach %s to root\n", node->ev.addr);
                    if(!g_ltopo.root){
                        printf(LTOPO_ERR_TAG "%s, error, no root in gltopo\n", __FUNCTION__);
                        //strncpy(pmeter->minfo.f_addr, node->ev.father, LTOPO_ADDR_LEN);
                        }
                    else{
                        strncpy(pmeter->minfo.f_addr, g_ltopo.root->minfo.addr, LTOPO_ADDR_LEN);
                        pmeter->minfo.resultby=LTOPO_RESULT_BY_CAL;
                        }
                    
                    }
                else {//跳变的父节点不是root，则复制跳变的父节点给表
                    strncpy(pmeter->minfo.f_addr, node->ev.father, LTOPO_ADDR_LEN);
                    pmeter->minfo.resultby=LTOPO_RESULT_BY_CAL;
                    }
                return 0;
                }
            else if(!strcmp(pmeter->minfo.f_addr, node->ev.father)){ //表父节点已知，并且和跳变一致      
                printf(LTOPO_TAG "find it! exists\n");
                return 0;
                }
            else{ //表的父节点已知，并且和跳变不一致
                if(!g_ltopo.root){
                    printf(LTOPO_ERR_TAG "%s, error2, no root in gltopo\n", __FUNCTION__);
                    return 0;
                    }
                if(!strcmp(g_ltopo.root->minfo.addr, pmeter->minfo.f_addr) && !strcmp(node->ev.father, "-1"))
                    printf(LTOPO_TAG "find it, exits, the meter's father is root\n");
                else{
                    printf(LTOPO_TAG "find it! conflicts!\n");
                    }
                return 0;
                }
            }
        }
    return -1;
}
int ltopo_alg_merge(LTOPO_LIST * meter_head, LTOPO_LIST * ev_head)
{
    LTOPO_JUMP_EV_NODE *pnode;
    LTOPO_LIST *p;

    list_for_each(p, ev_head){
        pnode=container_of(p, LTOPO_JUMP_EV_NODE, list);
        if(ltopo_alg_merge_node(meter_head, pnode)!=0){
            printf(LTOPO_ERR_TAG "%s, ltopo_alg_merge_node failed\n", __FUNCTION__);
            return -1;
            }
        }
    return 0;
}
void ltopo_alg_sm_enter_confirming_state(LTOPO_METER_NODE * pmeter)
{
    printf(LTOPO_TAG "%s, %s: %d -> %d \n", __FUNCTION__, pmeter->minfo.addr, 
                    pmeter->minfo.sm_state, LTOPO_SM_STATE_CONFIRMING);
    pmeter->minfo.sm_state=LTOPO_SM_STATE_CONFIRMING;
    pmeter->minfo.sm_counter=0;
}
void ltopo_alg_sm_enter_confirmed_state(LTOPO_METER_NODE * pmeter)
{
    printf(LTOPO_TAG "%s, %s: %d -> %d \n", __FUNCTION__, pmeter->minfo.addr, 
                    pmeter->minfo.sm_state, LTOPO_SM_STATE_CONFIRMED);
    pmeter->minfo.sm_state=LTOPO_SM_STATE_CONFIRMED;
    pmeter->minfo.sm_counter=0;
}
void ltopo_alg_sm_enter_unknown_state(LTOPO_METER_NODE * pmeter)
{
    printf(LTOPO_TAG "%s, %s: %d -> %d \n", __FUNCTION__, pmeter->minfo.addr, 
                    pmeter->minfo.sm_state, LTOPO_SM_STATE_UNKNOWN);
    pmeter->minfo.sm_state=LTOPO_SM_STATE_UNKNOWN;
    pmeter->minfo.sm_counter=-1;
    strcpy(pmeter->minfo.f_addr, "-1");
    pmeter->minfo.f_id=-1;
    pmeter->minfo.level=-1;
    pmeter->minfo.resultby=-1;
}

int ltopo_alg_meter_sm_proc(LTOPO_METER_NODE * pmeter, LTOPO_JUMP_EV_NODE * node)
{

    printf(LTOPO_TAG "%s, addr %s father %s, status %d, counter %d\n", __FUNCTION__, 
                        pmeter->minfo.addr, pmeter->minfo.f_addr, pmeter->minfo.sm_state, pmeter->minfo.sm_counter);

    switch(pmeter->minfo.sm_state){
        case LTOPO_SM_STATE_UNKNOWN:
            strncpy(pmeter->minfo.f_addr, node->ev.father, LTOPO_ADDR_LEN);
            pmeter->minfo.resultby=LTOPO_RESULT_BY_CAL;
            ltopo_alg_sm_enter_confirming_state(pmeter);
            break;
        case LTOPO_SM_STATE_CONFIRMING:
            if(!strcmp(pmeter->minfo.f_addr, node->ev.father)){ //一致      
                pmeter->minfo.sm_counter++;
                if(pmeter->minfo.sm_counter>=LTOPO_NODE_SM_COUNTER_MAX)
                    ltopo_alg_sm_enter_confirmed_state(pmeter);
                }
            else{ //不一致
                pmeter->minfo.sm_counter--;
                if(pmeter->minfo.sm_counter<0)
                    ltopo_alg_sm_enter_unknown_state(pmeter);
                }            
            break;
        case LTOPO_SM_STATE_CONFIRMED:
            if(!strcmp(pmeter->minfo.f_addr, node->ev.father)){ //一致      
                if(pmeter->minfo.sm_counter>0)
                    pmeter->minfo.sm_counter--;
                }
            else{ //不一致
                pmeter->minfo.sm_counter++;
                if(pmeter->minfo.sm_counter>=LTOPO_NODE_SM_COUNTER_MAX)
                    ltopo_alg_sm_enter_unknown_state(pmeter);
                }            
            break;
        }
    return 0;
}
int ltopo_alg_merge_node2(LTOPO_LIST * meter_head, LTOPO_JUMP_EV_NODE * node)
{
    LTOPO_LIST * p;
    LTOPO_METER_NODE * pmeter=NULL;
    printf(LTOPO_TAG "Enter %s\n", __FUNCTION__);
    
    list_for_each(p, meter_head){
        pmeter=container_of(p, LTOPO_METER_NODE, list);
        if(!strcmp(pmeter->minfo.addr, node->ev.addr)){ //找到跳变对应的表
            if(pmeter->minfo.mtype==LTOPO_METER_TYPE_TTU){ //如果对应的表是TTU，不处理
                printf(LTOPO_TAG "find root\n");
                return 0;
                }
            else if(!strcmp(pmeter->minfo.f_addr, "-1")){//如果表父节点未知，拷贝表的父节点
                printf(LTOPO_TAG "find it! merges\n");
                if(!strcmp(node->ev.father, "-1")){ //跳变的父节点是root，则复制root给表
                    printf(LTOPO_TAG "should attach %s to root\n", node->ev.addr);
                    if(!g_ltopo.root){
                        printf(LTOPO_ERR_TAG "%s, error, no root in gltopo\n", __FUNCTION__);
                        //strncpy(pmeter->minfo.f_addr, node->ev.father, LTOPO_ADDR_LEN);
                        }
                    else{
                        strncpy(pmeter->minfo.f_addr, g_ltopo.root->minfo.addr, LTOPO_ADDR_LEN);
                        pmeter->minfo.resultby=LTOPO_RESULT_BY_CAL;
                        }
                    
                    }
                else {//跳变的父节点不是root，则复制跳变的父节点给表
                    strncpy(pmeter->minfo.f_addr, node->ev.father, LTOPO_ADDR_LEN);
                    pmeter->minfo.resultby=LTOPO_RESULT_BY_CAL;
                    }
                return 0;
                }
            else if(!strcmp(pmeter->minfo.f_addr, node->ev.father)){ //表父节点已知，并且和跳变一致      
                printf(LTOPO_TAG "find it! exists\n");
                return 0;
                }
            else{ //表的父节点已知，并且和跳变不一致
                if(!g_ltopo.root){
                    printf(LTOPO_ERR_TAG "%s, error2, no root in gltopo\n", __FUNCTION__);
                    return 0;
                    }
                if(!strcmp(g_ltopo.root->minfo.addr, pmeter->minfo.f_addr) && !strcmp(node->ev.father, "-1"))
                    printf(LTOPO_TAG "find it, exits, the meter's father is root\n");
                else{
                    printf(LTOPO_TAG "find it! conflicts!\n");
                    }
                return 0;
                }
            }
        }
    return -1;
}
int ltopo_alg_merge2(LTOPO_LIST * meter_head, LTOPO_LIST * ev_head)
{
    int ret;
    LTOPO_LIST *p, *p2;
    LTOPO_JUMP_EV_NODE *pnode;
    LTOPO_METER_NODE * pmeter=NULL;

    list_for_each(p, ev_head){
        pnode=container_of(p, LTOPO_JUMP_EV_NODE, list);
        list_for_each(p2, meter_head){
            pmeter=container_of(p2, LTOPO_METER_NODE, list);
            if(!strcmp(pmeter->minfo.addr, pnode->ev.addr)){ //找到跳变对应的表
                printf(LTOPO_TAG "%s, got %s, meter father %s, ev father %s\n", __FUNCTION__, 
                                    pmeter->minfo.addr, pmeter->minfo.f_addr, pnode->ev.father);
                ret=ltopo_alg_meter_sm_proc(pmeter, pnode);
                if(ret!=0){
                    printf(LTOPO_ERR_TAG "%s, ltopo_alg_meter_sm_proc failed\n", __FUNCTION__);
                    return -1;
                    }
                break;
                }
            }
        if(!p2){
            printf(LTOPO_ERR_TAG "%s, can't find EV %s father %s\n", __FUNCTION__, 
                                    pnode->ev.addr, pnode->ev.father);
            return -1;
            }
        }
    return 0;
}
int ltopo_alg_record_ev_list(char * filename, LTOPO_LIST * head)
{
    ltopo_xml_save_ev_list(filename, head);
    return 0;
}

//特征值比较
int ltopo_alg_ev_comp(LTOPO_JUMP_EV_INFO * p1, LTOPO_JUMP_EV_INFO * p2, int diffval)
{
    if(p1->direction!=p2->direction)
        return -1;
    if(p1->start-p2->start>LTOPO_EV_COMP_START_DIFF || p2->start-p1->start>LTOPO_EV_COMP_START_DIFF)
        return -2;
    if(p1->value-p2->value>diffval || p2->value-p1->value>diffval)
    //if(p1->maxv-p1->minv-(p2->maxv-p2->minv)>diffval || p2->maxv-p2->minv-(p1->maxv-p1->minv)>diffval)
        return 1;
    return 0;
}

#if 0
int proc_single_jump(int phase, int sline, int mbindex, int vl)
{
    int j, bcount, iwsize, owsize;

    LTOPO_JUMP_EV_INFO mbev;
    LTOPO_JUMP_EV_NODE * pnew, *pnode, head;
    LTOPO_LIST *p;
    char father[LTOPO_ADDR_LEN];

    //printf(LTOPO_TAG "Enter %s\n", __FUNCTION__);
    printf(LTOPO_TAG "**%s, got the SINGLE jump, sline %d, box %s\n", __FUNCTION__, sline, g_ltopoalg.mbox[phase][mbindex].addr);

    ltopo_alg_gen_ev(sline, &g_ltopoalg.mbox[phase][mbindex].load[sline], g_ltopoalg.wsize+LTOPO_WINDOW_MARGIN_SIZE, &mbev, vl);
    mbev.type=LTOPO_METER_TYPE_MBOX;
    mbev.index=mbindex;
    mbev.phase=phase;
    strncpy(mbev.addr, g_ltopoalg.mbox[phase][mbindex].addr, LTOPO_ADDR_LEN);
    mbev.father[0]=0x0;
    
    head.list.next=NULL;
    pnew=(LTOPO_JUMP_EV_NODE *) malloc(sizeof(LTOPO_JUMP_EV_NODE));
    if(!pnew){
        printf(LTOPO_ERR_TAG "%s, malloc mbox EV node failed\n", __FUNCTION__);
        goto err;
        }
    memset(pnew, 0x0, sizeof(LTOPO_JUMP_EV_NODE));
    memcpy(&pnew->ev, &mbev, sizeof(LTOPO_JUMP_EV_INFO));
    list_add(&head.list, &pnew->list);
    
    bcount=g_ltopoalg.bcount[phase];
    iwsize=g_ltopoalg.wsize; // 扫描内窗口，共iwsize+1个点 
    owsize=iwsize+2*LTOPO_WINDOW_MARGIN_SIZE;   // 扫描外窗口，共owsize+1个点

    for(j=0;j<bcount;j++){
        //printf(LTOPO_TAG "******scan %s, line %d\n", g_ltopoalg.branch[phase][j].event_file, sline);
        ltopo_alg_scan_window(&g_ltopoalg.branch[phase][j], sline, iwsize, owsize, 
                                g_ltopoalg.vh, g_ltopoalg.vl);
        if(g_ltopoalg.branch[phase][j].iwstatus==LTOPO_EVENT_TYPE_JUMP||
                            g_ltopoalg.branch[phase][j].owstatus==LTOPO_EVENT_TYPE_JUMP){
            printf(LTOPO_TAG "%s, sline %d, ts %d\n", g_ltopoalg.branch[phase][j].event_file, sline, (int)g_ltopoalg.start+sline);
            ltopo_alg_gen_ev(sline, &g_ltopoalg.branch[phase][j].load[sline], g_ltopoalg.wsize+LTOPO_WINDOW_MARGIN_SIZE, &mbev, vl);
            mbev.type=LTOPO_METER_TYPE_BRANCH;
            mbev.index=j;
            mbev.phase=phase;
            strncpy(mbev.addr, g_ltopoalg.branch[phase][j].addr, LTOPO_ADDR_LEN);
            mbev.father[0]=0x0;
            
            pnew=(LTOPO_JUMP_EV_NODE *) malloc(sizeof(LTOPO_JUMP_EV_NODE));
            if(!pnew){
                printf(LTOPO_ERR_TAG "%s, malloc branch EV node failed\n", __FUNCTION__);
                goto err;
                }
            memset(pnew, 0x0, sizeof(LTOPO_JUMP_EV_NODE));
            memcpy(&pnew->ev, &mbev, sizeof(LTOPO_JUMP_EV_INFO));

            list_for_each(p, &head.list){
                pnode=container_of(p, LTOPO_JUMP_EV_NODE, list);
                if(pnew->ev.maxv>pnode->ev.maxv){
                    list_add_before(&head.list, p, &pnew->list);
                    break;
                    }
                }
            }
        }
    strcpy(father, "-1");
    list_for_each(p, &head.list){
        pnode=container_of(p, LTOPO_JUMP_EV_NODE, list);
        strncpy(pnode->ev.father, father, LTOPO_ADDR_LEN);
        strncpy(father, pnode->ev.addr, LTOPO_ADDR_LEN);
        }
    
    list_show(&head.list, ltopo_alg_ev_list_show);
    ltopo_alg_merge(&g_ltopo.meters.list, &head.list);

    ltopo_print_topo(&g_ltopo.meters.list);
    list_fini(&head.list, ltopo_alg_ev_list_fini);

    return 0;
    err:
    return -1;
}
#endif
int ltopo_alg_calcu_ev(int phase, int sline, int type, int index, LTOPO_JUMP_EV_INFO * ev)
{
    if(type==LTOPO_METER_TYPE_MBOX){
        ltopo_alg_gen_ev(sline, &g_ltopoalg.mbox[phase][index].load[sline], g_ltopoalg.wsize+2*LTOPO_WINDOW_MARGIN_SIZE, ev, g_ltopoalg.vl);
        strncpy(ev->addr, g_ltopoalg.mbox[phase][index].addr, LTOPO_ADDR_LEN);
        }
    else if(type==LTOPO_METER_TYPE_BRANCH){
        ltopo_alg_gen_ev(sline, &g_ltopoalg.branch[phase][index].load[sline], g_ltopoalg.wsize+2*LTOPO_WINDOW_MARGIN_SIZE, ev, g_ltopoalg.vl);
        strncpy(ev->addr, g_ltopoalg.branch[phase][index].addr, LTOPO_ADDR_LEN);
        }
    ev->type=type;
    ev->index=index;
    ev->phase=phase;
    ev->father[0]=0x0;
    return 0;
}
//处理单未知唯二跳变
int ltopo_alg_proc_s2_jump_1unknown(int phase, int sline, int known, int unknown, int vl, int action)
{
    int i, ret;
    int bcount ; /*branch个数*/
    int known_f=-1 ; /*已知表箱终端父亲的index*/
    LTOPO_JUMP_EV_INFO knownev; /*已知表箱的ev*/
    LTOPO_JUMP_EV_INFO knownev_f; /*已知表箱父亲的ev*/
    LTOPO_JUMP_EV_INFO mbev; /*其他ev*/
    LTOPO_METER_NODE * knownmb/*已知表箱终端节点指针*/, *knownmb_f, *unknownmb;
    LTOPO_LIST *p, head /*已知表箱终端所在整条分支BRANCH_LIST的链表头*/;
    char father[LTOPO_ADDR_LEN];

    //初始化整条分支链表的链表头
    head.next=NULL;

    knownmb=g_ltopoalg.mbox[phase][known].pm;
    unknownmb=g_ltopoalg.mbox[phase][unknown].pm;
    knownmb_f=ltopo_get_meter(knownmb->minfo.f_addr, &g_ltopo.meters.list);

    bcount=g_ltopoalg.bcount[phase];
    //找到表箱终端父亲在g_ltopoalg.branch[phase]中的index
    for(i=0;i<bcount;i++){
        if(!strncmp(g_ltopoalg.branch[phase][i].addr, knownmb->minfo.f_addr, strlen(knownmb->minfo.f_addr))){
            known_f=i;
            break;
            }
        }
    if(i==bcount){
        printf(LTOPO_TAG "can't find father in g_ltopoalg.branch\n");
        return -1;
        }

    //生成已知表箱终端的ev
    ret=ltopo_alg_gen_ev(sline, &g_ltopoalg.mbox[phase][known].load[sline], g_ltopoalg.wsize+2*LTOPO_WINDOW_MARGIN_SIZE, &knownev, vl);
    if(ret!=0){
        printf(LTOPO_ERR_TAG "%s, ltopo_alg_gen_ev failed\n", __FUNCTION__);
        return -1;
        }
    knownev.type=LTOPO_METER_TYPE_MBOX;
    knownev.index=known;
    knownev.phase=phase;
    strncpy(knownev.addr, g_ltopoalg.mbox[phase][known].addr, LTOPO_ADDR_LEN);
    knownev.father[0]=0x0;

    //生成已知表箱终端父亲的ev
    ret=ltopo_alg_gen_ev(sline, &g_ltopoalg.branch[phase][known_f].load[sline], g_ltopoalg.wsize+2*LTOPO_WINDOW_MARGIN_SIZE, &knownev_f, vl);
    if(ret!=0){
        printf(LTOPO_ERR_TAG "%s, ltopo_alg_gen_ev failed\n", __FUNCTION__);
        return -1;
        }
    knownev_f.type=LTOPO_METER_TYPE_MBOX;
    knownev_f.index=known;
    knownev_f.phase=phase;
    strncpy(knownev_f.addr, g_ltopoalg.branch[phase][known_f].addr, LTOPO_ADDR_LEN);
    knownev_f.father[0]=0x0;

    //从已知表箱终端，往上构建整条分支
    ret=ltopo_build_whole_branch_by_mboxaddr(phase, &head, knownmb->minfo.addr);
    if(ret!=0){
        printf(LTOPO_ERR_TAG "%s, ltopo_build_whole_branch_by_mboxaddr failed\n", __FUNCTION__);
        return -1;
        }
    list_show(&head, branch_list_show);

    //比较已知表箱终端和其父亲的ev，如果不同，则已知表箱终端的父亲后面还有其他表箱终端贡献跳变，
    //此表箱终端即未知表箱终端
    if(ltopo_alg_ev_comp(&knownev, &knownev_f, LTOPO_EV_COMP_VL_TIMES*g_ltopoalg.vl)!=0){
        printf(LTOPO_TAG "###%s lies in or after %s\n", unknownmb->minfo.addr, knownmb_f->minfo.addr);
        strncpy(father, knownmb_f->minfo.addr, LTOPO_ADDR_LEN);
        }
    else{ //未知表箱终端在已知表箱终端父亲的前面
        printf(LTOPO_TAG "###%s lies beyond %s\n", unknownmb->minfo.addr, knownmb_f->minfo.addr);
        int inbranch=0;
        BRANCH_LIST * pbranchlist, *pbranchlistbak=NULL;
        //下面检查整条分支，如果整条分支有分支单元与已知表箱终端ev不一致，
        //则未知表箱终端位于最后一个不一致的分支单元下面或后面；
        //如果一致，则未知表箱终端位于与整条分支无关的分支上
        list_for_each(p, &head){
            pbranchlist=container_of(p, BRANCH_LIST, list);
            if(strcmp(pbranchlist->minfo.addr, knownmb->minfo.addr)){
                ltopo_alg_calcu_ev(phase, sline, LTOPO_METER_TYPE_BRANCH, pbranchlist->index, &mbev);
                if(ltopo_alg_ev_comp(&knownev, &mbev, LTOPO_EV_COMP_VL_TIMES*g_ltopoalg.vl)!=0){
                    inbranch++;
                    pbranchlistbak=pbranchlist;
                    }
                }
            }
        if(inbranch){
            strncpy(father, pbranchlistbak->minfo.addr, LTOPO_ADDR_LEN);
            printf(LTOPO_TAG "inbranch, father %s\n", father);
            }
        else{
            strncpy(father, g_ltopo.root->minfo.addr, LTOPO_ADDR_LEN);
            printf(LTOPO_TAG "outbranch, father %s\n", father);
            }
        }

    //设置g_ltopoalg.branch中的排除项，即已知表箱终端所在的整条分支
    ret=ltopo_branch_list_set_exclude(&head, 1);
    if(ret!=0){
        ltopo_branch_list_set_exclude(&head, 0);
        goto err;
        }
    ret=ltopo_alg_proc_s1_jump(phase,  sline, unknown, vl, father, action);
    ltopo_branch_list_set_exclude(&head, 0);
    if(ret!=0){
        printf(LTOPO_ERR_TAG "%s, ltopo_alg_proc_s1_jump failed\n", __FUNCTION__);
        goto err;
        }
        
    list_fini(&head, branch_list_fini);
    head.next=NULL;
    return 0;
err:
    list_fini(&head, branch_list_fini);
    head.next=NULL;
    return -1;
}

//处理唯二跳变
int ltopo_alg_proc_s2_jump(int phase, int sline, int mbindex1, int mbindex2, int vl, int action)
{

    METER_INFO * pm1, * pm2;
    pm1=&g_ltopoalg.mbox[phase][mbindex1];
    pm2=&g_ltopoalg.mbox[phase][mbindex2];

    if(!strcmp(pm1->pm->minfo.f_addr, "-1") && !strcmp(pm2->pm->minfo.f_addr, "-1")){
        printf(LTOPO_TAG "Two unknown meters. Temporarily Ignore it.\n");
        return 0;
        }
    else if(strcmp(pm1->pm->minfo.f_addr, "-1") && strcmp(pm2->pm->minfo.f_addr, "-1")){
        printf(LTOPO_TAG "Two known meters. Ignore it. \n");
        return 0;
        }
    else if(strcmp(pm1->pm->minfo.f_addr, "-1")){
        printf(LTOPO_TAG "Known %s and unknown %s, do it. \n", pm1->pm->minfo.addr, pm2->pm->minfo.addr);
        return ltopo_alg_proc_s2_jump_1unknown(phase, sline,  mbindex1,  mbindex2,  vl, action);
        }
    else{
        printf(LTOPO_TAG "Known %s and unknown %s, do it. \n", pm2->pm->minfo.addr, pm1->pm->minfo.addr);
        return ltopo_alg_proc_s2_jump_1unknown(phase, sline,  mbindex2,  mbindex1,  vl, action);
        }

}        

int ltopo_alg_scan_branch(int phase, char * id)
{
    int lcount, i, j;
    int iwsize,owsize; //inner window size, outer window size
    METER_INFO * b;
    int m_ow_s_count, m_ow_j_count;

    int bcount=g_ltopoalg.bcount[phase];
    iwsize=g_ltopoalg.wsize; // 扫描内窗口，共iwsize+1个点 
    owsize=iwsize+2*LTOPO_WINDOW_MARGIN_SIZE;   // 扫描外窗口，共owsize+1个点
    lcount=(g_ltopoalg.end-g_ltopoalg.start)/g_ltopoalg.mcycle+1;
    
    //检查所有分支单元，是否是平滑波动+有效跳变的组合，如果还有其他状态（异常数据和中间状态）则放弃本次计算
    for(j=0;j<bcount;j++){
        b=&g_ltopoalg.branch[phase][j];
        if(strstr(b->addr, id)){
            m_ow_s_count=0;
            m_ow_j_count=0;
            for(i=0;i<lcount-owsize;i++){ //i遍历扫描线
                ltopo_alg_scan_window(b, i, iwsize, owsize, 
                                        g_ltopoalg.vh, g_ltopoalg.vl);
                if(b->owstatus==LTOPO_EVENT_TYPE_SMOOTH)
                    m_ow_s_count++;
                else if(b->owstatus==LTOPO_EVENT_TYPE_JUMP){
                    m_ow_j_count++;
                    i=i+owsize;
                    }
                }
            printf(LTOPO_TAG "#1234 branch %s, %d\n", b->event_file, m_ow_j_count);
            }
        }
    return 0;

}
int ltopo_alg_scan_node(int phase, char *id, int wsize)
{
    LTOPO_LIST head;
    head.next->NULL;
    METER_INFO *node;
	int bcount, mcount;
	int i, j;
	for(i=0;i<wsize;i++)
		{
			
		}
}

/* 
ltopo_alg_scan_node(int phase, int sline, int wsize, int msize, int vh, int vl)
	{
		LTOPO_LIST *head;
		LTOPO_LIST *p;
		METER_INFO * meter;
		int j;
		head=(LTOPO_LIST*)malloc(sizeof(LTOPO_LIST));
		head->next=NULL;
		for(j=0;j<g_ltopoalg.mcount[phase];j++){
			meter=&g_ltopoalg.mbox[phase][j];
			p->next=head->next;
			head->next=p;
			p=ltopo_alg_print_meter_window(meter, sline, wsize, msize, vh, vl);
			}
		for(j=0;j<g_ltopoalg.bcount[phase];j++){
			meter=&g_ltopoalg.branch[phase][j];
			p->next=head->next;
			head->next=p;
			p=ltopo_alg_print_meter_window(meter, sline, wsize, msize, vh, vl);
			}
		return 0;
		
	}

*/
int ltopo_alg_scan_ev(int phase,int action,LTOPO_JUMP_EV_INFO * mbev)
{
	int lcount/*扫描线的个数*/, mcount/*表箱终端的个数*/, bcount/*分支单元的个数*/;
	int i,j;
	int jindex[3600];
	int m_w_s_count, m_w_j_count;	/*窗口跳变个数，平滑个数*/
	int b_w_s_count, b_w_j_count;
	mcount=g_ltopoalg.mcount[phase];
	bcount=g_ltopoalg.bcount[phase];
	LTOPO_LIST head;
	head.next=NULL;
	for(i=0;i<mcount;i++)
		{
			ltopo_alg_scan_node(&g_ltopoalg.phase, int sline, g_ltopoalg.wsize, int msize, g_ltopoalg.vh, g_ltopoalg.vl);
			if(g_ltopoalg.mbox[phase][j].wstatus==LTOPO_EVENT_TYPE_SMOOTH)
                m_w_s_count++;
            else if(g_ltopoalg.mbox[phase][j].wstatus==LTOPO_EVENT_TYPE_JUMP)
				{
                	if(jindex ==-1) //首先记录到jindex数组中
                    	jindex =j;
               	 	m_w_j_count++;
                }
		}
	for(j=0;i<bcount;j++)
		{
			ltopo_alg_scan_node(&g_ltopoalg.phase, int sline, g_ltopoalg.wsize, int msize, g_ltopoalg.vh, g_ltopoalg.vl);
			if(g_ltopoalg.branch[phase][j].wstatus==LTOPO_EVENT_TYPE_SMOOTH)
                b_w_s_count++;
            else if(g_ltopoalg.branch[phase][j].wstatus==LTOPO_EVENT_TYPE_JUMP)
				{
                	if(jindex ==-1) //首先记录到jindex数组中
                    	jindex =j;
               	 	b_w_j_count++;
                }
			
		}
	
}

int ltopo_alg_scan(int phase, int action)
{
	
    int s1jump=0;
    int i, j, ret;
    int lcount/*扫描线个数*/, mcount/*meter box个数*/;

    //m_iw_s_count (在一根扫描线上的)表箱终端、内窗口、smooth的个数，
    //m_iw_j_count 表箱终端、内窗口、跳变的个数
    int m_iw_s_count, m_iw_j_count;

    //m_ow_s_count 表箱终端、外窗口、smooth的个数，
    //m_ow_j_count 表箱终端、外窗口、跳变的个数
    int m_ow_s_count, m_ow_j_count;

    int iwsize,owsize; //inner window size, outer window size

    int jindex1, jindex2; //记录表箱终端跳变的index，最多记录两个

    if(phase<0 || phase>=LTOPO_TYPE_PHASE_MAX){
        printf(LTOPO_ERR_TAG "__FUNCTION__: invalid phase %d\n", phase);
        return -1;
        }

    printf(LTOPO_TAG "Enter %s\n", __FUNCTION__);
    iwsize=g_ltopoalg.wsize; // 扫描内窗口，共iwsize+1个点 
    owsize=iwsize+2*LTOPO_WINDOW_MARGIN_SIZE;   // 扫描外窗口，共owsize+1个点
    mcount=g_ltopoalg.mcount[phase];
    lcount=(g_ltopoalg.end-g_ltopoalg.start)/g_ltopoalg.mcycle+1;
    printf(LTOPO_TAG "**************s1 process**************\n");
    for(i=0;i<lcount-owsize;i++){ //i遍历扫描线
        m_iw_s_count=0;
        m_iw_j_count=0;
        m_ow_s_count=0;
        m_ow_j_count=0;
        jindex1=jindex2=-1;
#ifdef ALG_DEBUG            
        printf(LTOPO_TAG "------scan line %d\n", i);
#endif
        for(j=0;j<mcount;j++){ //j遍历meter box
#ifdef ALG_DEBUG            
            printf(LTOPO_TAG "scan %s, time %d\n", g_ltopoalg.mbox[phase][j].event_file, g_ltopoalg.start+i);
if(i==434)
    printf(LTOPO_TAG "ok\n");
#endif
            ltopo_alg_scan_window(&g_ltopoalg.mbox[phase][j], i, iwsize, owsize, 
                                    g_ltopoalg.vh, g_ltopoalg.vl);
            if(g_ltopoalg.mbox[phase][j].owstatus==LTOPO_EVENT_TYPE_SMOOTH)
                m_ow_s_count++;
            else if(g_ltopoalg.mbox[phase][j].owstatus==LTOPO_EVENT_TYPE_JUMP){
                m_ow_j_count++;
#ifdef ALG_DEBUG            
            printf(LTOPO_TAG "LTOPO222 %s, sline %d, ts %d\n", g_ltopoalg.mbox[phase][j].event_file, i, (int)g_ltopoalg.start+i);
#endif
                }
            if(g_ltopoalg.mbox[phase][j].iwstatus==LTOPO_EVENT_TYPE_SMOOTH)
                m_iw_s_count++;
            else if(g_ltopoalg.mbox[phase][j].iwstatus==LTOPO_EVENT_TYPE_JUMP){
                if(jindex1==-1) //首先记录到jinex1
                    jindex1=j;
                else if(jindex2==-1) //然后记录到jindex2
                    jindex2=j;
                m_iw_j_count++;
                }
            }
        if(m_iw_j_count>0)
            printf(LTOPO_TAG "m_iw_j_count %d, m_iw_s_count %d\n", m_iw_j_count, m_iw_s_count);
        if(m_iw_j_count==1 && m_ow_s_count==mcount-1){
            printf(LTOPO_TAG "### got the S1 jump, sline %d, ts %d, mbox %s\n", i, i+g_ltopoalg.start, g_ltopoalg.mbox[phase][jindex1].event_file);
            if(g_ltopoalg.mbox[phase][jindex1].next_needed){
                printf(LTOPO_TAG "scan line needs moving next\n");
                continue;
                }
            print_alg_print_all_window(phase, i, iwsize, owsize, 
                                    g_ltopoalg.vh, g_ltopoalg.vl);
            if(jindex1==-1 || jindex1>=mcount){
                printf(LTOPO_ERR_TAG "%s, got invalid sigle jump %d\n", __FUNCTION__, jindex1);
                return  -1;
                }
            s1jump++;
            ret=ltopo_alg_proc_s1_jump(phase, i, jindex1, g_ltopoalg.vl, g_ltopo.root->minfo.addr, action);
            if(ret>=0){
                printf(LTOPO_TAG "%s, ltopo_alg_proc_s1_jump OK\n", __FUNCTION__);
                }
            else{
                printf(LTOPO_ERR_TAG "%s, ltopo_alg_proc_s1_jump failed\n", __FUNCTION__);
                return -1;
                }
            i=i+owsize;
            }
        
#ifdef ALG_DEBUG            
        printf(LTOPO_TAG "mbox iw smooth %d, mbox iw jump %d\n", m_iw_s_count, m_iw_j_count);
        printf(LTOPO_TAG "mbox ow smooth %d, mbox ow jump %d\n", m_ow_s_count, m_ow_j_count);
#endif

        }

    printf(LTOPO_TAG "#1234 single jump %d\n", s1jump);
    printf(LTOPO_TAG "**************END s1**************\n");
    ltopo_alg_scan_branch(phase, "000000001005");
#if 0    
    printf(LTOPO_TAG "**************s2 process**************\n");
        //扫描唯二跳变
        for(i=0;i<lcount-owsize;i++){ //i遍历扫描线
            m_iw_s_count=0;
            m_iw_j_count=0;
            m_ow_s_count=0;
            m_ow_j_count=0;
            jindex1=jindex2=-1;
            for(j=0;j<mcount;j++){ //j遍历meter box
#ifdef ALG_DEBUG            
                printf(LTOPO_TAG "------scan %s, line %d\n", g_ltopoalg.mbox[phase][j].event_file, i);
#endif
                ltopo_alg_scan_window(&g_ltopoalg.mbox[phase][j], i, iwsize, owsize, 
                                        g_ltopoalg.vh, g_ltopoalg.vl);
                if(g_ltopoalg.mbox[phase][j].owstatus==LTOPO_EVENT_TYPE_SMOOTH)
                    m_ow_s_count++;
                else if(g_ltopoalg.mbox[phase][j].owstatus==LTOPO_EVENT_TYPE_JUMP){
                    m_ow_j_count++;
#ifdef ALG_DEBUG            
                    printf(LTOPO_TAG "%s, sline %d, ts %d\n", g_ltopoalg.mbox[phase][j].event_file, i, (int)g_ltopoalg.start+i);
#endif
                    }
                if(g_ltopoalg.mbox[phase][j].iwstatus==LTOPO_EVENT_TYPE_SMOOTH)
                    m_iw_s_count++;
                else if(g_ltopoalg.mbox[phase][j].iwstatus==LTOPO_EVENT_TYPE_JUMP){
                    if(jindex1==-1) //首先记录到jinex1
                        jindex1=j;
                    else if(jindex2==-1) //然后记录到jindex2
                        jindex2=j;
                    m_iw_j_count++;
                    }
                }
            if(m_iw_j_count==2 && m_ow_s_count==mcount-2){
                printf(LTOPO_TAG "### got the S2 jump, sline %d, mbox %s %s\n", i, g_ltopoalg.mbox[phase][jindex1].addr, g_ltopoalg.mbox[phase][jindex2].event_file);
                if(g_ltopoalg.mbox[phase][jindex1].next_needed && g_ltopoalg.mbox[phase][jindex2].next_needed){
                    printf(LTOPO_TAG "scan line needs moving next\n");
                    continue;
                    }
                print_alg_print_all_window(phase, i, iwsize, owsize, 
                                        g_ltopoalg.vh, g_ltopoalg.vl);
                if(jindex1==-1 || jindex1>=mcount){
                    printf(LTOPO_ERR_TAG "%s, got invalid s2 jump 1 %d\n", __FUNCTION__, jindex1);
                    return  -1;
                    }
                if(jindex2==-1 || jindex2>=mcount){
                    printf(LTOPO_ERR_TAG "%s, got invalid s2 jump 2 %d\n", __FUNCTION__, jindex1);
                    return  -1;
                    }
                if(g_ltopoalg.action==LTOPO_ALG_ACTION_STATIS){
                    printf(LTOPO_TAG "%s, ignore s2 jump in statistics\n", __FUNCTION__);
                    return 0;
                    }
                ret=ltopo_alg_proc_s2_jump(phase, i, jindex1, jindex2, g_ltopoalg.vl, action);
                if(ret!=0){
                    printf(LTOPO_ERR_TAG "%s, ltopo_alg_proc_s2_jump failed\n", __FUNCTION__);
                    return -1;
                    }
                i=i+owsize;
                }
            
#ifdef ALG_DEBUG            
            printf(LTOPO_TAG "mbox iw smooth %d, mbox iw jump %d\n", m_iw_s_count, m_iw_j_count);
            printf(LTOPO_TAG "mbox ow smooth %d, mbox ow jump %d\n", m_ow_s_count, m_ow_j_count);
#endif
    
            }
        
        printf(LTOPO_TAG "**************END s2**************\n");
#endif //0        
    return 0;
}
#if 0 //SCAN2
int ltopo_scan_window2(METER_INFO * meter, int sline, int wsize,int vh, int vl)
{
    LTOPO_LOAD * w;
    w=&meter->load[sline];
    
#ifdef ALG_DEBUG 
    int i;
    for(i=0;i<=wsize;i++)
        printf(LTOPO_TAG "%d ", w[i].ap);
    printf(LTOPO_TAG " -->");
    //printf(LTOPO_TAG "%d %d %d %d %d %d -->", m[0].ap, m[1].ap, m[2].ap, m[3].ap, m[4].ap, m[5].ap, m[6], m[7]);
#endif
    //如果本窗口有丢数据，放弃
    if(is_window_nodata(w,wsize)){
#ifdef ALG_DEBUG            
        printf(LTOPO_TAG "no data \n");
#endif
        meter->iwstatus=LTOPO_EVENT_NO_DATA;
        }
    //如果本窗口有跳变，先处理，判断jump时需要先剔除LTOPO_DATA_MIDDLE -1的值
    else if(is_window_jump(w, wsize, vh)){
#ifdef ALG_DEBUG            
        printf(LTOPO_TAG "jump \n");
#endif
        meter->iwstatus=LTOPO_EVENT_TYPE_JUMP;
        }
    else if(is_window_middle(w,wsize)){
#ifdef ALG_DEBUG            
        printf(LTOPO_TAG "middle \n");
#endif
        meter->iwstatus=LTOPO_EVENT_TYPE_MIDDLE;
        }
    else if(is_window_smooth(w, wsize, vl)){
#ifdef ALG_DEBUG            
        printf(LTOPO_TAG "smooth \n");
#endif
        meter->iwstatus=LTOPO_EVENT_TYPE_SMOOTH;
        }
    else{
#ifdef ALG_DEBUG            
        printf(LTOPO_TAG "middle2 \n");
#endif
        meter->iwstatus=LTOPO_EVENT_TYPE_MIDDLE;
        }

    return 0;
}
#endif //0
void ltopo_alg_print_window(LTOPO_LOAD * w, int wsize)
{
    int i;
    for(i=0;i<wsize+1;i++)
        printf( "%d ", w[i].ap);
    printf( ",");
}

int ltopo_alg_ev_list_valid(LTOPO_LIST * head)
{
    LTOPO_LIST *p1, * p2;
    LTOPO_JUMP_EV_NODE * pe;
    LTOPO_METER_NODE  *pm;

    list_for_each(p1, head){
        pe=container_of(p1, LTOPO_JUMP_EV_NODE, list);
        list_for_each(p2, &g_ltopo.meters.list){
            pm=container_of(p2, LTOPO_METER_NODE, list);
            if(!strcmp(pm->minfo.addr, pe->ev.addr)){
                if(strcmp(pm->minfo.f_addr, "-1") && strcmp(pe->ev.father, "-1") 
                        &&strcmp(pm->minfo.f_addr, pe->ev.father))
                    return -1;
                }
            }
        }
    return 0;
}
int ltopo_alg_ev_validation_node(LTOPO_LIST * meter_head, LTOPO_JUMP_EV_NODE * node)
{
    LTOPO_LIST * p;
    LTOPO_METER_NODE * pmeter=NULL;
    printf(LTOPO_TAG "Enter %s\n", __FUNCTION__);
    
    list_for_each(p, meter_head){
        pmeter=container_of(p, LTOPO_METER_NODE, list);
        if(!strcmp(pmeter->minfo.addr, node->ev.addr)){ //找到跳变对应的表
            printf(LTOPO_TAG "find %s in meter list: ", pmeter->minfo.addr);
            if(pmeter->minfo.mtype==LTOPO_METER_TYPE_TTU){ //如果对应的表是TTU，不处理
                printf(LTOPO_ERR_TAG "meter type TTU!\n");
                return LTOPO_EV_VALIDATION_ERR_IGNORE;
                }
            else if(pmeter->minfo.mtype==LTOPO_METER_TYPE_MBOX && g_ltopo.root && !strcmp(pmeter->minfo.f_addr, g_ltopo.root->minfo.addr)){
                printf(LTOPO_ERR_TAG "meter father type TTU!\n");
                return LTOPO_EV_VALIDATION_ERR_IGNORE;
                }
            else if(!strcmp(pmeter->minfo.f_addr, "-1")){//如果表父节点未知，拷贝表的父节点
                printf("unknown father, OK\n");
                return LTOPO_EV_VALIDATION_OK;
                }
            else if(!strcmp(pmeter->minfo.f_addr, node->ev.father)){ //表父节点已知，并且和跳变一致      
                printf("known father and the same! OK\n");
                return LTOPO_EV_VALIDATION_OK;
                }
            else{ //表的父节点已知，并且和跳变不一致
                printf(LTOPO_ERR_TAG "conflicts, meter father %s, ev father %s\n", pmeter->minfo.f_addr, node->ev.father);
                return LTOPO_EV_VALIDATION_ERR_CONFLICTS;
                }
            }
        }
    printf(LTOPO_ERR_TAG "can't find %s in meter list\n", pmeter->minfo.addr);
    return LTOPO_EV_VALIDATION_ERR_IGNORE;
}

int ltopo_alg_ev_validation(LTOPO_LIST * meter_head, LTOPO_LIST * ev_head)
{
    int ret=LTOPO_EV_VALIDATION_OK;
    LTOPO_JUMP_EV_NODE *pnode;
    LTOPO_LIST *p;

    list_for_each(p, ev_head){
        pnode=container_of(p, LTOPO_JUMP_EV_NODE, list);
        ret=ltopo_alg_ev_validation_node(meter_head, pnode);
        if(ret!=LTOPO_EV_VALIDATION_OK){
            printf(LTOPO_ERR_TAG "%s, ltopo_alg_ev_validation_node failed\n", __FUNCTION__);
            return ret;
            }
        }
    return LTOPO_EV_VALIDATION_OK;
}
//处理单一跳变
//如果返回值等于0，则处理正常
//如果返回值大于0，则处理正常
//如果返回值小于0，则中断处理

int ltopo_alg_proc_s1_jump(int phase, int sline, int mbindex, int vl, char * f_addr, int action)
{
    int j, bcount, iwsize, owsize, ret;

    LTOPO_JUMP_EV_INFO mbev /*表箱终端ev*/, brev /*分支单元ev*/;
    LTOPO_JUMP_EV_NODE * pnew, *pnode;
    LTOPO_LIST *p, evhead /*ev链表头*/;
    char father[LTOPO_ADDR_LEN];

    LTOPO_LIST dup; //复制g_ltopo.meters
    
    //初始化dup和evhead
    dup.next=NULL;
    evhead.next=NULL;

    bcount=g_ltopoalg.bcount[phase];
    iwsize=g_ltopoalg.wsize; // 扫描内窗口，共iwsize+1个点 
    owsize=iwsize+2*LTOPO_WINDOW_MARGIN_SIZE;   // 扫描外窗口，共owsize+1个点
    
    printf(LTOPO_TAG "**%s, got the SINGLE jump, sline %d, box %s, father %s\n", __FUNCTION__, sline, g_ltopoalg.mbox[phase][mbindex].addr, f_addr);

    //检查所有分支单元，是否是平滑波动+有效跳变的组合，如果还有其他状态（异常数据和中间状态）则放弃本次计算
    for(j=0;j<bcount;j++){
        //如果exclude置为1，则不去扫描这个分支单元
        //对于处理唯一跳变，扫描所有分支单元的负荷数据
        //如果有其他状态（异常数据和中间状态），则放弃本次计算
        if(g_ltopoalg.branch[phase][j].excluded)
            continue;
        //printf(LTOPO_TAG "******scan %s, line %d\n", g_ltopoalg.branch[phase][j].event_file, sline);
        ltopo_alg_scan_window(&g_ltopoalg.branch[phase][j], 
                        sline, iwsize, owsize, g_ltopoalg.vh, g_ltopoalg.vl);
        
        if(g_ltopoalg.branch[phase][j].iwstatus==LTOPO_EVENT_NO_DATA){
            printf(LTOPO_WARN_TAG "%s, %s, no data\n", __FUNCTION__, g_ltopoalg.branch[phase][j].event_file);           
            return 1;
            }
        //内窗口是middle，同时外窗口不是jump，不应该出现这种情况，不符合条件
        if(g_ltopoalg.branch[phase][j].iwstatus==LTOPO_EVENT_TYPE_MIDDLE &&
                g_ltopoalg.branch[phase][j].owstatus!=LTOPO_EVENT_TYPE_JUMP){
            printf(LTOPO_WARN_TAG "%s, %s, %s\n", __FUNCTION__, g_ltopoalg.branch[phase][j].event_file, "middle");           
            return 1;
            }
        }
    //printf(LTOPO_TAG "******scan finished \n");
    printf(LTOPO_TAG "gen ev %s\n", g_ltopoalg.mbox[phase][mbindex].event_file);

    
    mbev.type=LTOPO_METER_TYPE_MBOX;
    mbev.index=mbindex;
    mbev.phase=phase;
    strncpy(mbev.addr, g_ltopoalg.mbox[phase][mbindex].addr, LTOPO_ADDR_LEN);
    mbev.father[0]=0x0;

    //计算表箱终端的跳变特征值
    ret=ltopo_alg_gen_ev(sline, &g_ltopoalg.mbox[phase][mbindex].load[sline], g_ltopoalg.wsize+2*LTOPO_WINDOW_MARGIN_SIZE, &mbev, vl);
    if(ret!=0){
        printf(LTOPO_ERR_TAG "%s, ltopo_alg_gen_ev mbox failed\n", __FUNCTION__);
        return -1;
        }

    
    //生成表箱终端的跳变ev节点
    pnew=(LTOPO_JUMP_EV_NODE *) malloc(sizeof(LTOPO_JUMP_EV_NODE));
    if(!pnew){
        printf(LTOPO_ERR_TAG "%s, malloc mbox EV node failed\n", __FUNCTION__);
        return -1;
        }
    memset(pnew, 0x0, sizeof(LTOPO_JUMP_EV_NODE));
    memcpy(&pnew->ev, &mbev, sizeof(LTOPO_JUMP_EV_INFO));
    //插入链表
    ret=list_add(&evhead, &pnew->list);
    if(ret!=0){
        printf(LTOPO_ERR_TAG "%s, list_add failed\n", __FUNCTION__);
        free(pnew);
        return -1;
        }
    

    //扫描分支单元的负荷数据
    for(j=0;j<bcount;j++){
        //如果exclude置为1，则不去扫描这个分支单元
        //对于处理唯一跳变，扫描所有分支单元的负荷数据
        //对于处理唯二跳变转化后的唯一跳变，扫描exclude为0的分支单元的负荷数据
        if(g_ltopoalg.branch[phase][j].excluded)
            continue;
        printf(LTOPO_TAG "******scan %s, line %d\n", g_ltopoalg.branch[phase][j].event_file, sline);
        ltopo_alg_scan_window(&g_ltopoalg.branch[phase][j], 
                        sline, iwsize, owsize, g_ltopoalg.vh, g_ltopoalg.vl);
        //ltopo_alg_scan_window(&g_ltopoalg.branch[phase][j], 
        //                /*sline, */sline>=LTOPO_WINDOW_MARGIN_SIZE?sline-LTOPO_WINDOW_MARGIN_SIZE:sline, 
        //                iwsize+LTOPO_BRANCH_WINDOW_ADD, owsize+LTOPO_BRANCH_WINDOW_ADD, g_ltopoalg.vh, g_ltopoalg.vl);
        if(g_ltopoalg.branch[phase][j].iwstatus==LTOPO_EVENT_TYPE_JUMP||
                            g_ltopoalg.branch[phase][j].owstatus==LTOPO_EVENT_TYPE_JUMP){
            //出现分支负荷跳变
            printf(LTOPO_TAG "%s, sline %d, ts %d\n", g_ltopoalg.branch[phase][j].event_file, sline, (int)g_ltopoalg.start+sline);

            brev.type=LTOPO_METER_TYPE_BRANCH;
            brev.index=j;
            brev.phase=phase;
            strncpy(brev.addr, g_ltopoalg.branch[phase][j].addr, LTOPO_ADDR_LEN);
            brev.father[0]=0x0;
            //计算分支负荷跳变特征值
            ret=ltopo_alg_gen_ev(sline, &g_ltopoalg.branch[phase][j].load[sline], g_ltopoalg.wsize+2*LTOPO_WINDOW_MARGIN_SIZE, &brev, vl);
            if(ret!=0){
                printf(LTOPO_ERR_TAG "%s, ltopo_alg_gen_ev branch failed\n", __FUNCTION__);
                ret=-1;
                goto err;
                }
            
            //表箱终端与跳变分支的特征值比较
            ret=ltopo_alg_ev_comp(&mbev, &brev, LTOPO_EV_COMP_VL_TIMES*vl);
            if(ret==-1){
                printf(LTOPO_ERR_TAG "%s, error, got different direction between mbox(%s) and branch(%s), in single jump\n", __FUNCTION__, mbev.addr, brev.addr);
                ret=1;
                goto err;
                }
            else if(ret==-2){
                printf(LTOPO_ERR_TAG "%s, error, got different start between mbox(%s %d) and branch(%s %d), in single jump\n", __FUNCTION__, mbev.addr, mbev.start, brev.addr, brev.start);
                ret=1;
                goto err;
                }
            else if(ret==1){
                printf(LTOPO_ERR_TAG "%s, warning, got different value between mbox(%s %d) and branch(%s %d), in single jump\n", __FUNCTION__, mbev.addr, mbev.value, brev.addr, brev.value);
                printf(LTOPO_ERR_TAG "warning, mbox %d, branch %d, %d,", mbev.value, brev.value, mbev.value-brev.value); 
                printf(" sline %d, %s %s, ", sline, mbev.addr, g_ltopoalg.branch[phase][j].event_file);
                ltopo_alg_print_window(&g_ltopoalg.mbox[phase][mbindex].load[sline], 5);
                ltopo_alg_print_window(&g_ltopoalg.branch[phase][j].load[sline], 5);
                printf(LTOPO_ERR_TAG "\n");
                ret=1;
                goto err;
                //continue; //continue or ignore?
                }
            //比较特征值成功
            pnew=(LTOPO_JUMP_EV_NODE *) malloc(sizeof(LTOPO_JUMP_EV_NODE));
            if(!pnew){
                printf(LTOPO_ERR_TAG "%s, malloc branch EV node failed\n", __FUNCTION__);
                ret=-1;
                goto err;
                }
            memset(pnew, 0x0, sizeof(LTOPO_JUMP_EV_NODE));
            memcpy(&pnew->ev, &brev, sizeof(LTOPO_JUMP_EV_INFO));

            //将跳变分支插入链表，按负荷值从大到小，插入链表
            //负荷值大的，就是靠前的分支，插入链表前面
            //后面要改为定期读取电量，按照电量进行比对，电量大的靠前
            list_for_each(p, &evhead){
                pnode=container_of(p, LTOPO_JUMP_EV_NODE, list);
                if(pnode->ev.type==LTOPO_METER_TYPE_MBOX || pnew->ev.maxv>=pnode->ev.maxv){
                    list_add_before(&evhead, p, &pnew->list);
                    break;
                    }
                }
            //如果分支负荷最小，则插入链表尾
            //这个是不应该出现的，因为链表尾是表箱终端，他的负荷值最小
            if(!p){
                printf(LTOPO_ERR_TAG "%s, fatal err, branch should before meter box\n", __FUNCTION__);
                ret=-1;
                goto err;
                //list_add(&evhead, &pnew->list);
                }
            }
        }
    //按链表顺序赋值father，前一个节点是后一个节点的father，
    //第一个节点的father，从参数copy生成
    strcpy(father, f_addr);
    list_for_each(p, &evhead){
        pnode=container_of(p, LTOPO_JUMP_EV_NODE, list);
        strncpy(pnode->ev.father, father, LTOPO_ADDR_LEN);
        strncpy(father, pnode->ev.addr, LTOPO_ADDR_LEN);
        }
    
    list_show(&evhead, ltopo_alg_ev_list_show);
    
    
    //if(g_ltopoalg.action==LTOPO_ALG_ACTION_CALCUL){
    if(action==LTOPO_ALG_ACTION_CALCUL){
#if 0
        //add validation to the list
        //
        ret=ltopo_alg_ev_validation(&g_ltopo.meters.list, &evhead);
        if(ret==LTOPO_EV_VALIDATION_ERR_CONFLICTS){
            printf("%s, ltopo_alg_ev_validation CONFLICTS!\n", __FUNCTION__);
            g_ltopo.conflicts=1;
            ret=-1;
            goto err;
            }
        else if(ret==LTOPO_EV_VALIDATION_ERR_IGNORE){
            printf("%s, ltopo_alg_ev_validation CONFLICTS!\n", __FUNCTION__);
            ret=-1;
            goto err;
            }
        //计算
        //复制一个meters，如果可以merge成功，则merge到g_ltopo.meters
        ret=ltopo_meters_dup(&dup, &g_ltopo.meters.list);
        if(ret!=0){
            printf(LTOPO_ERR_TAG "%s, ltopo_meters_dup failed\n", __FUNCTION__);
            ret=-1;
            goto err;
            }

        if(ltopo_alg_merge(&dup, &evhead)==0){
            ltopo_alg_merge(&g_ltopo.meters.list, &evhead);
            ltopo_refresh_mlist(&g_ltopo.meters.list, LTOPO_XML_FILE);
            }
        else{
            printf(LTOPO_ERR_TAG "%s, ltopo_alg_merge failed\n", __FUNCTION__);
            ret=-1;
            goto err;
            }
#else        
        if(ltopo_alg_merge2(&g_ltopo.meters.list, &evhead)==0){
            ltopo_refresh_mlist(&g_ltopo.meters.list, LTOPO_XML_FILE);
            }
        else{
            printf(LTOPO_ERR_TAG "%s, ltopo_alg_merge failed\n", __FUNCTION__);
            ret=-1;
            goto err;
            }
#endif        
        }
    else{
        //统计
        char fullname[128];
    
        snprintf(fullname, sizeof(fullname)-1, "%s/%s-%d", LTOPO_XML_STATIS_PATH, mbev.addr, sline+g_ltopoalg.start);
        printf(LTOPO_TAG "fullname %s\n", fullname);
        if(ltopo_alg_ev_list_valid(&evhead)==0)
            ltopo_alg_record_ev_list(fullname , &evhead);
        else
            printf(LTOPO_TAG "invalid %s\n", fullname);
        }

    list_fini(&dup, meter_fini);
    list_fini(&evhead, ltopo_alg_ev_list_fini);
    return 0;
err:
    list_fini(&dup, meter_fini);
    list_fini(&evhead, ltopo_alg_ev_list_fini);
    return ret;
}
#if 0 //SCAN2
int ltopo_alg_scan2(int phase)
{
    int i, j;
    int lcount/*扫描线个数*/, mcount/*meter box个数*/;

    //m_iw_s_count (在一根扫描线上的)表箱终端、内窗口、smooth的个数，
    //m_iw_j_count 表箱终端、内窗口、跳变的个数
    int m_iw_s_count, m_iw_j_count;

    int iwsize; //inner window size

    int jindex1, jindex2; //记录表箱终端跳变的index，最多记录两个

    if(phase<0 || phase>=LTOPO_TYPE_PHASE_MAX){
        printf(LTOPO_ERR_TAG "__FUNCTION__: invalid phase %d\n", phase);
        return -1;
        }
    
    iwsize=g_ltopoalg.wsize; // 扫描内窗口，共iwsize+1个点 
    mcount=g_ltopoalg.mcount[phase];
    lcount=(g_ltopoalg.end-g_ltopoalg.start)/g_ltopoalg.mcycle+1;
    for(i=0;i<lcount-iwsize;i++){ //i遍历扫描线
        m_iw_s_count=0;
        m_iw_j_count=0;
        jindex1=jindex2=-1;
#ifdef ALG_DEBUG            
        printf(LTOPO_TAG "------scan %s, line %d\n", g_ltopoalg.mbox[phase][j].event_file, i);
#endif
        for(j=0;j<mcount;j++){ //j遍历meter box
#ifdef ALG_DEBUG            
            printf(LTOPO_TAG "scan %s, ", g_ltopoalg.mbox[phase][j].event_file);
#endif
            ltopo_scan_window2(&g_ltopoalg.mbox[phase][j], i, iwsize, 
                                    g_ltopoalg.vh, g_ltopoalg.vl);
            if(g_ltopoalg.mbox[phase][j].iwstatus==LTOPO_EVENT_TYPE_SMOOTH)
                m_iw_s_count++;
            else if(g_ltopoalg.mbox[phase][j].iwstatus==LTOPO_EVENT_TYPE_JUMP){
                if(jindex1==-1) //首先记录到jinex1
                    jindex1=j;
                else if(jindex2==-1) //然后记录到jindex2
                    jindex2=j;
                m_iw_j_count++;
#ifdef ALG_DEBUG            
                printf(LTOPO_TAG "LTOPO222 %s, sline %d, ts %d\n", g_ltopoalg.mbox[phase][j].event_file, i, (int)g_ltopoalg.start+i);
#endif
                }
            }
        if(m_iw_j_count==1 && m_iw_s_count==mcount-1){ 
            if(jindex1==-1 || jindex1>=mcount){
                printf(LTOPO_ERR_TAG "%s, got invalid sigle jump %d\n", __FUNCTION__, jindex1);
                return  -1;
                }
            printf(LTOPO_TAG "LTOPO111 got the SINGLE jump, sline %d, mbox %s\n", i, g_ltopoalg.mbox[phase][jindex1].addr);
            ltopo_alg_proc_s1_jump(phase, i, jindex1, g_ltopoalg.vl);
            i=i+iwsize;
            }
        
#ifdef ALG_DEBUG            
        printf(LTOPO_TAG "mbox iw smooth %d, mbox iw jump %d\n", m_iw_s_count, m_iw_j_count);
#endif

        }

printf(LTOPO_TAG "****************************************\n");
#if 0    
    printf(LTOPO_TAG "s2 process\n");
        //扫描唯二跳变
        for(i=0;i<lcount-owsize;i++){ //i遍历扫描线
            m_iw_s_count=0;
            m_iw_j_count=0;
            m_ow_s_count=0;
            m_ow_j_count=0;
            jindex1=jindex2=-1;
            for(j=0;j<mcount;j++){ //j遍历meter box
#ifdef ALG_DEBUG            
                printf(LTOPO_TAG "------scan %s, line %d\n", g_ltopoalg.mbox[phase][j].event_file, i);
#endif
                ltopo_alg_scan_window(&g_ltopoalg.mbox[phase][j], i, iwsize, owsize, 
                                        g_ltopoalg.vh, g_ltopoalg.vl);
                if(g_ltopoalg.mbox[phase][j].owstatus==LTOPO_EVENT_TYPE_SMOOTH)
                    m_ow_s_count++;
                else if(g_ltopoalg.mbox[phase][j].owstatus==LTOPO_EVENT_TYPE_JUMP){
                    m_ow_j_count++;
#ifdef ALG_DEBUG            
                    printf(LTOPO_TAG "%s, sline %d, ts %d\n", g_ltopoalg.mbox[phase][j].event_file, i, (int)g_ltopoalg.start+i);
#endif
                    }
                if(g_ltopoalg.mbox[phase][j].iwstatus==LTOPO_EVENT_TYPE_SMOOTH)
                    m_iw_s_count++;
                else if(g_ltopoalg.mbox[phase][j].iwstatus==LTOPO_EVENT_TYPE_JUMP){
                    if(jindex1==-1) //首先记录到jinex1
                        jindex1=j;
                    else if(jindex2==-1) //然后记录到jindex2
                        jindex2=j;
                    m_iw_j_count++;
                    }
                }
            if(m_iw_j_count==2 && m_ow_s_count==mcount-2){
                if(jindex1==-1 || jindex1>=mcount){
                    printf(LTOPO_ERR_TAG "%s, got invalid s2 jump 1 %d\n", __FUNCTION__, jindex1);
                    return  -1;
                    }
                if(jindex2==-1 || jindex2>=mcount){
                    printf(LTOPO_ERR_TAG "%s, got invalid s2 jump 2 %d\n", __FUNCTION__, jindex1);
                    return  -1;
                    }
                printf(LTOPO_TAG "LTOPO111 got the S2 jump, sline %d, mbox %s %s\n", i, g_ltopoalg.mbox[phase][jindex1].addr, g_ltopoalg.mbox[phase][jindex2].addr);
                ltopo_alg_proc_s2_jump(phase, i, jindex1, jindex2, g_ltopoalg.vl);
                i=i+owsize;
                }
            
#ifdef ALG_DEBUG            
            printf(LTOPO_TAG "mbox iw smooth %d, mbox iw jump %d\n", m_iw_s_count, m_iw_j_count);
            printf(LTOPO_TAG "mbox ow smooth %d, mbox ow jump %d\n", m_ow_s_count, m_ow_j_count);
#endif
    
            }
#endif //0        
    return 0;
}
#endif //0 SCAN2

int ltopo_alg_clean(int phase)
{
    int i;
#if 0 //def M1        
    LTOPO_EVENT * p, * head;
#endif

    printf(LTOPO_TAG "Enter %s, phase %d\n", __FUNCTION__, phase);
    if(phase<0 || phase>=LTOPO_TYPE_PHASE_MAX){
        printf(LTOPO_ERR_TAG "__FUNCTION__: invalid phase %d\n", phase);
        return -1;
        }
    
    for(i=0;i<g_ltopoalg.mcount[phase];i++){
#if 0 //def M1        
        p=g_ltopoalg.mbox[phase][i].event;
        while(p){
            head=p->next;
            free(p);
            p=head;
            }
        g_ltopoalg.mbox[phase][i].event=NULL;
#endif //M1        
        if(g_ltopoalg.mbox[phase][i].load)
            free(g_ltopoalg.mbox[phase][i].load);
        }
    for(i=0;i<g_ltopoalg.bcount[phase];i++){
#if 0 //def M1        
        p=g_ltopoalg.branch[phase][i].event;
        while(p){
            head=p->next;
            free(p);
            p=head;
            }
        g_ltopoalg.branch[phase][i].event=NULL;
#endif //M1
        if(g_ltopoalg.branch[phase][i].load)
            free(g_ltopoalg.branch[phase][i].load);
        
        }
    free( g_ltopoalg.mbox[phase]);
    g_ltopoalg.mbox[phase]=NULL;

    free( g_ltopoalg.branch[phase]);
    g_ltopoalg.branch[phase]=NULL;
    return 0;
}
int verify_mcount_bcount(int mcount, int bcount)
{
#ifdef X86_LINUX
    return 0;
#else
    //mcount和bcount能否与表箱和分支档案对上，如果能返回0，否则返回-1
    if(mcount!=g_ltopo.mc){
        printf(LTOPO_ERR_TAG "%s, meter box mismatch, should %d, read %d\n", __FUNCTION__, g_ltopo.mc, mcount);
        return -1;
        }
    if(bcount!=g_ltopo.bc*5){
        printf(LTOPO_ERR_TAG "%s, branch mismatch, should %d, read %d\n", __FUNCTION__, 5*g_ltopo.bc, bcount);
        return -1;
        }
    return 0;
#endif    
}

LTOPO_ALG g_ltopoalg;
//path look like: /home/user/work/ltopo/ltopo/data/input/1585219561-1585223161
int ltopo_alg_set_path(char * path)
{
    int start,end;
    char * p, cmd[256], buf[256];
    if(strlen(path)>FILENAME_LEN-1){
        printf(LTOPO_TAG "%s, invalid path %s\n", __FUNCTION__, path);
        return -1;
        }
    if(path[strlen(path)-1]=='/')
        path[strlen(path)-1]=0x0;
    strncpy(g_ltopoalg.event_path, path, sizeof(g_ltopoalg.event_path)-1);
    p=strrchr(path, '/'); // p: /1585219561-1585223161
    if(!p)
        p=&path[0];
    else
        p++;
    start=atoi(p);
    p=strchr(p, '-');
    if(!p){
        printf(LTOPO_ERR_TAG "invalid path, no - between start and end\n");
        return -1;
        }
    end=atoi(++p);
    if(end-start<60){
        printf(LTOPO_ERR_TAG "too short period of time! start %d, end %d\n", start, end);
        return -1;
        }
    g_ltopoalg.start=start;
    g_ltopoalg.end=end;
    printf(LTOPO_TAG "%s, path %s, start at %d, end %d\n", __FUNCTION__, g_ltopoalg.event_path, 
                                       (int)g_ltopoalg.start, (int)g_ltopoalg.end);
    if(g_ltopoalg.start<=0 || g_ltopoalg.end<=0 || g_ltopoalg.start>=g_ltopoalg.end){
        printf(LTOPO_ERR_TAG "%s, invalid path %d to %d\n", __FUNCTION__,  (int)g_ltopoalg.start, (int)g_ltopoalg.end);
        return -1;
        }
    snprintf(cmd, sizeof(cmd)-1, "date -d @%d", g_ltopoalg.start);
    ltopo_get_shell_result(cmd, buf, sizeof(buf));
    if(strlen(buf)>=sizeof(buf)-1){
        printf(LTOPO_ERR_TAG "%s, start at '%s', end %d\n", __FUNCTION__, buf, (int)g_ltopoalg.end);
        return -1;
        }
    if(buf[strlen(buf)-1]=='\n')
        buf[strlen(buf)-1]=0x0;
    printf(LTOPO_TAG "%s, start at '%s', end %d\n", __FUNCTION__, buf, (int)g_ltopoalg.end);
    return 0;
}
void ltopo_alg_config(int cycle, int vh, int vl, int wsize, int flag)
{
    g_ltopoalg.mcycle=cycle;
    g_ltopoalg.vh=vh;
    g_ltopoalg.vl=vl;
    g_ltopoalg.wsize=wsize; // 窗口的间隔数，窗口有w+1个点，窗口时间长度cycle*wsize
    g_ltopoalg.flag=flag;
}
int ltopo_alg_init()
{
    memset(&g_ltopoalg, 0x0, sizeof(g_ltopoalg));
    ltopo_alg_config(LTOPO_DEFAULT_MCYCLE, LTOPO_DEFAULT_VH, LTOPO_DEFAULT_VL, LTOPO_DEFAULT_WSIZE, LTOPO_LOAD_TYPE_APOWER);
    return 0;
}

typedef struct struct_STATIS_NODE_PAIR {
    char addr[LTOPO_ADDR_LEN]; //表地址
    char f_addr[LTOPO_ADDR_LEN]; //父地址
}STATIS_NODE_PAIR;

typedef struct struct_STATIS_RECORDS {
    int rcount; //本记录的次数
    char f_addr[LTOPO_ADDR_LEN]; //父地址
    LTOPO_LIST list;
}STATIS_RECORDS;

typedef struct struct_LTOPO_STATIS
{
    int scount; //统计的次数
    char addr[LTOPO_ADDR_LEN]; //表地址
    STATIS_RECORDS rhead;
    LTOPO_LIST list;
} LTOPO_STATIS;


#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xmlmemory.h>
static void record_fini(void *arg)
{
        LTOPO_LIST *q = (LTOPO_LIST *)arg;
        STATIS_RECORDS  *p = container_of(q, STATIS_RECORDS, list);
        p->list.next = NULL;
        free(p);
}

static void statis_fini(void *arg)
{
        LTOPO_LIST *q = (LTOPO_LIST *)arg;
        LTOPO_STATIS * p = container_of(q, LTOPO_STATIS, list);
        list_fini(&p->rhead.list, record_fini);
        p->rhead.list.next=NULL;
        p->list.next = NULL;
        free(p);
}

int ltopo_alg_clean_statistics(LTOPO_LIST * head)
{
    list_fini(head, statis_fini);
    head->next=NULL;
    return 0;
}


int ltopo_alg_statis_insert_node_pair(LTOPO_LIST * head, STATIS_NODE_PAIR * pair)
{
    LTOPO_LIST *p;
    LTOPO_STATIS * ps, * ps_new=NULL;
    STATIS_RECORDS * pr, * pr_new=NULL;
    
    printf(LTOPO_TAG "%s-->%s\n", pair->addr, pair->f_addr);
    //查找链表中pair->addr是否存在
    list_for_each(p, head){
        ps=container_of(p, LTOPO_STATIS, list);
        if(!strcmp(ps->addr, pair->addr))
            break;
        }
    //pair->addr不存在
    if(!p){
        ps_new=(LTOPO_STATIS* )malloc(sizeof(LTOPO_STATIS));
        if(!ps_new){
            printf(LTOPO_TAG "%s, malloc LTOPO_STATIS failed\n", __FUNCTION__);
            goto err;
            }
        memset(ps_new, 0x0, sizeof(LTOPO_STATIS));
        strncpy(ps_new->addr, pair->addr, LTOPO_ADDR_LEN);
        list_add(head, &ps_new->list);
        ps=ps_new;
        }

    //now ps is ready
    //查找pair->f_addr在ps记录中是否存在
    list_for_each(p, &ps->rhead.list){
        pr=container_of(p, STATIS_RECORDS, list);
        if(!strcmp(pr->f_addr, pair->f_addr)){
            pr->rcount++;
            ps->scount++;
            return 0;
            }
        }
    //pair->f_addr在ps记录中不存在
    pr_new=(STATIS_RECORDS*)malloc(sizeof(STATIS_RECORDS));
    if(!pr_new){
        printf(LTOPO_TAG "%s, malloc STATIS_RECORDS failed\n", __FUNCTION__);
        goto err;
        }
    memset(pr_new, 0x0, sizeof(STATIS_RECORDS));
    strncpy(pr_new->f_addr, pair->f_addr, LTOPO_ADDR_LEN);
    pr_new->rcount++;
    ps->scount++;
    list_add(&ps->rhead.list, &pr_new->list);

    return 0;
err:
    
    ltopo_alg_clean_statistics(head);
    return -1;
}

int ltopo_alg_statistics_file(LTOPO_LIST * head, char * xml)
{
    xmlDocPtr doc; //xml整个文档的树形结构
    xmlNodePtr cur; //xml节点 
    xmlNodePtr son;
    xmlChar *key;
    STATIS_NODE_PAIR pair;
    int ret=0;
    
    printf(LTOPO_TAG "Enter %s:%s\n", __FUNCTION__, xml); 
    //获取树形结构
    doc=xmlReadFile(xml, NULL, XML_PARSE_NOBLANKS);
    if (doc == NULL) {
        printf(LTOPO_ERR_TAG "Failed to parse xml file:%s\n", xml);
        goto err;
    }
    //获取根节点
    cur = xmlDocGetRootElement(doc);
    if (cur == NULL) {
        printf(LTOPO_ERR_TAG "Root is empty.\n");
        goto err;
    }

    cur = cur->xmlChildrenNode;
    while (cur != NULL)
    {
        if ((!xmlStrcmp(cur->name, (const xmlChar *)"METER")))
        {
            printf(LTOPO_TAG "\tgot %s\n", "METER");
            son=cur->xmlChildrenNode;
            memset(&pair, 0x0, sizeof(pair));
            while (son != NULL)
            {
                if ((!xmlStrcmp(son->name, (const xmlChar *)"ADDR")))
                {
                    key = xmlNodeListGetString(doc, son->xmlChildrenNode, 1);
                    strncpy(pair.addr, (char *)key, LTOPO_ADDR_LEN-1);
                    xmlFree(key);
                }                    
                if ((!xmlStrcmp(son->name, (const xmlChar *)"F_ADDR")))
                {
                    key = xmlNodeListGetString(doc, son->xmlChildrenNode, 1);
                    strncpy(pair.f_addr, (char *)key, LTOPO_ADDR_LEN-1);
                    xmlFree(key);
                }                    
                son=son->next;
            }
            if(pair.addr[0]!=0x0 && pair.f_addr[0]!=0x0){
                ret=ltopo_alg_statis_insert_node_pair(head, &pair);
                if(ret!=0){
                    printf(LTOPO_ERR_TAG "%s ltopo_alg_statis_insert_node_pair failed\n", __FUNCTION__);
                    goto err;
                    }
                }
        }
        cur = cur->next;
    }
    if(ret!=0)
        printf(LTOPO_TAG "%s, ltopo_xml_load__meter ret %d\n", __FUNCTION__, ret);
    xmlFreeDoc(doc);
    xmlCleanupParser();
    return 0;
err:
    if (doc) {xmlFreeDoc(doc);}
    xmlCleanupParser();
    return -1;
}
void ltopo_alg_print_statistics(LTOPO_LIST * head)
{
    LTOPO_LIST *p1, *p2;
    LTOPO_STATIS  *ps;
    STATIS_RECORDS * pr;
    
    
    list_for_each(p1, head){
        ps=container_of(p1, LTOPO_STATIS, list);
        printf(LTOPO_TAG "[%s]:",ps->addr);
        list_for_each(p2, &ps->rhead.list){
            pr=container_of(p2, STATIS_RECORDS, list);
            printf(LTOPO_TAG "(%s:%d) ", pr->f_addr, pr->rcount);
            }
        printf(LTOPO_TAG "\n");
        }
}
int ltopo_alg_merge_statistics_pair(LTOPO_LIST * dup, STATIS_NODE_PAIR * pair, int count, int total)
{
    LTOPO_LIST *p;
    LTOPO_METER_NODE  *pmeter;
    printf(LTOPO_TAG "%s-->%s:%d/%d\n", pair->addr, pair->f_addr,count, total);
    list_for_each(p, dup){
        pmeter=container_of(p, LTOPO_METER_NODE, list);
        if(!strcmp(pmeter->minfo.addr, pair->addr)){
            printf(LTOPO_TAG "got it, father %s result by %d\n", pmeter->minfo.f_addr, pmeter->minfo.resultby);
            if(pmeter->minfo.resultby!=LTOPO_RESULT_BY_CAL || pmeter->minfo.resultby!=LTOPO_RESULT_BY_STA){
                printf(LTOPO_TAG "UNKNOWN, copy it! ");
                if(!strcmp(pair->f_addr, "-1")){
                    if(!g_ltopo.root)
                        printf(LTOPO_ERR_TAG "%s, error, no root in gltopo\n", __FUNCTION__);
                    else
                        strncpy(pmeter->minfo.f_addr, g_ltopo.root->minfo.addr, LTOPO_ADDR_LEN);
                    }
                else
                    strncpy(pmeter->minfo.f_addr, pair->f_addr, LTOPO_ADDR_LEN);
                printf(LTOPO_TAG "Father %s\n", pmeter->minfo.f_addr);
                pmeter->minfo.resultby=LTOPO_RESULT_BY_STA;
                }
            else if(pmeter->minfo.resultby==LTOPO_RESULT_BY_CAL){
                printf(LTOPO_TAG "CALCULATED! ");
                if(!strcmp(pair->f_addr, "-1")){
                    printf(LTOPO_TAG "meter f_addr %s, pair f_addr %s\n", pmeter->minfo.f_addr, pair->f_addr);
                    }
                else if(!strcmp(pair->f_addr, pmeter->minfo.f_addr)){
                    printf(LTOPO_TAG "same f_addr %s\n", pmeter->minfo.f_addr);
                    }
                else{
                    printf(LTOPO_TAG "different f_addr, meter f_addr %s, pair f_addr %s\n", pmeter->minfo.f_addr, pair->f_addr);
                    }
                }
            else if(pmeter->minfo.resultby==LTOPO_RESULT_BY_STA){
                printf(LTOPO_TAG "STATISTICS! ");
                if(!strcmp(pair->f_addr, "-1")){
                    printf(LTOPO_TAG "meter f_addr %s, pair f_addr %s\n", pmeter->minfo.f_addr, pair->f_addr);
                    }
                else if(!strcmp(pair->f_addr, pmeter->minfo.f_addr)){
                    printf(LTOPO_TAG "same f_addr %s\n", pmeter->minfo.f_addr);
                    }
                else{
                    printf(LTOPO_TAG "different f_addr, meter f_addr %s, pair f_addr %s\n", pmeter->minfo.f_addr, pair->f_addr);
                    }
                }
            }
        }
    return 0;
}
int ltopo_alg_merge_statistics(LTOPO_LIST * dup, LTOPO_LIST * statis)
{
    LTOPO_LIST *p1, *p2;
    LTOPO_STATIS  *ps;
    STATIS_RECORDS * pr, *prmax;
    STATIS_NODE_PAIR pair;
    int count=0;
    
    list_for_each(p1, statis){
        count=0;
        memset(&pair, 0x0, sizeof(pair));
        ps=container_of(p1, LTOPO_STATIS, list);
        strncpy(pair.addr, ps->addr, LTOPO_ADDR_LEN);
        list_for_each(p2, &ps->rhead.list){
            pr=container_of(p2, STATIS_RECORDS, list);
            if(count==0){
                count=pr->rcount;
                prmax=pr;
                }
            else{
                if(count<pr->rcount){
                    count=pr->rcount;
                    prmax=pr;
                    }
                }
            }
        strncpy(pair.f_addr, prmax->f_addr, LTOPO_ADDR_LEN);
        ltopo_alg_merge_statistics_pair(dup, &  pair, count, ps->scount);
        }
    return 0;
}

int ltopo_alg_statistics(char * path)
{
    int ret;
    struct dirent *entry;  
    DIR *dir = NULL;  
    char fullname[128];
    LTOPO_LIST statis, dup;

    printf(LTOPO_TAG "Enter %s, path %s\n", __FUNCTION__, path);

    statis.next=NULL;
    if((dir = opendir(path))==NULL)   {  
        printf(LTOPO_ERR_TAG "opendir failed!");  
        return -1;  
        }
    while((entry=readdir(dir)))  {
        if(entry->d_type==8){
            snprintf(fullname, sizeof(fullname)-1, "%s/%s", path, entry->d_name);
            ret=ltopo_alg_statistics_file(&statis, fullname);
            if(ret!=0){
                printf(LTOPO_ERR_TAG "%s, ltopo_alg_statistics_file %s failed\n", __FUNCTION__, fullname);
                closedir(dir);
                return -1;
                }
            }
        }

    closedir(dir);

    ltopo_alg_print_statistics(&statis);

    dup.next=NULL;
    ret=ltopo_meters_dup(&dup, &g_ltopo.meters.list);
    if(ret!=0){
        printf(LTOPO_ERR_TAG "%s, ltopo_meters_dup failed\n", __FUNCTION__);
        return -1;
        }

    ltopo_alg_merge_statistics(&dup, &statis);
    ltopo_print_mlist(&dup);

#ifdef X86_LINUX    
    ltopo_refresh_mlist(&dup, LTOPO_TEST_PATH "data/output/2.xml");
#else
    ltopo_refresh_mlist(&dup, "./2.xml");
#endif

    list_fini(&dup, meter_fini);

    
    ltopo_alg_clean_statistics(&statis);
    return 0;
}

