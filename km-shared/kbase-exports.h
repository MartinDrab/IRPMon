
#ifndef __KBASE_EXPORTS_H__
#define __KBASE_EXPORTS_H__


#ifdef KBASE_EXPORTS

#define KBASE_API			__declspec(dllexport)

#else

#define KBASE_API			__declspec(dllimport)

#endif










#endif
