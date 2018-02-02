/* Copyright (c) 2000 IBK-Landquart-Switzerland. All rights reserved.
 *
 * Module      :  DllInit.c
 * Application :  DLL initialisation code for gcc/g++
 * Author      :  Peter Koch, IBK
 *
 * Date        Description                                 Who
 * --------------------------------------------------------------------------
 * Mar 2000    First release                               P.Koch, IBK
 *
 * OPEN SOURCE LICENSE
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to IBK at info@ibk-software.ch.
 */
#include <windows.h>

BOOL APIENTRY DllMain(HINSTANCE hInst, DWORD reason, LPVOID reserved)
{
  return TRUE;
}

