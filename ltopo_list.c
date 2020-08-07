/*
 * Generic single linked list implementation
 */
#include <stdio.h>
#include "ltopo_list.h"


int list_add(LTOPO_LIST *head, LTOPO_LIST *node)
{
    LTOPO_LIST * p;
    if (!head || !node)
        return -1;
    
    if(head->next==NULL)
        p=head;
    else{
        p=head->next;
        while(p->next){
            p=p->next;
            }
        }
    p->next=node;
    node->next=NULL;
    return 0;

}
/*
int list_add_after(LTOPO_LIST *where, LTOPO_LIST *node)
{
    LTOPO_LIST * p;
    if (!where || !node)
        return -1;
    p=where->next;
    where->next=node;
    node->next=p;
    return 0;
}
*/
int list_add_before(LTOPO_LIST *head, LTOPO_LIST *where, LTOPO_LIST *node)
{
    LTOPO_LIST * p;
    if (!where || !node)
        return -1;

    p=head;
    while(p){
        if(p->next==where){
            p->next=node;
            node->next=where;
            return 0;
            }
        p=p->next;
        }
    return -1;
}
int list_add_head(LTOPO_LIST *head, LTOPO_LIST *node)
{
    LTOPO_LIST * p;
    if (!head || !node)
        return -1;

    p=head->next;
    head->next=node;
    node->next=p;
    return 0;
}
int list_remove(LTOPO_LIST *head, LTOPO_LIST *node)
{
    LTOPO_LIST * p;
    if (!head || !node)
        return -1;

    p=head;
    while(p->next){
        if(p->next==node){
            p->next=node->next;
            node->next=NULL;
            return 0;
            }
        p=p->next;
        }
    return -1;
}

int list_show(LTOPO_LIST *head, list_handler_t show)
{
    LTOPO_LIST *p;
    if(!head || !show)
        return -1;
    for (p = head->next; p != NULL; p = p->next)
        show(p);
    return 0;
}

int list_fini(LTOPO_LIST *head, list_handler_t fini)
{
    LTOPO_LIST *p, *q;
    if(!head || !fini)
        return -1;
    p = head->next;
    while (p) {
        q = p;
        p = p->next;
        fini(q);
        }
    return 0;
}

