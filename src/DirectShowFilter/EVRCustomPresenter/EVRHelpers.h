#pragma once


template<class T> void SafeRelease(T *&p)
{
	if (p) {
		p->Release();
		p = nullptr;
	}
}

template<class T> void SafeDelete(T *&p)
{
	if (p) {
		delete p;
		p = nullptr;
	}
}

template <class T> void CopyComPointer(T *&dest, T *src)
{
	if (dest) {
		dest->Release();
	}
	dest = src;
	if (dest) {
		dest->AddRef();
	}
}

template <class T1, class T2> bool AreComObjectsEqual(T1 *p1, T2 *p2)
{
	bool bResult = false;

	if (p1 == p2) {
		bResult = true;
	} else if (p1 == nullptr || p2 == nullptr) {
		bResult = false;
	} else {
		IUnknown *pUnk1 = nullptr;
		IUnknown *pUnk2 = nullptr;

		if (SUCCEEDED(p1->QueryInterface(IID_PPV_ARGS(&pUnk1)))) {
			if (SUCCEEDED(p2->QueryInterface(IID_PPV_ARGS(&pUnk2)))) {
				bResult = (pUnk1 == pUnk2);
				pUnk2->Release();
			}
			pUnk1->Release();
		}
	}

	return bResult;
}

inline LONG MFTimeToMsec(LONGLONG time)
{
	return (LONG)(time / (10000000 / 1000));
}


class RefCountedObject
{
public:
	RefCountedObject() : m_RefCount(1)
	{
	}

	ULONG AddRef()
	{
		return ::InterlockedIncrement(&m_RefCount);
	}

	ULONG Release()
	{
		ULONG Count = ::InterlockedDecrement(&m_RefCount);
		if (Count == 0) {
			delete this;
		}
		return Count;
	}

protected:
	virtual ~RefCountedObject()
	{
	}

private:
	LONG m_RefCount;
};


template <class T> struct NoOp
{
	void operator()(T &t)
	{
	}
};

template <class T> class LinkedList
{
protected:
	struct Node
	{
		Node *pPrev;
		Node *pNext;
		T Item;

		Node() : pPrev(nullptr), pNext(nullptr)
		{
		}

		Node(T item) : pPrev(nullptr), pNext(nullptr), Item(item)
		{
		}
	};

public:
	class Position
	{
	public:
		friend class LinkedList<T>;

		Position() : pNode(nullptr)
		{
		}

		bool operator==(const Position &rhs) const
		{
			return pNode == rhs.pNode;
		}

		bool operator!=(const Position &rhs) const
		{
			return pNode != rhs.pNode;
		}

	private:
		const Node *pNode;

		Position(Node *p) : pNode(p)
		{
		}
	};

	LinkedList() : m_Count(0)
	{
		m_Anchor.pNext = &m_Anchor;
		m_Anchor.pPrev = &m_Anchor;
	}

	virtual ~LinkedList()
	{
		Clear();
	}

	HRESULT InsertBack(T Item)
	{
		return InsertAfter(Item, m_Anchor.pPrev);
	}

	HRESULT InsertFront(T Item)
	{
		return InsertAfter(Item, &m_Anchor);
	}

	HRESULT RemoveBack(T *pItem = nullptr)
	{
		if (IsEmpty()) {
			return E_UNEXPECTED;
		}
		return RemoveItem(Back(), pItem);
	}

	HRESULT RemoveFront(T *pItem = nullptr)
	{
		if (IsEmpty()) {
			return E_UNEXPECTED;
		}
		return RemoveItem(Front(), pItem);
	}

	HRESULT GetBack(T *pItem)
	{
		if (IsEmpty()) {
			return E_UNEXPECTED;
		}
		return GetItem(Back(), pItem);
	}

	HRESULT GetFront(T *pItem)
	{
		if (IsEmpty()) {
			return E_UNEXPECTED;
		}
		return GetItem(Front(), pItem);
	}

	size_t GetCount() const { return m_Count; }

	bool IsEmpty() const
	{
		return m_Count == 0;
	}

	template <class TFree> void Clear(TFree &Free)
	{
		Node *pNext = m_Anchor.pNext;

		while (pNext != &m_Anchor) {
			Free(pNext->Item);

			Node *pTmp = pNext->pNext;
			delete pNext;
			pNext = pTmp;
		}

		m_Anchor.pNext = &m_Anchor;
		m_Anchor.pPrev = &m_Anchor;

		m_Count = 0;
	}

	virtual void Clear()
	{
		Clear< NoOp<T> >(NoOp<T>());
	}

	Position FrontPosition()
	{
		if (IsEmpty()) {
			return Position(nullptr);
		}
		return Position(Front());
	}

	Position EndPosition() const
	{
		return Position();
	}

	HRESULT GetItemPos(Position Pos, T *pItem)
	{
		if (!Pos.pNode) {
			return E_INVALIDARG;
		}
		return GetItem(Pos.pNode, pItem);
	}

	Position Next(Position Pos)
	{
		if (Pos.pNode && (Pos.pNode->pNext != &m_Anchor)) {
			return Position(Pos.pNode->pNext);
		} else {
			return Position(nullptr);
		}
	}

	HRESULT Remove(Position &Pos, T *pItem)
	{
		if (!Pos.pNode) {
			return E_INVALIDARG;
		}

		Node *pNode = const_cast<Node*>(Pos.pNode);

		Pos = Position();

		return RemoveItem(pNode, pItem);
	}

protected:
	Node m_Anchor;
	size_t m_Count;

	Node *Front() const
	{
		return m_Anchor.pNext;
	}

	Node *Back() const
	{
		return m_Anchor.pPrev;
	}

	virtual HRESULT InsertAfter(T item, Node *pBefore)
	{
		if (pBefore == nullptr) {
			return E_POINTER;
		}

		Node *pNode = new(std::nothrow) Node(item);
		if (pNode == nullptr) {
			return E_OUTOFMEMORY;
		}

		Node *pAfter = pBefore->pNext;

		pBefore->pNext = pNode;
		pAfter->pPrev = pNode;

		pNode->pPrev = pBefore;
		pNode->pNext = pAfter;

		m_Count++;

		return S_OK;
	}

	virtual HRESULT GetItem(const Node *pNode, T *pItem)
	{
		if ((pNode == nullptr) || (pItem == nullptr)) {
			return E_POINTER;
		}

		*pItem = pNode->Item;

		return S_OK;
	}

	virtual HRESULT RemoveItem(Node *pNode, T *pItem)
	{
		if (pNode == nullptr) {
			return E_POINTER;
		}
		if (pNode == &m_Anchor) {
			return E_INVALIDARG;
		}

		pNode->pNext->pPrev = pNode->pPrev;
		pNode->pPrev->pNext = pNode->pNext;

		if (pItem) {
			*pItem = pNode->Item;
		}

		delete pNode;
		m_Count--;

		return S_OK;
	}
};


