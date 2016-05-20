#include "queue.h"
#include<stdlib.h>

//初始化队列
Queue *InitQueue()
{
    Queue *queue = NULL;

    queue = new Queue;

    queue->head = queue->tail = NULL;

    return queue;
}

//向队列末尾插入一个元素
void InsertQueue(Queue *queue, QString element)
{
    Node *new_node = NULL;

    if(queue != NULL)//参数检查
    {
        //得到一个新节点
        new_node = new Node;

        new_node->dat = new QString(element);
        new_node->next = NULL;

        if(queue->tail == NULL)//如果队尾为空，表示队列为空
        {
            queue->head = new_node;
            queue->tail = new_node;
        }
        else//非空队列，则将新节点添加到队列尾部
        {
            queue->tail->next = new_node;
            queue->tail = new_node;
        }
    }
}

//从队首删除一个元素
void DelQueue(Queue *queue)
{
    Node *p = NULL;

    if(queue != NULL)//参数检查
    {
        if(GetQueueLength(queue) == 1)//只有一个元素
        {
            delete queue->head->dat;//释放QString *指向的空间
            delete queue->head;//释放Node *指向的空间
            queue->head = queue->tail = NULL;
        }
        else if(GetQueueLength(queue) > 1)//一个以上的元素
        {
            delete queue->head->dat;//释放QString *指向的空间
            p = queue->head;//暂存节点地址
            queue->head = queue->head->next;
            delete p;
        }
    }
}

//获取队首元素
QString* GetQueueHead(Queue *queue)
{
    if(queue)
    {
        if(queue->head)
            return queue->head->dat;
    }
    return NULL;
}

//获取队列前i个元素
QStringList GetQueue_ahead_element(Queue *queue, int num)
{
    Node *p = NULL;
    p = queue->head;
    QStringList arguments;

    if(queue && GetQueueLength(queue) >= num)
    {
        for(int i = 0; i < num; i++)
        {
            arguments << *(p->dat);
            p = p->next;
        }
    }

    return arguments;
}

//清除队列中所有元素
void ClearQueue(Queue *queue)
{
    if(queue)//参数检查
    {
        Node *p = NULL;
        p = queue->head;
        int len = GetQueueLength(queue);

        //队列长度为0无需清理
        if(len == 1)//只有一个元素
        {
            delete queue->head->dat;//释放QString *指向的空间
            delete queue->head;//释放Node *指向的空间
            queue->head = queue->tail = NULL;
        }
        else if(len > 1)//一个以上的元素
        {
            while(queue->head != queue->tail)
            {
                delete queue->head->dat;//释放QString *指向的空间
                p = queue->head;//暂存节点地址
                queue->head = queue->head->next;
                delete p;
            }
            //还剩余一个节点
            delete queue->head->dat;
            delete queue->head;
            queue->head = queue->tail = NULL;
        }
    }
}

//获取队列长度
int GetQueueLength(Queue *queue)
{
    if(queue == NULL)
        return -1;

    int len = 0;
    Node *p = queue->head;

    if(queue->tail == NULL)//队列为空
    {
        return len;
    }
    else if(queue->head == queue->tail && queue->head != NULL)//队列只有一个元素
    {
        len = 1;
    }
    else//队列不止一个元素
    {
        len++;
        while(p != queue->tail)
        {
            p = p->next;
            len++;
        }
    }

    return len;
}
