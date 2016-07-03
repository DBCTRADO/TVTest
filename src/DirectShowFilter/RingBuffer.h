#pragma once

#include <algorithm>
#include <utility>
#include <deque>


template<typename T, size_t Unit = 1> class CRingBuffer
{
public:
	static const size_t UnitBytes = Unit * sizeof(T);

	CRingBuffer()
		: m_pBuffer(nullptr)
		, m_Capacity(0)
		, m_Used(0)
		, m_Pos(0)
	{
	}

	~CRingBuffer()
	{
		Free();
	}

	bool Allocate(size_t Size)
	{
		Free();

		try {
			m_pBuffer = new T[Size * Unit];
		} catch (...) {
			return false;
		}
		m_Capacity = Size;
		m_Used = 0;
		m_Pos = 0;

		return true;
	}

	bool Resize(size_t Size)
	{
		if (m_Capacity == Size)
			return true;

		if (Size == 0) {
			Free();
			return true;
		}

		if (m_pBuffer == nullptr)
			return Allocate(Size);

		T *pNewBuffer;
		try {
			pNewBuffer = new T[Size * Unit];
		} catch (...) {
			return false;
		}

		if (m_Used > 0) {
			if (m_Used > Size) {
				m_Pos = (m_Pos + (m_Used - Size)) % m_Capacity;
				m_Used = Size;
			}
			if (m_Pos + m_Used <= m_Capacity) {
				::CopyMemory(pNewBuffer, &m_pBuffer[m_Pos * Unit], m_Used * UnitBytes);
			} else {
				const size_t Avail = m_Capacity - m_Pos;
				::CopyMemory(pNewBuffer, &m_pBuffer[m_Pos * Unit], Avail * UnitBytes);
				::CopyMemory(&pNewBuffer[Avail * Unit], m_pBuffer, (m_Used - Avail) * UnitBytes);
			}
		}

		delete [] m_pBuffer;
		m_pBuffer = pNewBuffer;
		m_Capacity = Size;
		m_Pos = 0;

		return true;
	}

	void Free()
	{
		if (m_pBuffer) {
			delete [] m_pBuffer;
			m_pBuffer = nullptr;
			m_Capacity = 0;
			m_Used = 0;
			m_Pos = 0;
		}
	}

	T *Front() { return &m_pBuffer[m_Pos * Unit]; }
	const T *Front() const { return &m_pBuffer[m_Pos * Unit]; }

	T *Push()
	{
		size_t Pos = m_Pos + m_Used;
		if (Pos >= m_Capacity)
			Pos -= m_Capacity;
		if (m_Used < m_Capacity) {
			m_Used++;
		} else {
			m_Pos++;
			if (m_Pos == m_Capacity)
				m_Pos = 0;
		}

		return &m_pBuffer[Pos * Unit];
	}

	void Pop()
	{
		_ASSERT(m_Used != 0);
		m_Used--;
		m_Pos++;
		if (m_Pos == m_Capacity)
			m_Pos = 0;
	}

	void Write(const T *pData)
	{
		::CopyMemory(Push(), pData, UnitBytes);
	}

	void Read(T *pData)
	{
		::CopyMemory(pData, Front(), UnitBytes);
		Pop();
	}

	size_t Read(T *pData, size_t Size)
	{
		size_t Avail = (std::min)(m_Used, m_Capacity - m_Pos);
		if (Avail > Size)
			Avail = Size;
		::CopyMemory(pData, Front(), Avail * UnitBytes);
		m_Pos += Avail;
		if (m_Pos == m_Capacity)
			m_Pos = 0;
		m_Used -= Avail;

		if (m_Used > 0) {
			const size_t Remain = Size - Avail;
			if (Remain > 0)
				return Read(pData + Avail * Unit, Remain) + Avail;
		}

		return Avail;
	}

	void Clear()
	{
		m_Used = 0;
		m_Pos = 0;
	}

	bool IsAllocated() const { return m_pBuffer != nullptr; }
	size_t GetCapacity() const { return m_Capacity; }
	size_t GetUsed() const { return m_Used; }
	bool IsEmpty() const { return m_Used == 0; }
	bool IsFull() const { return m_Used == m_Capacity; }

protected:
	T *m_pBuffer;
	size_t m_Capacity;
	size_t m_Used;
	size_t m_Pos;
};