struct ComAutoRelease
{
	void operator()(IUnknown *p)
	{
		if (p) {
			p->Release();
		}
	}
};

template<class T> struct AutoDelete
{
	void operator()(T *p)
	{
		if (p) {
			delete p;
		}
	}
};


template <class T, bool NULLABLE = false> class ComPtrList : public LinkedList<T*>
{
public:
	void Clear()
	{
		LinkedList<T*>::Clear(ComAutoRelease());
	}

	~ComPtrList()
	{
		Clear();
	}

protected:
	HRESULT InsertAfter(T *pItem, Node *pBefore) override
	{
		if (!pItem && !NULLABLE) {
			return E_POINTER;
		}

		HRESULT hr = LinkedList<T*>::InsertAfter(pItem, pBefore);

		if (SUCCEEDED(hr)) {
			if (pItem) {
				pItem->AddRef();
			}
		}

		return hr;
	}

	HRESULT GetItem(const Node *pNode, T **ppItem) override
	{
		T *pItem = nullptr;
		HRESULT hr = LinkedList<T*>::GetItem(pNode, &pItem);

		if (SUCCEEDED(hr)) {
			*ppItem = pItem;
			if (pItem) {
				pItem->AddRef();
			}
		}

		return hr;
	}

	HRESULT RemoveItem(Node *pNode, T **ppItem) override
	{
		T *pItem = nullptr;
		HRESULT hr = LinkedList<T*>::RemoveItem(pNode, &pItem);

		if (SUCCEEDED(hr)) {
			if (ppItem) {
				*ppItem = pItem;
			} else {
				if (pItem) {
					pItem->Release();
				}
			}
		}

		return hr;
	}
};

typedef ComPtrList<IMFSample> VideoSampleList;


template <class T> class ThreadSafeQueue
{
public:
	HRESULT Queue(T *p)
	{
		CAutoLock Lock(&m_Lock);
		return m_List.InsertBack(p);
	}

	HRESULT Dequeue(T **pp)
	{
		CAutoLock Lock(&m_Lock);

		if (m_List.IsEmpty()) {
			*pp = nullptr;
			return S_FALSE;
		}

		return m_List.RemoveFront(pp);
	}

	HRESULT PutBack(T *p)
	{
		CAutoLock Lock(&m_Lock);
		return m_List.InsertFront(p);
	}

	void Clear() 
	{
		CAutoLock Lock(&m_Lock);
		m_List.Clear();
	}

private:
	CCritSec m_Lock; 
	ComPtrList<T> m_List;
};


template<class T> class AsyncCallback : public IMFAsyncCallback
{
public: 
	typedef HRESULT (T::*InvokeFn)(IMFAsyncResult *pAsyncResult);

	AsyncCallback(T *pParent, InvokeFn fn)
		: m_pParent(pParent)
		, m_pInvokeFn(fn)
	{
	}

	STDMETHODIMP QueryInterface(REFIID riid, void **ppv) override
	{
		if (ppv == nullptr) {
			return E_POINTER;
		}

		if (riid == __uuidof(IUnknown)) {
			*ppv = static_cast<IUnknown*>(static_cast<IMFAsyncCallback*>(this));
		} else if (riid == __uuidof(IMFAsyncCallback)) {
			*ppv = static_cast<IMFAsyncCallback*>(this);
		} else {
			*ppv = nullptr;
			return E_NOINTERFACE;
		}

		AddRef();

		return S_OK;
	}

	STDMETHODIMP_(ULONG) AddRef() override
	{
		//return m_pParent->AddRef();
		return 1;
	}

	STDMETHODIMP_(ULONG) Release() override
	{
		//return m_pParent->Release();
		return 1;
	}

	STDMETHODIMP GetParameters(DWORD *pdwFlags, DWORD *pdwQueue) override
	{
		return E_NOTIMPL;
	}

	STDMETHODIMP Invoke(IMFAsyncResult *pAsyncResult) override
	{
		return (m_pParent->*m_pInvokeFn)(pAsyncResult);
	}

	T *m_pParent;
	InvokeFn m_pInvokeFn;
};


class CSamplePool
{
public:
	CSamplePool();
	virtual ~CSamplePool();

	HRESULT Initialize(VideoSampleList &Samples);
	HRESULT Clear();

	HRESULT GetSample(IMFSample **ppSample);
	HRESULT ReturnSample(IMFSample *pSample);
	bool AreSamplesPending();

private:
	CCritSec m_Lock;

	VideoSampleList m_VideoSampleQueue;
	bool m_bInitialized;
	size_t m_PendingCount;
};
