#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xmlmemory.h>

#include "ltopo_define.h"
#include "ltopo.h"
#include "ltopo_alg.h"
#include "ltopo_xml.h"



//创建表节点
static xmlNodePtr xml_create_meter_node(const XML_METER *meter)
{
    char type[8] = {0};
    xmlNodePtr meter_node = NULL;

    if(!meter){
        printf(LTOPO_ERR_TAG "%s failed, meter NULL!\n", __FUNCTION__);
        return NULL;
        }
    meter_node = xmlNewNode(NULL, BAD_CAST"METER");
    if (meter_node == NULL) {
        printf(LTOPO_ERR_TAG "Failed to create new node.\n");
        return NULL;
    }
    //设置属性
    snprintf(type, sizeof(type)-1, "%d", meter->mtype);
    xmlNewChild(meter_node, NULL, BAD_CAST"TYPE", (xmlChar *)type);
    xmlNewChild(meter_node, NULL, BAD_CAST"ADDR", (xmlChar *)meter->addr);
    xmlNewChild(meter_node, NULL, BAD_CAST"F_ADDR", (xmlChar *)meter->f_addr);
    snprintf(type, sizeof(type)-1, "%d", meter->resultby);
    xmlNewChild(meter_node, NULL, BAD_CAST"RESULTBY", (xmlChar *)type);
    snprintf(type, sizeof(type)-1, "%d", meter->id);
    xmlNewChild(meter_node, NULL, BAD_CAST"ID", (xmlChar *)type);
    snprintf(type, sizeof(type)-1, "%d", meter->f_id);
    xmlNewChild(meter_node, NULL, BAD_CAST"F_ID", (xmlChar *)type);
    snprintf(type, sizeof(type)-1, "%d", meter->level);
    xmlNewChild(meter_node, NULL, BAD_CAST"LEVEL", (xmlChar *)type);
    snprintf(type, sizeof(type)-1, "%d", meter->sm_state);
    xmlNewChild(meter_node, NULL, BAD_CAST"STATUS", (xmlChar *)type);
    snprintf(type, sizeof(type)-1, "%d", meter->sm_counter);
    xmlNewChild(meter_node, NULL, BAD_CAST"COUNTER", (xmlChar *)type);

    return meter_node;
}

//向根节点中添加一个表节点
static int xml_add_meter_node_to_root(xmlNodePtr root_node, XML_METER *meter)
{
    xmlNodePtr meter_node = NULL;

    //创建一个新的表
    if (meter == NULL) {
        printf(LTOPO_ERR_TAG "%s failed, meter NULL\n", __FUNCTION__);
        return -1;
    }

    //创建一个表节点
    meter_node = xml_create_meter_node(meter);
    if (meter_node == NULL) {
        printf(LTOPO_ERR_TAG "%s, Failed to create phone node.\n", __FUNCTION__);
        return -1;
    }
    //根节点添加一个子节点
    xmlAddChild(root_node, meter_node);

    return 0;
}

//保存线路拓扑的xml文件
int ltopo_xml_save_file(LTOPO_LIST* head, const char *filename)
{
    XML_METER meter;
    xmlDocPtr doc = NULL;
    xmlNodePtr root_node = NULL;

    if(!filename){
        printf(LTOPO_ERR_TAG "%s, filename NULL\n", __FUNCTION__);
        return -1;
        }

    //remove(filename);
    //创建一个xml 文档
    doc = xmlNewDoc(BAD_CAST"1.0");
    if (doc == NULL) {
        printf(LTOPO_ERR_TAG "%s, failed to new doc.\n", __FUNCTION__);
        return -1;
    }

    //创建根节点
    root_node = xmlNewNode(NULL, BAD_CAST"ROOT");
    if (root_node == NULL) {
        printf(LTOPO_ERR_TAG "%s, failed to new root node.\n", __FUNCTION__);
        goto FAILED;
    }
    //将根节点添加到文档中
    xmlDocSetRootElement(doc, root_node);

    LTOPO_LIST * p;
    LTOPO_MBOX_NODE  *pmbox;
    list_for_each(p, head){
        pmbox=container_of(p, LTOPO_METER_NODE, list);
    
        memcpy(&meter, &pmbox->minfo, sizeof(XML_METER));
        if (xml_add_meter_node_to_root(root_node, &meter) != 0) {
            printf(LTOPO_ERR_TAG "Failed to add a new meter node.\n");
            goto FAILED;
            }
        }

    //将文档保存到文件中，按照GBK编码格式保存
    xmlSaveFormatFileEnc(filename, doc, "GBK", 1);
    xmlFreeDoc(doc);
    xmlCleanupParser();

    return 0;
    FAILED:
    if (doc) {
        xmlFreeDoc(doc);
    }
    xmlCleanupParser();

    return -1;
}

