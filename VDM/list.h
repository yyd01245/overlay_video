
#ifndef _H_ZLIST_
#define _H_ZLIST_

#ifdef __cplusplus
extern "C"{
#endif



#include<stdio.h>
#include<stdlib.h>

struct list_head{
    struct list_head*next,*prev;
};



#define LIST_HEAD_INIT(name) {&(name),&(name)}

static inline void INIT_LIST_HEAD(struct list_head*node){

    (node)->next=(node);
    (node)->prev=(node);
}




static inline void __list_add(struct list_head*_new,struct list_head*prev,struct list_head*next){

    (prev)->next=(_new);
    (next)->prev=(_new);
    (_new)->prev=(prev);
    (_new)->next=(next);

}



/**
 * list_add - add a _new entry
 * @_new: _new entry to be added
 * @head: list head to add it after
 *
 * Insert a _new entry after the specified head.
 * This is good for implementing stacks.
 */
static void list_add(struct list_head*head,struct list_head*_new)
{
    __list_add(_new,head,head->next);
}


/**
 * list_add_tail - add a _new entry
 * @_new: _new entry to be added
 * @head: list head to add it before
 *
 * Insert a _new entry before the specified head.
 * This is useful for implementing queues.
 */
static void list_add_tail(struct list_head*head,struct list_head*_new)
{
    __list_add(_new,head->prev,head);
}



static inline void __list_del(struct list_head*prev,struct list_head*next){

    (prev)->next=(next);
    (next)->prev=(prev);

}

/**
 * list_del - deletes entry from list.
 * @entry: the element to delete from the list.
 * Note: list_empty() on entry does not return true after this, the entry is
 * in an undefined state.
 */
static void list_del(struct list_head*pos)
{

    __list_del(pos->prev,pos->next);
    pos->prev=NULL;
    pos->next=NULL;

}


/**
 * list_empty - tests whether a list is empty
 * @head: the list to test.
 */

static inline int list_empty(const struct list_head *head)
{
	return head->next == head;
}



static inline void __list_splice(const struct list_head *list,
				 struct list_head *prev,
				 struct list_head *next)
{
	struct list_head *first = list->next;
	struct list_head *last = list->prev;

	first->prev = prev;
	prev->next = first;

	last->next = next;
	next->prev = last;
}

/**
 * list_splice - join two lists, this is designed for stacks
 * @list: the _new list to add.
 * @head: the place to add it in the first list.
 */
static void list_splice(const struct list_head *list,
				struct list_head *head)
{
	if (!list_empty(list))
		__list_splice(list, head, head->next);
}

/**
 * list_splice_tail - join two lists, each list being a queue
 * @list: the _new list to add.
 * @head: the place to add it in the first list.
 */
static void list_splice_tail(struct list_head *list,
				struct list_head *head)
{
	if (!list_empty(list))
		__list_splice(list, head->prev, head);
}




#define LIST_HEAD(name) \
    struct list_head name=LIST_HEAD_INIT(name)


#ifndef offsetof
#define offsetof(type,memb) \
    ((size_t)&((type*)0)->memb)
#endif


#define container_of(ptr,type,memb) ({\
    const typeof(((type*)0)->memb) *__mptr=(ptr);\
    (type*)((char*)__mptr-offsetof(type,memb));})


#define container_of_(ptr,type,memb) \
    (type*)((char*)ptr-offsetof(type,memb))



#define list_entry(ptr,type,memb) \
    container_of(ptr,type,memb)


#define list_entry_(ptr,type,memb) \
    container_of_(ptr,type,memb)



#define list_next_entry(epos, memb) \
	list_entry_((epos)->memb.next, typeof(*(epos)), memb)


#define list_prev_entry(epos, memb) \
	list_entry_((epos)->memb.prev, typeof(*(epos)), memb)



#define list_first_entry(ptr, type, memb) \
	list_entry_((ptr)->next, type, memb)


#define list_last_entry(ptr, type, memb) \
	list_entry_((ptr)->prev, type, memb)






#define list_for_each(pos, head) \
    for (pos = (head)->next; pos != (head); pos = pos->next) 


#define list_for_each_prev(pos, head) \
	for (pos = (head)->prev; pos != (head); pos = pos->prev)




#define list_for_each_safe(pos,savpos,head) \
    for ( pos = (head)->next,savpos=(pos)->next;\
          pos != (head);\
          pos = savpos, savpos = pos->next) 


#define list_for_each_prev_safe(pos, savpos, head) \
	for (pos = (head)->prev, savpos = pos->prev;\
	     pos != (head);\
	     pos = savpos, savpos = pos->prev)







#define list_for_each_entry(epos, head, memb) \
    for (epos = list_first_entry(head,typeof(*epos),memb);\
        &epos->memb != (head);\
        epos=list_next_entry(epos,memb))

#define list_for_each_entry_reverse(epos, head, memb) \
    for (epos = list_last_entry(head,typeof(*epos),memb);\
        &epos->memb != (head);\
        epos=list_prev_entry(epos,memb))




#define list_for_each_entry_safe(pos, savepos, head, member)			\
	for (pos = list_first_entry(head, typeof(*pos), member),	\
		savepos = list_next_entry(pos, member);			\
	     &pos->member != (head); 					\
	     pos = savepos, savepos = list_next_entry(savepos, member))

#define list_for_each_entry_safe_reverse(pos, savepos, head, member)			\
	for (pos = list_last_entry(head, typeof(*pos), member),	\
		savepos = list_prev_entry(pos, member);			\
	     &pos->member != (head); 					\
	     pos = savepos, savepos = list_prev_entry(savepos, member))














#ifdef __cplusplus
}
#endif


#endif
