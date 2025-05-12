#ifndef BASE_MACROS_H
#define BASE_MACROS_H

#if 1
    #define Assert(e) if(!(e)) {*(int*)0 = 0;}
#else
    #define Assert(e)
#endif

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

#define Min(a,b) (((a)<(b))?(a):(b))
#define Max(a,b) (((a)>(b))?(a):(b))
#define ClampTop(a,b) Min(a,b)
#define ClampBot(a,b) Max(a,b)

#endif
