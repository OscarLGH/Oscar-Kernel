#ifndef _LIST_H
#define _LIST_H

#include "Irq.h"

typedef struct _LIST_HEAD
{
	struct _LIST_HEAD * Next;
	struct _LIST_HEAD * Prev;
	//SPIN_LOCK ListSpinLock;
}LIST_HEAD;

#define LIST_HEAD_INIT(name) {&(name), &(name)}
#define LIST_HEAD_DEF(name) LIST_HEAD name = LIST_HEAD_INIT(name)

static inline void INIT_LIST_HEAD(LIST_HEAD * List)
{
	List->Next = List;
	List->Prev = List;
	//List->ListSpinLock.SpinLock = 0;
}

static inline int ListEmpty(const LIST_HEAD * Head)
{
	return Head->Next == Head;
}

static inline void _ListAdd(LIST_HEAD * New, LIST_HEAD * Prev, LIST_HEAD * Next)
{
	Next->Prev = New;
	New->Next = Next;
	New->Prev = Prev;
	Prev->Next = New;
}

static inline void ListAdd(LIST_HEAD * New, LIST_HEAD * Head)
{
	_ListAdd(New, Head, Head->Next);
}

static inline void ListAddTail(LIST_HEAD * New, LIST_HEAD * Head)
{
	_ListAdd(New, Head->Prev, Head);
}

static inline void _ListDelete(LIST_HEAD * Prev, LIST_HEAD * Next)
{
	Next->Prev = Prev;
	Prev->Next = Next;
}

static inline void ListDelete(LIST_HEAD * Entry)
{
	_ListDelete(Entry->Prev, Entry->Next);
	Entry->Next = (void *)0x00100100;
	Entry->Prev = (void *)0x00200200;
}

#define offsetof(TYPE, MEMBER)((unsigned long long) &((TYPE *)0)->MEMBER)

#define ContainerOf(Ptr, Type, Member)({	\
	const typeof(((Type *)0)->Member) * __mPtr = (void *)(Ptr);	\
	(Type *)((char *)__mPtr - offsetof(Type, Member));	\
})

#define ListEntry(Ptr, Type, Member) \
	ContainerOf(Ptr, Type, Member)

#define ListFirstEntry(Ptr, Type, Member) \
	ListEntry((Ptr)->Next, Type, Member)

#define ListNextEntry(Pos, Member) \
	ListEntry((Pos)->Member.Next, typeof(*(Pos)), Member)

#define ListForEachEntry(Pos, Head, Member)	\
	for(Pos = ListFirstEntry(Head, typeof(*Pos), Member);	\
		&Pos->Member != (Head);	\
		Pos = ListNextEntry(Pos, Member))

#endif