//保存线路拓扑的xml文件
int ltopo_xml_save_ev_list(const char *filename, LTOPO_LIST * head)
{
    XML_METER meter;
    xmlDocPtr doc = NULL;
    xmlNodePtr root_node = NULL;

    if(!filename){
        printf(LTOPO_ERR_TAG "%s, filename NULL\n", __FUNCTION__);
        return -1;
        }

    //创建一个xml 文档
    doc = xmlNewDoc(BAD_CAST"1.0");
    if (doc == NULL) {
        printf(LTOPO_ERR_TAG "%s, failed to new doc.\n", __FUNCTION__);
        return -1;
    }

    //创建根节点
    root_node = xmlNewNode(NULL, BAD_CAST"ROOT");
    if (root_node == NULL) {
        printf(LTOPO_ERR_TAG "%s, failed to new root node.\n", __FUNCTION__);
        goto FAILED;
    }
    //将根节点添加到文档中
    xmlDocSetRootElement(doc, root_node);

    LTOPO_LIST * p;
    LTOPO_JUMP_EV_NODE  *pev;
    list_for_each(p, head){
        pev=container_of(p, LTOPO_JUMP_EV_NODE, list);

        memset(&meter, 0x0, sizeof(meter));
        meter.mtype=pev->ev.type;
        strncpy(meter.addr, pev->ev.addr, LTOPO_ADDR_LEN-1);
        strncpy(meter.f_addr, pev->ev.father, LTOPO_ADDR_LEN-1);
        if (xml_add_meter_node_to_root(root_node, &meter) != 0) {
            printf(LTOPO_ERR_TAG "Failed to add a new meter node.\n");
            goto FAILED;
            }
        }

    //将文档保存到文件中，按照GBK编码格式保存
    xmlSaveFormatFileEnc(filename, doc, "GBK", 1);
    xmlFreeDoc(doc);
    xmlCleanupParser();

    return 0;
    FAILED:
    if (doc) {
        xmlFreeDoc(doc);
    }
    xmlCleanupParser();

    return -1;
}

//////////////////////////////////////////
char * ltopo_meter_para[]={"TYPE", "ADDR", "F_ADDR", "RESULTBY","ID", "F_ID", "LEVEL", "STATUS", "COUNTER"};
int ltopo_xml_load__meter(xmlDocPtr doc, xmlNodePtr cur)
{
    int j;
    xmlChar *key;
    xmlNodePtr son;
    XML_METER meter;

    cur = cur->xmlChildrenNode;
    while (cur != NULL)
    {

            if ((!xmlStrcmp(cur->name, (const xmlChar *)"METER")))
            {
                //printf(LTOPO_TAG "\tgot %s\n", "METER");
                memset(&meter, 0x0, sizeof(meter));
                son=cur->xmlChildrenNode;
                while (son != NULL)
                {
                    for(j=0;j<LTOPO_METER_PARA_MAX;j++)
                    {
                        if ((!xmlStrcmp(son->name, (const xmlChar *)ltopo_meter_para[j])))
                        {
                            key = xmlNodeListGetString(doc, son->xmlChildrenNode, 1);
                            //printf(LTOPO_TAG "<%s>%s</%s>\n", ltopo_meter_para[j], key, ltopo_meter_para[j]);
                            if(!key){
                                printf(LTOPO_ERR_TAG "%s, NULL <%s>\n", __FUNCTION__, ltopo_meter_para[j]);
                                return -1;
                                }
                            ltopo_set_meter_para(&meter, j, (char *)key);
                            xmlFree(key);
                        }                    
                    }
                    son=son->next;
                }
                ltopo_add_meter_in_mlist(&meter);
                //ltopo_increase_meter_count();
            }
        cur = cur->next;
    }
    return 0;
}

//读取线路拓扑的xml文件
int ltopo_xml_load_file(char * xml)
{
	xmlDocPtr doc; //xml整个文档的树形结构
	xmlNodePtr cur; //xml节点	
	int ret;

    printf(LTOPO_TAG "Enter %s:%s\n", __FUNCTION__, xml);	
	//获取树形结构
    doc=xmlReadFile(xml, NULL, XML_PARSE_NOBLANKS);
	//doc = xmlParseFile(xml);
	if (doc == NULL) {
		printf(LTOPO_ERR_TAG "Failed to parse xml file:%s\n", xml);
		goto FAILED;
	}
	//获取根节点
	cur = xmlDocGetRootElement(doc);
	if (cur == NULL) {
		printf(LTOPO_ERR_TAG "Root is empty.\n");
		goto FAILED;
	}
    ret=ltopo_xml_load__meter(doc, cur);
    if(ret!=0)
        printf(LTOPO_TAG "%s, ltopo_xml_load__meter ret %d\n", __FUNCTION__, ret);
	xmlFreeDoc(doc);
    xmlCleanupParser();
	return 0;
FAILED:
	if (doc) {xmlFreeDoc(doc);}
    xmlCleanupParser();
	return -1;
}


