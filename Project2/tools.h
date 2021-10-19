#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include<Windows.h>
#include<stdio.h>
void __cdecl OutputDebugStringF(const char* format, ...);

#ifdef _DEBUG  
#define DbgPrintf   OutputDebugStringF  
#else  
#define DbgPrintf  
#endif 
