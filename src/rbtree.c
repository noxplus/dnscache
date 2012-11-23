#include "inc.h"

#define  CRED   0x1 //最低1bit
#define  CBLACK 0x0
#define  LEFT   0x2 //次低1bit
#define  RIGHT  0x0

#define SetBlack(node) (node->Color &= ~CRED)
#define SetRed(node) (node->Color |= CRED)
#define SetLeft(node) (node->Color |= LEFT)
#define SetRight(node) (node->Color &= ~LEFT)

int GetColor(RBNode* node)
{
    if (node == NULL) return CBLACK;
    return node->Color & CRED;
}
int GetLeftRight(RBNode* node)
{
    if (node == NULL) return LEFT;
    return node->Color & LEFT;
}

int Comp(DNSRecode* val1, DNSRecode* val2)
{
    if (val1->index != val2->index)
        return val1->index - val2->index;

    return strncmp(val1->uname.cname, val2->uname.cname, NAMEMAXLEN);
}

void LeftRote(RBRoot* TreeRoot, RBNode* node)
{
    //r->p; p->l; rl->lr
    RBNode* rchd;
    RBNode* rchdleft;

    rchd = node->Right;
    rchdleft = rchd->Left;

    rchd->Parent = node->Parent;
    if (node->Parent != NULL)
    {
        if (GetLeftRight(node) == LEFT)
        {
            node->Parent->Left = rchd;
            SetLeft(rchd);
        }
        else
        {
            node->Parent->Right = rchd;
            SetRight(rchd);
        }
    }
    else
    {
        TreeRoot->Root = rchd;
        SetBlack(rchd);
    }
    rchd->Left = node;
    node->Parent = rchd;
    SetLeft(node);
    node->Right = rchdleft;
    if (rchdleft != NULL)
    {
        rchdleft->Parent = node;
        SetRight(rchdleft);
    }

    return;
}
void RightRote(RBRoot* TreeRoot, RBNode* node)
{
    //l->p; p->r; lr->rl
    RBNode* lchd;
    RBNode* lchdright;

    lchd = node->Left;
    lchdright = lchd->Right;

    lchd->Parent = node->Parent;
    if (node->Parent != NULL)
    {
        if (GetLeftRight(node) == LEFT)
        {
            node->Parent->Left = lchd;
            SetLeft(lchd);
        }
        else
        {
            node->Parent->Right = lchd;
            SetRight(lchd);
        }
    }
    else
    {
        TreeRoot->Root = lchd;
        SetBlack(lchd);
    }
    lchd->Right = node;
    node->Parent = lchd;
    SetRight(node);
    node->Left = lchdright;
    if (lchdright != NULL)
    {
        lchdright->Parent = node;
        SetLeft(lchdright);
    }

    return;
}

int CheckNode(RBRoot* TreeRoot, RBNode* nNode)
{
    RBNode*     pNode = nNode->Parent;

    if (pNode == NULL)
    {
        TreeRoot->Root = nNode;
        SetBlack(nNode);
        return 0;
    }
    if (GetColor(nNode) == CBLACK || GetColor(pNode) == CBLACK) return 0;

    RBNode*     gNode = pNode->Parent;

    if (GetColor(gNode->Left) == CRED && GetColor(gNode->Right) == CRED)
    {
        SetBlack(gNode->Left);
        SetBlack(gNode->Right);
        SetRed(gNode);
        return CheckNode(TreeRoot, gNode);
    }

    if (GetLeftRight(nNode) == LEFT && GetLeftRight(pNode) == LEFT)
    {
        RightRote(TreeRoot, gNode);
        SetRed(gNode);
        SetBlack(pNode);
        return 0;
    }
    if (GetLeftRight(nNode) == RIGHT && GetLeftRight(pNode) == RIGHT)
    {
        LeftRote(TreeRoot, gNode);
        SetRed(gNode);
        SetBlack(pNode);
        return 0;
    }
    if (GetLeftRight(nNode) == RIGHT && GetLeftRight(pNode) == LEFT)
    {
        LeftRote(TreeRoot, pNode);
        RightRote(TreeRoot, gNode);
        SetRed(gNode);
        SetRed(pNode);
        SetBlack(nNode);
        return 0;
    }
    if (GetLeftRight(nNode) == LEFT && GetLeftRight(pNode) == RIGHT)
    {
        RightRote(TreeRoot, pNode);
        LeftRote(TreeRoot, gNode);
        SetRed(gNode);
        SetRed(pNode);
        SetBlack(nNode);
        return 0;
    }

    return 0;//
}

