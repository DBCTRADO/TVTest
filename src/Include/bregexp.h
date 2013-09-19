/*     bregexp.h
	external use header file
						1999.11.22  T.Baba
*/
/*
 *	2002.08.24	modified by K2
 *	2011.06.17	modified by K.Takata
 */

#include <stddef.h>

#ifdef _BREGEXP_
/* for internal use */
#define BREGEXPAPI	__declspec(dllexport)
#define BREGCONST
#else
/* for external use */
#define BREGEXPAPI	__declspec(dllimport)
#define BREGCONST	const
#endif


#ifdef UNICODE
#define BMatch		BMatchW
#define BSubst		BSubstW
#define BMatchEx	BMatchExW
#define BSubstEx	BSubstExW
#define BTrans		BTransW
#define BSplit		BSplitW
#define BRegfree	BRegfreeW
#define BRegexpVersion	BRegexpVersionW

#define BoMatch		BoMatchW
#define BoSubst		BoSubstW
#endif /* UNICODE */

#define BREGEXP_MAX_ERROR_MESSAGE_LEN	80


typedef struct bregexp {
	BREGCONST TCHAR *outp;		/* result string start ptr  */
	BREGCONST TCHAR *outendp;	/* result string end ptr    */
	BREGCONST int   splitctr;	/* split result counter     */
	BREGCONST TCHAR **splitp;	/* split result pointer ptr     */
	INT_PTR rsv1;				/* reserved for external use    */
	TCHAR *parap;				/* parameter start ptr ie. "s/xxxxx/yy/gi"  */
	TCHAR *paraendp;			/* parameter end ptr     */
	TCHAR *transtblp;			/* translate table ptr   */
	TCHAR **startp;				/* match string start ptr   */
	TCHAR **endp;				/* match string end ptr     */
	int nparens;				/* number of parentheses */
} BREGEXP;

typedef BOOL (__stdcall *BCallBack)(int kind, int value, ptrdiff_t index);

#if defined(__cplusplus)
extern "C"
{
#endif

#ifdef _K2REGEXP_
/* K2Editor */
BREGEXPAPI
int BMatch(TCHAR *str, TCHAR *target, TCHAR *targetstartp, TCHAR *targetendp,
		int one_shot,
		BREGEXP **rxp, TCHAR *msg);
BREGEXPAPI
int BSubst(TCHAR *str, TCHAR *target, TCHAR *targetstartp, TCHAR *targetendp,
		BREGEXP **rxp, TCHAR *msg, BCallBack callback);
#else
/* Original */
BREGEXPAPI
int BMatch(TCHAR *str, TCHAR *target, TCHAR *targetendp,
		BREGEXP **rxp, TCHAR *msg);
BREGEXPAPI
int BSubst(TCHAR *str, TCHAR *target, TCHAR *targetendp,
		BREGEXP **rxp, TCHAR *msg);

/* Sakura Editor */
BREGEXPAPI
int BMatchEx(TCHAR *str, TCHAR *targetbegp, TCHAR *target, TCHAR *targetendp,
		BREGEXP **rxp, TCHAR *msg);
BREGEXPAPI
int BSubstEx(TCHAR *str, TCHAR *targetbegp, TCHAR *target, TCHAR *targetendp,
		BREGEXP **rxp, TCHAR *msg);
#endif


BREGEXPAPI
int BTrans(TCHAR *str, TCHAR *target, TCHAR *targetendp,
		BREGEXP **rxp, TCHAR *msg);
BREGEXPAPI
int BSplit(TCHAR *str, TCHAR *target, TCHAR *targetendp,
		int limit, BREGEXP **rxp, TCHAR *msg);
BREGEXPAPI
void BRegfree(BREGEXP *rx);

BREGEXPAPI
TCHAR *BRegexpVersion(void);


#ifndef _K2REGEXP_
/* bregonig.dll native APIs */

BREGEXPAPI
int BoMatch(const TCHAR *patternp, const TCHAR *optionp,
		const TCHAR *strstartp,
		const TCHAR *targetstartp, const TCHAR *targetendp,
		BOOL one_shot,
		BREGEXP **rxp, TCHAR *msg);

BREGEXPAPI
int BoSubst(const TCHAR *patternp, const TCHAR *substp, const TCHAR *optionp,
		const TCHAR *strstartp,
		const TCHAR *targetstartp, const TCHAR *targetendp,
		BCallBack callback,
		BREGEXP **rxp, TCHAR *msg);

#endif


#if defined(__cplusplus)
}
#endif

