#include "hatchList.h"
#include <iostream>

CHatchNode::CHatchNode()
{
	m_cpDataBuffer = NULL;
	m_ulDataLength = 0;
}

CHatchNode::~CHatchNode()
{
	if(m_cpDataBuffer != NULL)
	{
		delete[] m_cpDataBuffer;
		m_cpDataBuffer = NULL;
	}
}

CHatchNode::CHatchNode(char *data,unsigned long length)
{
	try
	{
		if(NULL == data)
		{
			m_cpDataBuffer = NULL;
		}

		char *temp = new char[length];
		if(NULL == temp)
		{
			FAI_LOG("memory isn't enough.");
			exit(0);
		}

		memcpy(temp,data,length);
		m_ulDataLength = length;
		m_cpDataBuffer = temp;
	}
	catch(...)
	{
		FAI_LOG("Node operator is failure.");
	}
}
	
CHatchNode::CHatchNode(const CHatchNode &node)
{
	try
	{
		if(node.m_cpDataBuffer == NULL)
		{
			m_cpDataBuffer = NULL;
		}

		char *temp = new char[node.m_ulDataLength];
		if(NULL == temp)
		{
			FAI_LOG("memory isn't enough.");
			exit(0);
		}

		memcpy(m_cpDataBuffer,node.m_cpDataBuffer,node.m_ulDataLength);
		m_ulDataLength = node.m_ulDataLength;
		m_cpDataBuffer = temp;
	}
	catch(...)
	{
		FAI_LOG("Node operator is failure.");
	}
}

CHatchNode & CHatchNode::operator=(const CHatchNode & node)
{//链路指针无处理，保持原位
	try
	{
		if(m_cpDataBuffer != NULL)
		{
			delete[] m_cpDataBuffer;
			m_cpDataBuffer = NULL;
			m_ulDataLength = NULL;
		}

		if(node.m_cpDataBuffer == NULL)
		{
			m_cpDataBuffer = NULL;
		}

		char *temp = new char[node.m_ulDataLength];
		if(NULL == temp)
		{
			FAI_LOG("memory isn't enough.");
			exit(0);
		}

		memcpy(m_cpDataBuffer,node.m_cpDataBuffer,node.m_ulDataLength);
		m_ulDataLength = node.m_ulDataLength;
		m_cpDataBuffer = temp;

		return *this;
	}
	catch(...)
	{
		FAI_LOG("Node operator is failure.");
	}
}

unsigned long CHatchNode::hatchGetLength() const
{
	return m_ulDataLength;
}

void CHatchNode::hatchGetDataBuffer(char **buf)
{
	try
	{
		char *tempBuf = new char[m_ulDataLength];
		if(NULL == tempBuf)
		{
			*buf = NULL;
			FAI_LOG("memory isn't enough.");
			return;
		}

		memcpy(tempBuf,m_cpDataBuffer,m_ulDataLength);
		*buf = tempBuf;

		return;
	}
	catch(...)
	{
		FAI_LOG("Node operator is failure.");
	}
}

///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////

CHatchLoop::CHatchLoop()
{
	m_ulLoopLength = 0;
}

CHatchLoop::~CHatchLoop()
{
	pHatchNode temp = NULL;
	while(m_ulLoopLength > 0)
	{
		hatchPopFront(&temp);
		delete[] temp;
	}

	temp = NULL;
}

int CHatchLoop::hatchPushBack(CHatchNode *node)
{
	if(NULL != node)
	{
		m_listLoop.push_back(node);
		m_ulLoopLength++;
	}
	else
		WAR_LOG("node is null.");

	return 0;
}

int CHatchLoop::hatchPopFront(CHatchNode **node)
{
	pHatchNode &temp = m_listLoop.front();
	*node = temp;
	m_listLoop.pop_front();
	m_ulLoopLength--;

	return 0;
}

bool CHatchLoop::hatchIsEmpty()
{
	return ((m_ulLoopLength > 0) ? true : false);
}
	
unsigned long CHatchLoop::hatchSize()
{
	return m_ulLoopLength;
}

LoopIterator CHatchLoop::hatchIteratorBegin()
{
	return m_listLoop.begin();
}

LoopIterator CHatchLoop::hatchIteratorEnd()
{
	return m_listLoop.end();
}