//插入：有相同的则覆盖，否则插入新节点
int RBTreeInsert(RBRoot* TreeRoot, DNSRecode* nVal)
{
    RBNode* pNode = TreeRoot->Root;
    RBNode* nNode = NULL;

    int iret = 0;

    if (TreeRoot->Root == NULL)
    {
        nNode = (RBNode*)malloc(sizeof(RBNode));
        nNode->Value = (DNSRecode*)malloc(sizeof(DNSRecode));
        memcpy(nNode->Value, nVal, sizeof(DNSRecode));
        nNode->Parent = NULL;
        nNode->Left = NULL;
        nNode->Right = NULL;
        TreeRoot->Root = nNode;
        SetBlack(nNode);
        return 0;
    }

    for(;;)
    {
        iret = Comp(pNode->Value, nVal);
        if (iret == 0)//相同，覆盖内容
        {
            memcpy(pNode->Value, nVal, sizeof(DNSRecode));
            return 0;
        }
        else if (iret < 0)
        {
            if (pNode->Right == NULL)
            {
                nNode = (RBNode*)malloc(sizeof(RBNode));
                nNode->Value = (DNSRecode*)malloc(sizeof(DNSRecode));
                memcpy(nNode->Value, nVal, sizeof(DNSRecode));
                pNode->Right = nNode;
                nNode->Parent = pNode;
                nNode->Left = NULL;
                nNode->Right = NULL;
                SetRed(nNode);
                SetRight(nNode);
                break;
            }
            pNode = pNode->Right;
        }
        else
        {
            if (pNode->Left == NULL)
            {
                nNode = (RBNode*)malloc(sizeof(RBNode));
                nNode->Value = (DNSRecode*)malloc(sizeof(DNSRecode));
                memcpy(nNode->Value, nVal, sizeof(DNSRecode));
                pNode->Left = nNode;
                nNode->Parent = pNode;
                nNode->Left = NULL;
                nNode->Right = NULL;
                SetRed(nNode);
                SetLeft(nNode);
                break;
            }
            pNode = pNode->Left;
        }
    }

    return CheckNode(TreeRoot, nNode);
}

void* RBTreeSearch(RBRoot* TreeRoot, DNSRecode* sVal)
{
    RBNode*     pNode = TreeRoot->Root;
    int iret;

    if (TreeRoot->Root == NULL)
    {
        return 0;
    }
    for(;;)
    {
        iret = Comp(pNode->Value, sVal);
        if (iret == 0)//find!
        {
            return pNode->Value;
        }
        else if (iret < 0)
        {
            if (pNode->Right == NULL)
            {
                return NULL;
            }
            pNode = pNode->Right;
        }
        else
        {
            if (pNode->Left == NULL)
            {
                return NULL;
            }
            pNode = pNode->Left;
        }
    }
}

void traver(RBRoot* TreeRoot)
{
    int deep = 0;
    int ldeep = 0;
    RBNode* node = TreeRoot->Root;
    RBNode* prev = NULL;
    RBNode* next = NULL;
    if (TreeRoot->Root == NULL) printf("trav: root = NULL\n");
    else printf("trav: root = %03ld\n", TreeRoot->Root->Color);
    while(node != NULL)
    {
        if (prev == node->Parent)
        {
            prev = node;
            next = node->Left;
            deep = ldeep + 1;
        }
        if (next == NULL || prev == node->Left)
        {
            int i;
            for (i = 0; i < ldeep; i++)
                printf("   ");
            if (GetColor(node) == CRED)
                printf("\033[0;31m%03ld\033[m\n", node->Color);
            else
                printf("\033[0;32m%03ld\033[m\n", node->Color);
            prev = node;
            next = node->Right;
            deep = ldeep + 1;
        }
        if (next == NULL || prev == node->Right)
        {
            prev = node;
            next = node->Parent;
            deep = ldeep - 1;
        }
        node = next;
        ldeep = deep;
    }
    printf("~fin~\n");
}

#ifdef ONLY_RUN
int main(int argc, char** argv)
{
    int i = atoi(argv[1]);
    RBRoot root;

    root.Root = NULL;
    RBNode *node = NULL;
    for (; i>0; i--)
    {
        node = (RBNode*)malloc(sizeof(RBNode));
        node->Key = random32()%10000;
//        printf("%03lu ", node->Key);
//        printf("input:");
//        scanf("%lu", &node->Key);
        RBTreeInsert(&root, node);
    }
//    printf("\n");
    traver(&root);
    return 0;
}
#endif
