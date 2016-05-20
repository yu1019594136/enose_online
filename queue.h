#ifndef QUEUE_H
#define QUEUE_H

#include <QDebug>
#include <QString>
#include <QStringList>

typedef struct NODE
{
    QString *dat;
    struct NODE *next;
} Node;

typedef struct QUEUE
{
    Node *head;
    Node *tail;
}Queue;

//初始化队列
Queue *InitQueue();

//向队列末尾插入一个元素
void InsertQueue(Queue *queue, QString element);

//从队首删除一个元素
void DelQueue(Queue *queue);

//获取队首元素
QString * GetQueueHead(Queue *queue);

//获取队列前i个元素
QStringList GetQueue_ahead_element(Queue *queue, int num);

//清除队列中所有元素
void ClearQueue(Queue *queue);

//获取队列长度
int GetQueueLength(Queue *queue);

#endif // QUEUE_H
