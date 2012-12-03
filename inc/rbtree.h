#ifndef _RBTREE_H_
#define _RBTREE_H_

#include "transdns.h"
typedef struct _tp_rbtree_node RBNode;
struct _tp_rbtree_node
{
    RBNode*     Parent;
    RBNode*     Left;
    RBNode*     Right;
    DNSRecode*  Value;
    uint32      Color;
};
typedef struct _tp_rbtree_root
{
    RBNode*     Root;
}RBRoot;

//RBTree
void* RBTreeSearch(RBRoot*, DNSRecode*);
int RBTreeInsert(RBRoot*, DNSRecode*);

#endif
