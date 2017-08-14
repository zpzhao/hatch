#ifndef HATCHLIST_H_H
#define HATCHLIST_H_H

#include "hatchLog.h"
#include "hatchCommonFile.h"
using namespace std;

class CHatchNode
{
public:
	CHatchNode();
	CHatchNode(char *data,unsigned long length);
	CHatchNode(const CHatchNode &node);
	virtual ~CHatchNode();

	CHatchNode & operator=(const CHatchNode & node);
	unsigned long hatchGetLength() const;
	void hatchGetDataBuffer(char **buf);
private:
	char *m_cpDataBuffer;
	unsigned long m_ulDataLength;
};

typedef CHatchNode *pHatchNode;
typedef list<pHatchNode>::iterator LoopIterator;

class CHatchLoop
{
public:
	CHatchLoop();
	virtual ~CHatchLoop();

public:
	int hatchPushBack(CHatchNode *node);
	int hatchPopFront(CHatchNode **node);
	bool hatchIsEmpty();
	unsigned long hatchSize();
	LoopIterator hatchIteratorBegin();
	LoopIterator hatchIteratorEnd();

public:
	LoopIterator m_listIterator;
private:	
	list<pHatchNode> m_listLoop;
	unsigned long m_ulLoopLength;
};

#endif HATCHLIST_H_H