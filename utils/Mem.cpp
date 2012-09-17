/**
 *  Copyright (c) 2009-2011, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

/*
 * @file Mem.cpp
 *
 * @brief memory utils
 *
 * @ingroup utils
 */

#include <stdio.h>
#include "Mem.h"

#include "Queue.h"

#ifdef ANDROID_BUILD
#include "Log.h"
#define printf LogOutput
#endif

#ifdef OMX_MEM_CHECK

#define DEFAULT_SHORT_NAME_OF_MM  "UM"
#define STR_LEN 64

#define MEM_CHECK_BOARDER
#ifdef MEM_CHECK_BOARDER
#define PRE_SIZE 64
#define AFT_SIZE 64
#define MAGICNUMBER 0xab
#endif


typedef struct _memdesc{
    struct _memdesc * next;
    fsl_osal_u32 size;
    fsl_osal_ptr mem;
    fsl_osal_char desstring[STR_LEN];
    fsl_osal_s32 line;
    fsl_osal_u32 age;
}Mem_Desc;

typedef struct _timestampmanager{
    Mem_Desc * allocatedbuffer;
    Mem_Desc * freelist;
    Mem_Desc * head;
    Mem_Desc * tail;
    fsl_osal_u32 age;
    fsl_osal_u32 size;
    fsl_osal_u32 maxsize;
    fsl_osal_u32 allocatednum; 
    fsl_osal_char * shortname;
}Mem_Mgr;

#define COPYMEMORYDESC(des, src) \
    do { \
        des->size = src->size; \
        des->mem = src->mem; \
        des->line = src->line;\
    }while(0)


Mem_Mgr g_tm;
Mem_Mgr *tm = &g_tm;


fsl_osal_void init_memmanager(fsl_osal_char * shortname)
{
    fsl_osal_memset(tm, 0, sizeof(Mem_Mgr));
    if (shortname){
        tm->shortname = shortname;
    }else{
        tm->shortname = DEFAULT_SHORT_NAME_OF_MM;
    }

    tm->allocatednum = 5000;
}

static fsl_osal_void printandfree_allnonfreememory(Mem_Mgr *tm)
{
    Mem_Desc * bt = tm->head;
    printf("%s: Non-freed memory list:\n", tm->shortname);
    while(bt){
        printf("\tmem %p\tsize = %ld\tage = %ld", bt->mem, bt->size, bt->age);
        if (bt->desstring){
            printf(" desc:  %s:%d\n",bt->desstring, bt->line);
        }else{
            printf("\n");
        }
        bt=bt->next;
    }
    printf("%s: End.\n", tm->shortname);
}
    
fsl_osal_void deinit_memmanager()
{
    printandfree_allnonfreememory(tm);
    if (tm->allocatedbuffer){
        fsl_osal_dealloc(tm->allocatedbuffer);
    }
    fsl_osal_memset(tm, 0, sizeof(Mem_Mgr));
}

fsl_osal_void clear_memmanager()
{
    fsl_osal_u32 i;
    Mem_Desc * bt = tm->allocatedbuffer;
    tm->freelist = tm->head = tm->tail = NULL;
    for (i=0;i<tm->allocatednum;i++){
        bt->next = tm->freelist;
        tm->freelist = bt;
        bt++;
    }
}

static Mem_Desc * new_Mem_Desc()
{
    Mem_Desc * newbuffer = NULL;
    if (tm->freelist){
        newbuffer = tm->freelist;
        tm->freelist = newbuffer->next;
        return newbuffer;
    }
    if (tm->allocatednum)
        tm->allocatednum <<=1;
    else
        tm->allocatednum = 1000;
    if ((newbuffer = (Mem_Desc *)fsl_osal_malloc_new(sizeof(Mem_Desc)*tm->allocatednum)) != NULL) {
        Mem_Desc *oldhead, *nb;
        fsl_osal_u32 i = 0;
        
        oldhead = tm->head;
        nb = newbuffer;
        tm->freelist = tm->head = tm->tail = NULL;
        for (i=0;i<(tm->allocatednum-1);i++){
            if (oldhead){
                COPYMEMORYDESC(nb, oldhead);
                nb->next = NULL;
                if (tm->tail){
                    (tm->tail)->next = nb;
                    tm->tail = nb;
                }else{
                    tm->head = tm->tail = nb;
                }
                oldhead = oldhead->next;
            }else{
                nb->next = tm->freelist;
                tm->freelist = nb;
            }
            nb++;
        }
        if (tm->allocatedbuffer){
            fsl_osal_dealloc(tm->allocatedbuffer);
        }
        tm->allocatedbuffer = newbuffer;
        return nb;
    }else{
        return newbuffer;
    }
}

