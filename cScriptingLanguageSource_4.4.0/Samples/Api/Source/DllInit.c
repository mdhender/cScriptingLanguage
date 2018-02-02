/*  Copyright (c) 2000 IBK-Landquart-Switzerland. All rights reserved.
 *
 *  Module      :  DllInit.c
 *  Application :  DLL initialisation code for gcc/g++
 *  Author      :  Peter Koch, IBK
 *
 *  Date        Description                                 Who
 *  --------------------------------------------------------------------------
 *  Mar 2000    First release                               P.Koch, IBK
 */
#include <windows.h>

BOOL APIENTRY DllMain(HINSTANCE hInst, DWORD reason, LPVOID reserved)
{
  return TRUE;
}
