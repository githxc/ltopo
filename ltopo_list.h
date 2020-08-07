#ifndef _LTOPO_LIST_H
#define _LTOPO_LIST_H


#ifdef  __cplusplus
extern "C" {
#endif

/**
 * offsetof - offset of a structure member
 * @TYPE:       the type of the struct.
 * @MEMBER:     the name of the member within the struct.
 */
#define offsetof(TYPE, MEMBER) ((size_t)(&(((TYPE *)0)->MEMBER)))

/**
 * container_of - cast a member of a structure out to the containing structure
 * @ptr:        the pointer to the member.
 * @type:       the type of the container struct this is embedded in.
 * @member:     the name of the member within the struct.
 *
 */
#define container_of(ptr, type, member) ({                      \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type, member) );})
        
//获得包含节点的结构
#define list_entry(ptr, type, member) \
        container_of(ptr, type, member)

//正向遍历链表head所在的结构
/*
#define list_for_each_entry(pos, head, member)				\
	for (pos = list_entry((head)->next, typeof(*pos), member);	\
	     &pos->member != NULL; 	\
	     pos = list_entry(pos->member.next, typeof(*pos), member))
*/
#define list_for_each(pos, head) \
        for (pos = (head)->next; pos != NULL; pos = pos->next)

typedef struct struct_LTOPO_LIST {
        struct struct_LTOPO_LIST *next;
} LTOPO_LIST;
//判断head节点是不是空闲
static inline int list_empty(LTOPO_LIST *head)
{
	return head->next == NULL;
}

typedef void (*list_handler_t)(void *arg);

extern int list_add(LTOPO_LIST *head, LTOPO_LIST *node);
extern int list_add_before(LTOPO_LIST *head, LTOPO_LIST *where, LTOPO_LIST *node);
int list_add_head(LTOPO_LIST *head, LTOPO_LIST *node);
int list_remove(LTOPO_LIST *head, LTOPO_LIST *node);

extern int list_fini(LTOPO_LIST *head, list_handler_t fini);
extern int list_show(LTOPO_LIST *head, list_handler_t show);

#ifdef  __cplusplus
}
#endif
#endif /* _LIST_H */