template<typename T, size_t Unit, size_t ChunkSize> class CChunkedRingBuffer
{
public:
	static const size_t UnitBytes = Unit * sizeof(T);

	CChunkedRingBuffer()
		: m_MaxChunks(1)
		, m_Capacity(ChunkSize)
		, m_Used(0)
		, m_Pos(0)
	{
	}

	~CChunkedRingBuffer()
	{
		Free();
	}

	bool Resize(size_t MaxChunks)
	{
		if (m_MaxChunks == MaxChunks)
			return true;

		if (MaxChunks == 0)
			return false;

		if (m_ChunkList.size() > MaxChunks) {
			delete [] m_ChunkList.front();
			m_ChunkList.pop_front();
			if (ChunkSize - m_Pos >= m_Used)
				m_Used = 0;
			else
				m_Used -= ChunkSize - m_Pos;
			if (m_ChunkList.size() > MaxChunks) {
				const size_t DeleteSize = (m_ChunkList.size() - MaxChunks) * ChunkSize;
				if (DeleteSize >= m_Used)
					m_Used = 0;
				else
					m_Used -= DeleteSize;
				do {
					delete [] m_ChunkList.front();
					m_ChunkList.pop_front();
				} while (m_ChunkList.size() > MaxChunks);
			}
			m_Pos = 0;
		}

		m_MaxChunks = MaxChunks;
		m_Capacity = ChunkSize * MaxChunks;

		return true;
	}

	void Free()
	{
		while (!m_ChunkList.empty()) {
			delete [] m_ChunkList.front();
			m_ChunkList.pop_front();
		}

		m_Used = 0;
		m_Pos = 0;
	}

	T *Front() { return m_ChunkList.front() + m_Pos * Unit; }
	const T *Front() const { return m_ChunkList.front() + m_Pos * Unit; }

	T *Push()
	{
		size_t Capacity = m_ChunkList.size() * ChunkSize;

		if (m_Used == Capacity
				&& m_ChunkList.size() < m_MaxChunks) {
			T *pBuffer = new T[ChunkSize * Unit];
			m_ChunkList.push_back(pBuffer);
			if (m_Pos != 0)
				::CopyMemory(pBuffer, m_ChunkList.front(), m_Pos * UnitBytes);
			Capacity += ChunkSize;
		}

		size_t Pos = m_Pos + m_Used;

		if (Pos >= Capacity)
			Pos -= Capacity;
		if (m_Used < Capacity) {
			m_Used++;
		} else {
			m_Pos++;
			if (m_Pos == ChunkSize) {
				m_Pos = 0;
				RotateBuffer();
			}
		}

		return m_ChunkList[Pos / ChunkSize] + (Pos % ChunkSize) * Unit;
	}

	void Pop()
	{
		_ASSERT(m_Used != 0);
		m_Used--;
		m_Pos++;
		if (m_Pos == ChunkSize) {
			m_Pos = 0;
			if (m_Used > 0)
				RotateBuffer();
		}
	}

	void Pop(size_t Size)
	{
		if (m_Used <= Size) {
			Clear();
		} else {
			const size_t Pos = m_Pos + Size;
			for (size_t i = 0; i < Pos / ChunkSize; i++)
				RotateBuffer();
			m_Pos = Pos % ChunkSize;
			m_Used -= Size;
		}
	}

	void Write(const T *pData)
	{
		::CopyMemory(Push(), pData, UnitBytes);
	}

	void Read(T *pData)
	{
		::CopyMemory(pData, Front(), UnitBytes);
		Pop();
	}

	size_t Read(T *pData, size_t Size)
	{
		size_t Avail = (std::min)(m_Used, ChunkSize - m_Pos);
		if (Avail > Size)
			Avail = Size;
		::CopyMemory(pData, Front(), Avail * UnitBytes);
		m_Pos += Avail;
		m_Used -= Avail;
		if (m_Pos == ChunkSize) {
			m_Pos = 0;
			if (m_Used > 0)
				RotateBuffer();
		}

		if (m_Used > 0) {
			const size_t Remain = Size - Avail;
			if (Remain > 0)
				return Read(pData + Avail * Unit, Remain) + Avail;
		}

		return Avail;
	}

	void Clear()
	{
		m_Used = 0;
		m_Pos = 0;
	}

	void ShrinkToFit()
	{
		const size_t Used = m_Pos + m_Used;
		while (!m_ChunkList.empty()
			   && (m_ChunkList.size() - 1) * ChunkSize >= Used) {
			delete [] m_ChunkList.back();
			m_ChunkList.pop_back();
		}
	}

	size_t GetChunkSize() const { return ChunkSize; }
	size_t GetMaxChunkCount() const { return m_MaxChunks; }
	size_t GetAllocatedChunkCount() const { return m_ChunkList.size(); }
	size_t GetCapacity() const { return m_Capacity; }
	size_t GetUsed() const { return m_Used; }
	size_t GetAllocatedSize() const { return m_ChunkList.size() * ChunkSize; }
	bool IsEmpty() const { return m_Used == 0; }
	bool IsFull() const { return m_Used == m_Capacity; }

protected:
	void RotateBuffer()
	{
		T *pBuffer = m_ChunkList.front();
		m_ChunkList.pop_front();
		m_ChunkList.push_back(pBuffer);
	}

	std::deque<T*> m_ChunkList;
	size_t m_MaxChunks;
	size_t m_Capacity;
	size_t m_Used;
	size_t m_Pos;
};
