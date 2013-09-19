#include "stdafx.h"
#include "Bitstream.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif




CBitstream::CBitstream(const BYTE *pBits,SIZE_T Size)
	: m_pBits(pBits)
	, m_Size(Size)
	, m_Pos(0)
{
}


CBitstream::~CBitstream()
{
}


DWORD CBitstream::GetBits(int Bits)
{
	if (m_Size<<3<m_Pos+Bits)
		return 0;

	const BYTE *p=&m_pBits[m_Pos>>3];
	int Shift=(int)(7-(m_Pos&7));
	DWORD Value=0;
	for (int i=0;i<Bits;i++) {
		Value<<=1;
		Value|=(*p>>Shift)&0x01;
		Shift--;
		if (Shift<0) {
			Shift=7;
			p++;
		}
	}
	m_Pos+=Bits;
	return Value;
}


bool CBitstream::GetFlag()
{
	return GetBits(1)!=0;
}


int CBitstream::GetUE_V()
{
	int Info;
	int Length=GetVLCSymbol(&Info);

	if (Length<0)
		return -1;
	return (1<<(Length>>1))+Info-1;
}


int CBitstream::GetSE_V()
{
	int Info;
	int Length=GetVLCSymbol(&Info);

	if (Length<0)
		return -1;
	unsigned int n=(1U<<(Length>>1))+Info-1;
	int Value=(n+1)>>1;
	return (n&0x01)!=0?Value:-Value;
}


bool CBitstream::Skip(int Bits)
{
	if (m_Pos+Bits>m_Size<<3)
		return false;
	m_Pos+=Bits;
	return true;
}


int CBitstream::GetVLCSymbol(int *pInfo)
{
	const BYTE *p=&m_pBits[m_Pos>>3];
	const BYTE *pEnd=m_pBits+m_Size;
	int Shift=(int)(7-(m_Pos&7));
	int BitCount=1;
	int Length=0;

	while (((*p>>Shift)&0x01)==0) {
		Length++;
		BitCount++;
		Shift--;
		if (Shift<0) {
			Shift=7;
			p++;
			if (p==pEnd) {
				m_Pos=m_Size<<3;
				return -1;
			}
		}
	}
	BitCount+=Length;
	if (m_Pos+BitCount>m_Size<<3) {
		m_Pos=m_Size<<3;
		return -1;
	}

	int Info=0;
	while (Length-->0) {
		Shift--;
		if (Shift<0) {
			Shift=7;
			p++;
		}
		Info<<=1;
		Info|=(*p>>Shift)&0x01;
	}
	*pInfo=Info;
	m_Pos+=BitCount;
	return BitCount;
}
