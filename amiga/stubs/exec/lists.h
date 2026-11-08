#ifndef EXEC_LISTS_H
#define EXEC_LISTS_H

#include "exec/exec.h"

#define EXEC_LISTS_H

void NewList(struct List* list);
void AddHead(struct List* list, struct Node* node);
void AddTail(struct List* list, struct Node* node);
struct Node* RemHead(struct List* list);
struct Node* RemTail(struct List* list);
void Remove(struct Node* node);
void Enqueue(struct List* list, struct Node* node);
struct Node* FindName(struct List* list, const char* name);

void NewMinList(struct MinList* list);
void AddHeadMin(struct MinList* list, struct MinNode* node);
void AddTailMin(struct MinList* list, struct MinNode* node);
struct MinNode* RemHeadMin(struct MinList* list);
struct MinNode* RemTailMin(struct MinList* list);
void RemoveMin(struct MinNode* node);
void EnqueueMin(struct MinList* list, struct MinNode* node);

#endif /* EXEC_LISTS_H */