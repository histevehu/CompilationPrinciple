// File Name: CharStack.h
#pragma once
#define MAX_DATA_LEN	256	// 数据缓冲区长度

// word type
#define WT_OPERATOR		0	// 操作符
#define WT_UINT			1	// 非负整数
#define WT_VARIABLE		2	// 变量

struct WORDNODE			// 单词序列节点
{
	unsigned short byType;		// 类别
	char Value[MAX_DATA_LEN];	// 值
	WORDNODE *pNext;			// 下一结点
};

struct CHARNODE
{
	char		cCur;		// 当前符号
	WORDNODE	*pWord;		// 单词节点
};

CHARNODE m_CharStack[1024];
int	m_nCharTop = -1;

void InitCharStack()
{
	m_nCharTop = -1;
}

void PushChar(char c, WORDNODE *pWord)
{
	++m_nCharTop;
	m_CharStack[m_nCharTop].cCur = c;
	m_CharStack[m_nCharTop].pWord = pWord;
}

CHARNODE* PopChar()
{
	return &m_CharStack[m_nCharTop--];
}

CHARNODE* TopChar()
{
	return &m_CharStack[m_nCharTop];
}

void PrintCharStack()
{
	int i;
	for (i = 0; i <= m_nCharTop; i++)
		printf("%c", m_CharStack[i].cCur);
	for (; i < 15; i++)
		printf(" ");
}