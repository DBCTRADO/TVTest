#ifndef TVTEST_DEBUG_DEF_H
#define TVTEST_DEBUG_DEF_H


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#ifndef TVTEST_NO_DEFINE_NEW
#define new DEBUG_NEW
#endif
#endif


#endif
