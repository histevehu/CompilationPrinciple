#include "stdafx.h"
#include "CharStack.h"
#include "IntStack.h"
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>

struct BNFNODE	// 产生式节点
{
	char	Left;					// 产生式左部
	char	Right[MAX_DATA_LEN];	// 产生式右部
	int		RLen;					// 产生式右边长度
} m_Bnf[MAX_DATA_LEN];
int	m_nBnfLen;

enum ACTIONTYPE	// 动作类别
{
	Push,		// 移进
	Sumup,		// 规约
	Accept,		// 接受
	Error		// 出错
};

struct LR1TABLE
{
	int		nStatus;		// 状态
	char	CurChar;		// 当前符号
	ACTIONTYPE	ActionType;	// 动作类别
	int		nNextStatus;	// 下一状态(push)或规约产生式序号(sumup)
} m_Lr1[MAX_DATA_LEN];
int		m_nLr1Len;

/*****************************************************
* 以下是词法分析文件操作
******************************************************/
// 清空链表
void ClearWords(WORDNODE* pHeader)
{
	WORDNODE* pNode;

	while (pHeader != NULL)
	{
		pNode = pHeader->pNext;
		free(pHeader);
		pHeader = pNode;
	}
}

// 增加结点
WORDNODE* AddNode(char c[], WORDNODE* pTail)
{
	// c第0个字节为单词类别，第1个为逗号，第2个以后是值

	WORDNODE* pNode = (WORDNODE*)malloc(sizeof(WORDNODE));
	pNode->byType = c[0] - '0';
	pNode->pNext = NULL;

	int nChars = MAX_DATA_LEN - 2;
	memcpy(pNode->Value, &c[2], nChars);

	pTail->pNext = pNode;
	return pNode;
}

bool ReadWords(char FileName[], WORDNODE* pHeader)
{
	// 打开文件
	FILE* f = fopen(FileName, "r");
	if (f == NULL)
	{
		ClearWords(pHeader);
		return false;
	}

	WORDNODE* pTail = pHeader;
	char c[MAX_DATA_LEN];

	// 读取数据
	while (!feof(f))
	{
		fscanf(f, "%s\n", c);
		pTail = AddNode(c, pTail);
		printf("%s\n", c);
	}

	// 关闭文件
	fclose(f);

	// 增加一个结束符
	c[0] = WT_OPERATOR + '0';
	c[1] = ',';
	c[2] = '#';
	c[3] = '\0';
	AddNode(c, pTail);

	return true;
}

/*****************************************************
* 以下是文法文件操作
******************************************************/
char* ReadFile(char FileName[], int* nLen)
{
	// 打开文件
	FILE* f = fopen(FileName, "r");
	if (f == NULL)
		return NULL;

	// 读取文件
	char* pChar = (char*)malloc(sizeof(char) * MAX_DATA_LEN);

	// 读取数据
	int nRead;
	*nLen = 0;
	while (!feof(f))
	{
		nRead = fread(pChar + *nLen, sizeof(char), MAX_DATA_LEN, f);
		*nLen += nRead;
		if (nRead < MAX_DATA_LEN)	// 文件结尾
			break;

		pChar = (char*)realloc(pChar, *nLen + sizeof(char) * MAX_DATA_LEN);
	}

	// 关闭文件
	fclose(f);

	return pChar;
}

bool ReadBnfs()
{
	// 读取文件
	int nLen;
	char* pChar = ReadFile("Bnf.txt", &nLen);
	if (pChar == NULL)
		return false;

	// 解析出文法产生式
	int nBegin, nCur, nIndex = 0;
	for (nBegin = 0, nCur = 0; nCur < nLen; nCur++)
	{
		// 左部
		m_Bnf[nIndex].Left = pChar[nCur];

		// 右部
		nCur += 2;
		nBegin = nCur;
		for (; pChar[nCur] != ';'; nCur++);
		m_Bnf[nIndex].RLen = nCur - nBegin;
		memcpy(m_Bnf[nIndex].Right, pChar + nBegin, m_Bnf[nIndex].RLen);
		m_Bnf[nIndex].Right[m_Bnf[nIndex].RLen] = '\0';

		nIndex++;
	}
	m_nBnfLen = nIndex;

	return true;
}

/*****************************************************
* 以下是LR(1)分析表文件操作
******************************************************/
ACTIONTYPE	GetActionType(char c)
{
	if (c >= '0' && c <= '9')
		return Push;

	switch (c)
	{
	case 'S':
		return Push;
	case 'r':
		return Sumup;
	case 'a':
		return Accept;
	}

	return Error;
}

bool ReadLR1()
{
	// 读取文件
	int nLen;
	char* pChar = ReadFile("Lr1.Lr1", &nLen);
	if (pChar == NULL)
		return false;

	// 解析出分析表
	int nBegin, nCur, nIndex = 0;
	for (nBegin = 0, nCur = 0; nCur < nLen; nCur++)
	{
		// 状态
		m_Lr1[nIndex].nStatus = atoi(&pChar[nCur]);
		for (; pChar[nCur] != ';'; nCur++);
		nCur++;

		// 符号
		m_Lr1[nIndex].CurChar = pChar[nCur];
		nCur += 2;

		// 动作类别
		m_Lr1[nIndex].ActionType = GetActionType(pChar[nCur]);
		if (pChar[nCur] == 'a')
		{
			nCur += 3;
			m_Lr1[nIndex].nNextStatus = 0;
		}
		else
		{
			if (pChar[nCur] == 'S' || pChar[nCur] == 'r')
				nCur++;
			// 状态转换
			m_Lr1[nIndex].nNextStatus = atoi(&pChar[nCur]);
			for (; pChar[nCur] != ';'; nCur++);
		}

		nIndex++;
	}
	m_nLr1Len = nIndex;

	return true;
}

