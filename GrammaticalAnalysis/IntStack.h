// File Name: IntStack.h
#pragma once
int m_IntStack[1024];
int	m_nIntTop = -1;

void InitIntStack()
{
	m_nIntTop = -1;
}

void PushInt(int c)
{
	m_IntStack[++m_nIntTop] = c;
}

int PopInt()
{
	return m_IntStack[m_nIntTop--];
}

int TopInt()
{
	if (m_nIntTop < 0)
		return '\0';
	return m_IntStack[m_nIntTop];
}

void PrintIntStack()
{
	int i;
	for (i = 0; i <= m_nIntTop; i++)
		printf("%3d", m_IntStack[i]);
	for (; i < 15; i++)
		printf("   ");
}