#ifdef MEM_CHECK_BOARDER
fsl_osal_void check_mem_boarder(Mem_Desc *bt)
{
    fsl_osal_s32 i;
    fsl_osal_char *buf = (fsl_osal_char *)(bt->mem);
    fsl_osal_char *pre, *after;

    pre = buf-PRE_SIZE;
    for(i=0; i<PRE_SIZE; i++) {
        if(pre[i] != MAGICNUMBER) {
            printf("Memory up overflow from[%s:%d], age: %d\n",
                    bt->desstring, bt->line, bt->age);
            break;
        }
    }

    after = pre + (bt->size - AFT_SIZE);
    for(i=0; i<AFT_SIZE; i++) {
        if(after[i] != MAGICNUMBER) {
            printf("Memory down overflow from[%s:%d], age: %d\n",
                    bt->desstring, bt->line, bt->age);
            break;
        }
    }
}
#endif


fsl_osal_ptr dbg_malloc(fsl_osal_u32 size, fsl_osal_char * desc, fsl_osal_s32 line)
{
    Mem_Desc * bt;
    fsl_osal_ptr buf = NULL;

#ifdef MEM_CHECK_BOARDER
    fsl_osal_ptr ptr = NULL;
    fsl_osal_s32 size1 = size;

    size += PRE_SIZE + AFT_SIZE;
#endif

    if ((buf = fsl_osal_malloc_new(size)) && (bt = new_Mem_Desc())){
        tm->age++;
        tm->size+=size;
        if (tm->size>tm->maxsize){
            tm->maxsize = tm->size;
            //printf("%s: mem exceed %ld bytes\n", tm->shortname, tm->maxsize);
        }

#ifdef MEM_CHECK_BOARDER
        fsl_osal_memset(buf, MAGICNUMBER, PRE_SIZE);
        ptr = (fsl_osal_ptr)((fsl_osal_u32)buf + PRE_SIZE + size1);
        fsl_osal_memset(ptr, MAGICNUMBER, AFT_SIZE);
        buf = (fsl_osal_ptr)((fsl_osal_u32)buf + PRE_SIZE);
#endif
        
        bt->size = size;
        bt->age = tm->age;
        bt->mem = buf;
		bt->line = line;
        bt->next = NULL;

        fsl_osal_memcpy(bt->desstring, desc, STR_LEN);
        bt->desstring[STR_LEN-1] = '\0';
        //printf("age: %d, in %s:%d allocate %p, size %d.\n",bt->age, desc, bt->line, (fsl_osal_s32)buf, size);
		
        if (tm->tail){
            (tm->tail)->next = bt;
            tm->tail = bt;
        }else{
            tm->head = tm->tail = bt;
        }
    }else{
        if (buf){
            fsl_osal_dealloc(buf);
            buf = NULL;
        }else{
            printf("%s: FATAL ERROR - Can not allocate %ld bytes\n", tm->shortname, size);
        }
        printf("FATAL ERROR: Can not allocate memory for memmanager!!\n");
    }

    return buf;
}


fsl_osal_ptr dbg_realloc(fsl_osal_ptr ptr, fsl_osal_u32 size, fsl_osal_char * desc, fsl_osal_s32 line)
{
    Mem_Desc * bt = tm->head;
    fsl_osal_ptr buf = NULL;

    if(ptr == NULL)
        return dbg_malloc(size, desc, line);

    if(size == 0) {
        dbg_free(ptr);
        return NULL;
    }

    //find the mem descripter for ptr
    while(bt) {
        if (bt->mem==ptr)
            break;
        bt=bt->next;
    }

    buf = dbg_malloc(size, desc, line);
    if(buf) {
        fsl_osal_memcpy(buf, ptr, bt->size - (PRE_SIZE + AFT_SIZE));
        dbg_free(ptr);
    }
    else
        dbg_free(ptr); //FIXME

    return buf;
}

fsl_osal_void dbg_free(fsl_osal_ptr mem)
{
    Mem_Desc * bt = tm->head, *btpr = NULL;
    fsl_osal_ptr ptr = mem;
    fsl_osal_s32 size = 0;
    while(bt){
        if (bt->mem==mem){
            size = bt->size;
#ifdef MEM_CHECK_BOARDER
            check_mem_boarder(bt);
            mem = (fsl_osal_ptr)((fsl_osal_u32)mem-PRE_SIZE);
            size -= PRE_SIZE + AFT_SIZE;
#endif
            tm->size-=bt->size;
            fsl_osal_memset(ptr, 0, size);
            fsl_osal_dealloc(mem);
            if (btpr){
                btpr->next = bt->next;
                if (tm->tail==bt){
                    tm->tail = btpr;
                }
            }else{//head
                tm->head = bt->next;
                if (tm->head==NULL){
                    tm->tail = NULL;
                }
            }
            bt->next = tm->freelist;
            tm->freelist = bt;
            return;
        }
        btpr = bt;
        bt=bt->next;
    }
    printf("%s Error memory freed pointer  = %p\n",tm->shortname, mem);
}


#ifdef __cplusplus
void * operator new(unsigned int size, char* file, int line)
{
    return dbg_malloc(size, file, line);
}

void operator delete(fsl_osal_ptr ptr, char* file, int line)
{
    operator delete(ptr);
}

#if 0
void * operator new(unsigned int size)
{
    return operator new(size, "Unknown", 0);
}
#endif

void operator delete(fsl_osal_ptr ptr)
{
    dbg_free(ptr);
}
#endif

#endif

