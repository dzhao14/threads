#include <stdlib.h>
#include <assert.h>

#include "queue.h"

queue* 
make_queue()
{
    queue* qq = malloc(sizeof(queue));
    qq->head = 0;
    qq->tail = 0;
    return qq;
}

void 
free_queue(queue* qq)
{
    assert(qq->head == 0 && qq->tail == 0);
    free(qq);
}

void 
queue_put(queue* qq, void* msg)
{
    qnode* node = malloc(sizeof(qnode));
    node->data = msg;
    node->prev = 0;
    node->next = 0;
    
    node->next = qq->head;
    qq->head = node;

    if (node->next) {
        node->next->prev = node;
    } 
    else {
        qq->tail = node;
    }
}

void* 
queue_get(queue* qq)
{
    if (!qq->tail) {
        // FIXME: We should block here.
        return 0;
    }

    qnode* node = qq->tail;

    if (node->prev) {
        qq->tail = node->prev;
        node->prev->next = 0;
    }
    else {
        qq->head = 0;
        qq->tail = 0;
    }

    void* msg = node->data;
    free(node);
    return msg;
}


