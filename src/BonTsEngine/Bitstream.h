#ifndef BITSTREAM_H
#define BITSTREAM_H


class CBitstream
{
public:
	CBitstream(const BYTE *pBits,SIZE_T Size);
	~CBitstream();

	SIZE_T GetPos() const { return m_Pos; }
	DWORD GetBits(int Bits);
	bool GetFlag();
	int GetUE_V();
	int GetSE_V();
	bool Skip(int Bits);

protected:
	int GetVLCSymbol(int *Info);

	const BYTE *m_pBits;
	SIZE_T m_Size;
	SIZE_T m_Pos;
};


#endif