/********************************************************
* 以下是语法分析部分
*********************************************************/
/************************************************
* 获取当前单词符号：
* 如果是整数或标识符，返回'i'
* 如果是算法，返回算法
*************************************************/
char GetCurChar(WORDNODE* pNode)
{
	switch (pNode->byType)
	{
	case WT_OPERATOR:	// 操作符
		return pNode->Value[0];
	case WT_UINT:		// 整数
	case WT_VARIABLE:	// 变量
		return 'i';
	}

	return '\0';
}

int GetLR1Index(int nStatus, char a)
{
	for (int i = 0; i < m_nLr1Len; i++)
	{
		if (m_Lr1[i].nStatus == nStatus && m_Lr1[i].CurChar == a)
			return i;
	}
	return -1;
}

// 打印状态
void PrintState(WORDNODE* pNode, int nBnfIndex)
{
	PrintIntStack();
	PrintCharStack();

	WORDNODE* pPrint = pNode;
	int nCount = 0;
	while (pPrint != NULL)
	{
		printf("%c", GetCurChar(pPrint));
		pPrint = pPrint->pNext;
		nCount++;
	}
	for (; nCount < 12; nCount++)
		printf(" ");

	if (nBnfIndex == -1)
	{
		if (GetCurChar(pNode) == 'i')
			printf("i=%s", pNode->Value);
	}
	else
		printf("%c-->%s", m_Bnf[nBnfIndex].Left, m_Bnf[nBnfIndex].Right);

	printf("\n");
}

void PopIntN(int n) {
	for (int i = 0; i < n; i++)
		PopInt();
}
void PopCharN(int n) {
	for (int i = 0; i < n; i++)
		PopChar();
}


bool LR1Analysis(WORDNODE* pHeader)
{
	char CurChar;
	int LR1Index;
	int nBnfIndex;
	int nLen;
	CHARNODE* Top;

	InitIntStack();
	PushInt(0);// 状态栈初始化

	InitCharStack();
	PushChar('#', NULL);// 文法符号栈初始化

	WORDNODE* pNode = pHeader->pNext;// 单词链表指向第一个单词

	PrintState(pNode, -1);// 打印堆栈

	while (pNode) {// 输入串
		CurChar = GetCurChar(pNode);
		LR1Index = GetLR1Index(TopInt(), CurChar);//根据栈顶状态和当前符号,查询分析表中的动作序号

		switch (m_Lr1[LR1Index].ActionType) {//m_Lr1[]该数组在读出LR1分析表时产生
		case Push:
			PushInt(m_Lr1[LR1Index].nNextStatus);
			PushChar(m_Lr1[LR1Index].CurChar, pNode);
			pNode = pNode->pNext;
			PrintState(pNode, -1);
			break;
		case Sumup://规约过程
			nBnfIndex = m_Lr1[LR1Index].nNextStatus;//取出下一个状态的产生式序号
			nLen = m_Bnf[nBnfIndex].RLen;//取出产生式右部的长度

			PopIntN(nLen);
			PopCharN(nLen);//将状态栈和符号栈的N个元素出栈

			PushChar(m_Bnf[nBnfIndex].Left, NULL);//将产生式的左部压入符号栈

			Top = TopChar();//得到符号栈的栈顶
			LR1Index = GetLR1Index(TopInt(), Top->cCur);//根据状态栈栈顶状态和符号栈栈顶符号查询分析表，确定转移状态；

			if (LR1Index == -1)
				return false;
			else
				PushInt(m_Lr1[LR1Index].nNextStatus);//压入状态栈
			PrintState(pNode, nBnfIndex);//打印
			break;
		case Accept:
			return true;
		default: return false;
		}
	}
}


int main(int argc, char* argv[])
{
	// 输入文件名
	char FileName[MAX_DATA_LEN];
	printf("请输入词法分析的文件名：\n");
	scanf("%s", FileName);

	// 读取文件
	printf("单词序列：\n");
	WORDNODE* pHeader = (WORDNODE*)malloc(sizeof(WORDNODE));
	if (!ReadWords(FileName, pHeader))
	{
		printf("读取词法分析文件失败！\n");
		ClearWords(pHeader);
		return 0;
	}

	// 读取文法
	if (!ReadBnfs())
	{
		printf("读取产生式失败！\n");
		ClearWords(pHeader);
		return 0;
	}

	// 读取LR(1)分析表
	if (!ReadLR1())
	{
		printf("读取LR(1)分析表失败！\n");
		ClearWords(pHeader);
		return 0;
	}

	// 语法分析
	printf("语法分析过程：\n");
	printf("状态栈                                       符号栈         输入串      产生式\n");
	if (LR1Analysis(pHeader))
		printf("语法分析成功！\n");
	else
		printf("语法分析失败！\n");

	// 清空链表
	ClearWords(pHeader);

	getchar();
	getchar();

	return 0;
}