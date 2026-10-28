// Copyright vedri studio Ltd. All Rights Reserved.

// Force-included (-include) before every standalone test translation unit.
//
// The Core headers are compiled inside Unreal on Win64, where engine and Windows SDK headers leave
// macros defined that silently break ordinary identifiers (a function named check(), a variable named
// near, an enum value DELETE, a member called GetObject...). This file reproduces those macros so such
// a name fails to compile here, on the Mac, instead of on the PC.
//
// Standard headers are included first, as they are in a real Unreal translation unit, so the macros
// only hit our code. If a test needs a standard header not listed here, add it to this list.

#pragma once

#include <algorithm>
#include <array>
#include <clocale>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <iterator>
#include <limits>
#include <locale>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

// Unreal Engine (CoreMinimal / UnrealMathUtility / AssertionMacros / Platform.h)
#define PI )
#define DELTA )
#define SMALL_NUMBER )
#define KINDA_SMALL_NUMBER )
#define BIG_NUMBER )
#define TEXT(...) )
#define check(...) )
#define checkf(...) )
#define verify(...) )
#define ensure(...) )
#define CONSTEXPR )
#define FORCEINLINE )

// Windows SDK (minwindef.h, winnt.h, wingdi.h, winbase.h, rpcndr.h, combaseapi.h, winuser.h)
#define near )
#define far )
#define small )
#define hyper )
#define interface )
#define IN )
#define OUT )
#define OPTIONAL )
#define CONST )
#define TRUE )
#define FALSE )
#define ERROR )
#define DELETE )
#define IGNORE )
#define ABSOLUTE )
#define RELATIVE )
#define TRANSPARENT )
#define OPAQUE )
#define CALLBACK )
#define GetObject )
#define GetMessage )
#define SendMessage )
#define PostMessage )
#define LoadImage )
#define CreateFile )
#define DeleteFile )
#define CopyFile )
#define MoveFile )
#define CreateDirectory )
#define GetCurrentTime )
#define Yield )
#define RGB(...) )
#define min(...) )
#define max(...) )
