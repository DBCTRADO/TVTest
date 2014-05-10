#ifndef HDUS_REMOCON_KEYHOOK_H
#define HDUS_REMOCON_KEYHOOK_H


#ifdef __cplusplus
extern "C" {
#endif


#define KEYHOOK_MESSAGE TEXT("HDUS Remocon KeyHook")

#define KEYHOOK_LPARAM_REPEATCOUNT	0x0000FFFF
#define KEYHOOK_LPARAM_SHIFT		0x00010000
#define KEYHOOK_LPARAM_CONTROL		0x00020000
#define KEYHOOK_GET_REPEATCOUNT(lParam)	((lParam)&KEYHOOK_LPARAM_REPEATCOUNT)
#define KEYHOOK_GET_SHIFT(lParam)		(((lParam)&KEYHOOK_LPARAM_SHIFT)!=0)
#define KEYHOOK_GET_CONTROL(lParam)		(((lParam)&KEYHOOK_LPARAM_CONTROL)!=0)

typedef BOOL (WINAPI *BeginHookFunc)(HWND hwnd,BOOL fLocal);
typedef BOOL (WINAPI *EndHookFunc)(void);
typedef BOOL (WINAPI *SetWindowFunc)(HWND hwnd);


#ifdef __cplusplus
}
#endif


#endif
