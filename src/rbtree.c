#include "inc.h"

#define  RED    0x1 //ื๎ตอ1bit
#define  BLACK  0x0
#define  LEFT   0x2 //ดฮตอ1bit
#define  RIGHT  0x0

uint32 random32(void)
{
    uint32 uiret = 0;
    int fd = open("/dev/urandom", O_RDONLY);
    read(fd, &uiret, sizeof(uiret));
    close(fd);

    return uiret;
}

#define SetBlack(node) (node->Color &= ~RED)
#define SetRed(node) (node->Color |= RED)
#define SetLeft(node) (node->Color |= LEFT)
#define SetRight(node) (node->Color &= ~LEFT)

int GetColor(RBNode* node)
{
    if (node == NULL) return BLACK;
    return node->Color & RED;
}
int GetLeftRight(RBNode* node)
{
    if (node == NULL) return LEFT;
    return node->Color & LEFT;
}

int Diff(RBNode* node1, RBNode* node2)
{
    return node1->Key - node2->Key;
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
    if (GetColor(nNode) == BLACK || GetColor(pNode) == BLACK) return 0;

    RBNode*     gNode = pNode->Parent;

    if (GetColor(gNode->Left) == RED && GetColor(gNode->Right) == RED)
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

int insert(RBRoot* TreeRoot, RBNode* nNode)
{
    RBNode*     pNode = TreeRoot->Root;

    //init
    nNode->Parent = NULL;
    nNode->Left = NULL;
    nNode->Right = NULL;
    SetRed(nNode);

    if (TreeRoot->Root == NULL)
    {
        TreeRoot->Root = nNode;
        SetBlack(nNode);
        return 0;
    }

    for(;;)
    {
        if (Diff(pNode, nNode) < 0)
        {
            if (pNode->Right == NULL)
            {
                pNode->Right = nNode;
                nNode->Parent = pNode;
                SetRight(nNode);
                break;
            }
            pNode = pNode->Right;
        }
        else
        {
            if (pNode->Left == NULL)
            {
                pNode->Left = nNode;
                nNode->Parent = pNode;
                SetLeft(nNode);
                break;
            }
            pNode = pNode->Left;
        }
    }

    return CheckNode(TreeRoot, nNode);
}

int search(RBRoot* TreeRoot, int key)
{
    RBNode*     pNode = TreeRoot->Root;
    RBNode      sNode;
    int comp;

    sNode.Key = key;

    if (TreeRoot->Root == NULL)
    {
        return 0;
    }
    for(;;)
    {
        comp = Diff(pNode, &sNode);
        if (comp == 0)//find!
        {
            return pNode->Key;
        }
        else if (comp < 0)
        {
            if (pNode->Right == NULL)
            {
                return 0;
            }
            pNode = pNode->Right;
        }
        else
        {
            if (pNode->Left == NULL)
            {
                return 0;
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
    else printf("trav: root = %03ld\n", TreeRoot->Root->Key);
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
            if (GetColor(node) == RED)
                printf("\033[0;31m%03ld\033[m\n", node->Key);
            else
                printf("\033[0;32m%03ld\033[m\n", node->Key);
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
        insert(&root, node);
    }
//    printf("\n");
    traver(&root);
    return 0;
}
