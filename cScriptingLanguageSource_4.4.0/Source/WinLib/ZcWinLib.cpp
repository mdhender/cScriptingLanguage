/* Copyright (c) 2001-2002 IBK-Landquart-Switzerland. All rights reserved.
 *
 * Module      :  ZcWinLib.cpp
 * Application :  CSL windows library
 * Purpose     :  Implementation
 *
 * Date        Description                                 Who
 * --------------------------------------------------------------------------
 * 2001.05.30  First implementation                        P.Koch, IBK
 * 2001.07.07  Renaming from css to csl                    P.Koch, IBK
 * 2001.10.17  Extended winPostVKey to enable simultaneous P.Koch, IBK
 *             posting of up to 3 VKeys
 * 2002.05.26  Release 4.04                                P.Koch, IBK
 *
 * OPEN SOURCE LICENSE
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to IBK at info@ibk-software.ch.
 */

#include <stdlib.h>
#include <ZRegular.hpp>
#include <ZExcept.hpp>
#include <windows.h>

#if ZC_GNU
  #include <strstream.h>
#else
  #include <strstrea.h>
#endif

#ifdef ZC_NATIVECSLLIB
  #include <ZCsl.hpp>
#else
  #include <ZCslWrap.hpp>
#endif

static void hitVKey(USHORT vk)
{
   UINT sc(MapVirtualKey(vk,0));
   keybd_event(vk, sc, KEYEVENTF_EXTENDEDKEY | 0, 0);
   keybd_event(vk, sc, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
} // hitVKey

static ZBoolean isCapsLock()
{
   BYTE keyState[256];
   GetKeyboardState((LPBYTE)&keyState);
   return keyState[VK_CAPITAL] & 1;
} // isCapsLock

static void toggleCapsLock()
{
   hitVKey(VK_CAPITAL);
} // toggleCapsLock

static ZBoolean isNumLock()
{
   BYTE keyState[256];
   GetKeyboardState((LPBYTE)&keyState);
   return keyState[VK_NUMLOCK] & 1;
} // isNumLock

static void toggleNumLock()
{
   hitVKey(VK_NUMLOCK);
} // toggleNumLock

static ZBoolean isScrollLock()
{
   BYTE keyState[256];
   GetKeyboardState((LPBYTE)&keyState);
   return keyState[VK_SCROLL] & 1;
} // isScrollLock

static void toggleScrollLock()
{
   hitVKey(VK_SCROLL);
} // toggleScrollLock

static void throwWinError(const ZString& func)
{
   DWORD errcode = GetLastError();
   LPTSTR lpMsgBuf;
   FormatMessage(
      FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
      NULL,
      errcode,
      MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT),
      (LPTSTR)&lpMsgBuf,
      0, NULL
   );
   ZException exc(func+" error "+ZString(errcode)+": "+lpMsgBuf);
   LocalFree(lpMsgBuf);
   ZTHROW(exc);
} // throwWinError

static ZString winActivate(ZCsl* csl)
{
   HWND handle((HWND)csl->get("handle").asUnsigned());
   if (!SetForegroundWindow(handle))
      throwWinError("winActivate");
   return ZString();
} // winActivate

static ZString winCapsLock(ZCsl* csl)
{
   ZBoolean caps(isCapsLock());
   int argCount = csl->get("argCount").asInt();
   if (argCount >= 1) {
      switch (csl->get("mode").asInt()) {
         case 0: // off
            if (caps) {
               toggleCapsLock();
               caps = zFalse;
            } // if
            break;
         case 1: // on
            if (!caps) {
               toggleCapsLock();
               caps = zTrue;
            } // if
            break;
         default: // 2 = toggle
            toggleCapsLock();
            caps = !caps;
      } // switch
   } // if
   return ZString(caps);
} // winCapsLock

static ZString winClose(ZCsl* csl)
{
   HWND handle((HWND)csl->get("handle").asUnsigned());
   if (!PostMessage(handle, WM_CLOSE, 0, 0))
      throwWinError("winClose");
   return ZString();
} // winClose

static void findChildsOf(
   ZCsl *csl,
   HWND handle,
   long &cnt,
   long yMax,
   const ZString& name,
   ZRegularExpression* iregex,
   long level)
{
   char title[255];
   handle = GetTopWindow(handle);
   while (handle && cnt < yMax) {
      GetWindowText(handle, title, sizeof(title));
      ZBoolean take;
      if (iregex) {
         long startPos, length;
         take = iregex->match(title, 1, &startPos, &length);
      } else
         take = name == title;
      if (take) {
         csl->set("win["+ZString(cnt)+"][0]", ZString((unsigned long)handle));
         csl->set("win["+ZString(cnt)+"][1]", title);
         cnt++;
      } // if
      if (level > 0 && cnt < yMax)
         findChildsOf(csl, handle, cnt, yMax, name, iregex, level-1);
      handle = GetNextWindow(handle, GW_HWNDNEXT);
   } // for
} // findChildsOf

static ZString winFind(ZCsl* csl)
{
   ZString name(csl->get("name"));

   long xMax(csl->varSizeof("win[0]"));
   long yMax(csl->varSizeof("win") / xMax);

   ZBoolean regular(zFalse);
   int argCount = csl->get("argCount").asInt();
   if (argCount >= 3) regular = csl->get("regular").asInt() ? 1 : 0;

   long level(0);
   if (argCount >= 4) level = csl->get("level").asInt();

   ZRegularExpression* iRegEx = 0;
   if (regular)
      iRegEx = new ZRegularExpression(name);
   long cnt(0);
   try {
      findChildsOf(csl, GetDesktopWindow(), cnt, yMax, name, iRegEx, level);
   } // try
   catch (const ZException &exc) {
      if (iRegEx) delete iRegEx;
      throw;
   } // catch
   return ZString(cnt);
} // winFind

static ZString winHide(ZCsl* csl)
{
   HWND handle((HWND)csl->get("handle").asUnsigned());
   ShowWindow(handle, SW_HIDE);
   return ZString();
} // winHide

static ZString winIsMaximized(ZCsl* csl)
{
   HWND handle((HWND)csl->get("handle").asUnsigned());
   return IsZoomed(handle) ? "1" : "0";
} // winIsMaximized

static ZString winIsMinimized(ZCsl* csl)
{
   HWND handle((HWND)csl->get("handle").asUnsigned());
   return IsIconic(handle) ? "1" : "0";
} // winIsMinimized

static ZString winIsVisible(ZCsl* csl)
{
   HWND handle((HWND)csl->get("handle").asUnsigned());
   return IsWindowVisible(handle) ? "1" : "0";
} // winIsVisible

static ZString winNumLock(ZCsl* csl)
{
   ZBoolean num(isNumLock());
   int argCount = csl->get("argCount").asInt();
   if (argCount >= 1) {
      switch (csl->get("mode").asInt()) {
         case 0: // off
            if (num) {
               toggleNumLock();
               num = zFalse;
            } // if
            break;
         case 1: // on
            if (!num) {
               toggleNumLock();
               num = zTrue;
            } // if
            break;
         default: // 2 = toggle
            toggleNumLock();
            num = !num;
      } // switch
   } // if
   return ZString(num);
} // winNumLock

static ZString winMaximize(ZCsl* csl)
{
   HWND handle((HWND)csl->get("handle").asUnsigned());
   if (!IsZoomed(handle)) ShowWindow(handle, SW_SHOWMAXIMIZED);
   return ZString();
} // winMaximize

static ZString winMinimize(ZCsl* csl)
{
   HWND handle((HWND)csl->get("handle").asUnsigned());
   if (!IsIconic(handle)) ShowWindow(handle, SW_MINIMIZE);
   return ZString();
} // winMinimize

static ZString winPostText(ZCsl* csl)
{
   ZBoolean caps(isCapsLock());
   ZBoolean cp(caps);
   ZString strText(csl->get("text"));
   char* text = strText;
   while (*text) {
      switch (*text) {
         case 'Ä':
         case 'Ö':
         case 'Ü':
         case 'É':
         case 'À':
         case 'È':
            if (!cp) { toggleCapsLock(); cp = zTrue; }
            break;
         default:
            if (cp) { toggleCapsLock(); cp = zFalse; }
      } // switch
      USHORT vk(VkKeyScan(*text++));
      UINT sc(MapVirtualKey(vk,0));
      if (sc == 0xFFFF) continue;
      UINT sc1(0), sc2(0), sc3(0);
      if (vk & 0x0100) {
         sc1 = MapVirtualKey(VK_LSHIFT,0);
         keybd_event(VK_SHIFT, sc1, KEYEVENTF_EXTENDEDKEY | 0, 0);
      } // if
      if (vk & 0x0200) {
         sc2 = MapVirtualKey(VK_LCONTROL,0);
         keybd_event(VK_CONTROL, sc2, KEYEVENTF_EXTENDEDKEY | 0, 0);
      } // if
      if (vk & 0x0400) {
         sc3 = MapVirtualKey(VK_MENU,0);
         keybd_event(VK_MENU, sc3, KEYEVENTF_EXTENDEDKEY | 0, 0);
      } // if
      keybd_event(vk, sc, 0, 0);
      keybd_event(vk, sc, KEYEVENTF_KEYUP, 0);
      if (vk & 0x0400)
         keybd_event(VK_MENU, sc3, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
      if (vk & 0x0200)
         keybd_event(VK_CONTROL, sc2, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
      if (vk & 0x0100)
         keybd_event(VK_SHIFT, sc1, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
   } // while
   if (caps != cp) toggleCapsLock();
   return ZString();
} // winPostText

static ZString winPostVKey(ZCsl* csl)
{
   int argc(csl->get("argCount").asInt());
   UINT sc[3];
   USHORT vk[3];
   int a;
   // get vkeys and press down in order
   for (a = 0; a < argc; a++) {
      vk[a] = csl->get("vkey"+ZString(a+1)).asUnsigned();
      sc[a] = MapVirtualKey(vk[a],0);
      keybd_event(vk[a], sc[a], KEYEVENTF_EXTENDEDKEY | 0, 0);
   } // for
   // release vkeys in reverse order
   for (a = argc-1; a >= 0; a--)
      keybd_event(vk[a], sc[a], KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
   return ZString();
} // winPostVKey

static ZString winPrintScreen(ZCsl* csl)
{
   BYTE sc(0);
   int argCount = csl->get("argCount").asInt();
   if (argCount >= 1)
      if (csl->get("mode").asInt()) sc = 1;
   keybd_event(VK_SNAPSHOT, sc, KEYEVENTF_EXTENDEDKEY | 0, 0);
   keybd_event(VK_SNAPSHOT, sc, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
   return ZString();
} // winPrintScreen

static ZString winRestore(ZCsl* csl)
{
   HWND handle((HWND)csl->get("handle").asUnsigned());
   if (IsZoomed(handle) || IsIconic(handle))
      ShowWindow(handle, SW_RESTORE);
   return ZString();
} // winRestore

static ZString winScrollLock(ZCsl* csl)
{
   ZBoolean scroll(isScrollLock());
   int argCount = csl->get("argCount").asInt();
   if (argCount >= 1) {
      switch (csl->get("mode").asInt()) {
         case 0: // off
            if (scroll) {
               toggleScrollLock();
               scroll = zFalse;
            } // if
            break;
         case 1: // on
            if (!scroll) {
               toggleScrollLock();
               scroll = zTrue;
            } // if
            break;
         default: // 2 = toggle
            toggleScrollLock();
            scroll = !scroll;
      } // switch
   } // if
   return ZString(scroll);
} // winScrollLock

static ZString winShow(ZCsl* csl)
{
   HWND handle((HWND)csl->get("handle").asUnsigned());
   ShowWindow(handle, SW_SHOW);
   return ZString();
} // winShow

ZCslInitLib(aCsl)
{
   ZString iFile("ZcWinLib");
   ZString init(ZString(
      "const winVersion = '4.04';\n"
      "const winCompiler = '")+ZC_COMPILER+"';\n"
      "const winLibtype = '"+ZC_CSLLIBTYPE+"';\n"
      "const winBuilt = '"+ZString(__DATE__)+" "+__TIME__+"';\n"

      "const VK_LBUTTON = "+ZString(VK_LBUTTON)+";\n"
      "const VK_RBUTTON  = "+ZString(VK_RBUTTON)+";\n"
      "const VK_CANCEL = "+ZString(VK_CANCEL)+";\n"
      "const VK_MBUTTON = "+ZString(VK_MBUTTON)+";\n"
      "const VK_BACK = "+ZString(VK_BACK)+";\n"
      "const VK_TAB = "+ZString(VK_TAB)+";\n"
      "const VK_CLEAR = "+ZString(VK_CLEAR)+";\n"
      "const VK_RETURN = "+ZString(VK_RETURN)+";\n"
      "const VK_SHIFT = "+ZString(VK_SHIFT)+";\n"
      "const VK_CONTROL = "+ZString(VK_CONTROL)+";\n"
      "const VK_MENU = "+ZString(VK_MENU)+";\n"
      "const VK_PAUSE = "+ZString(VK_PAUSE)+";\n"
      "const VK_CAPITAL = "+ZString(VK_CAPITAL)+";\n"
      "const VK_ESCAPE = "+ZString(VK_ESCAPE)+";\n"
      "const VK_SPACE = "+ZString(VK_SPACE)+";\n"
      "const VK_PRIOR = "+ZString(VK_PRIOR)+";\n"
      "const VK_NEXT = "+ZString(VK_NEXT)+";\n"
      "const VK_END = "+ZString(VK_END)+";\n"
      "const VK_HOME = "+ZString(VK_HOME)+";\n"
      "const VK_LEFT = "+ZString(VK_LEFT)+";\n"
      "const VK_UP = "+ZString(VK_UP)+";\n"
      "const VK_RIGHT = "+ZString(VK_RIGHT)+";\n"
      "const VK_DOWN = "+ZString(VK_DOWN)+";\n"
      "const VK_SELECT = "+ZString(VK_SELECT)+";\n"
      "const VK_PRINT = "+ZString(VK_PRINT)+";\n"
      "const VK_EXECUTE = "+ZString(VK_EXECUTE)+";\n"
      "const VK_SNAPSHOT = "+ZString(VK_SNAPSHOT)+";\n"
      "const VK_INSERT = "+ZString(VK_INSERT)+";\n"
      "const VK_DELETE = "+ZString(VK_DELETE)+";\n"
      "const VK_HELP = "+ZString(VK_HELP)+";\n"
      "const VK_LWIN = "+ZString(VK_LWIN)+";\n"
      "const VK_RWIN = "+ZString(VK_RWIN)+";\n"
      "const VK_APPS = "+ZString(VK_APPS)+";\n"
      "const VK_NUMPAD0 = "+ZString(VK_NUMPAD0)+";\n"
      "const VK_NUMPAD1 = "+ZString(VK_NUMPAD1)+";\n"
      "const VK_NUMPAD2 = "+ZString(VK_NUMPAD2)+";\n"
      "const VK_NUMPAD3 = "+ZString(VK_NUMPAD3)+";\n"
      "const VK_NUMPAD4 = "+ZString(VK_NUMPAD4)+";\n"
      "const VK_NUMPAD5 = "+ZString(VK_NUMPAD5)+";\n"
      "const VK_NUMPAD6 = "+ZString(VK_NUMPAD6)+";\n"
      "const VK_NUMPAD7 = "+ZString(VK_NUMPAD7)+";\n"
      "const VK_NUMPAD8 = "+ZString(VK_NUMPAD8)+";\n"
      "const VK_NUMPAD9 = "+ZString(VK_NUMPAD9)+";\n"
      "const VK_MULTIPLY = "+ZString(VK_MULTIPLY)+";\n"
      "const VK_ADD = "+ZString(VK_ADD)+";\n"
      "const VK_SEPARATOR = "+ZString(VK_SEPARATOR)+";\n"
      "const VK_SUBTRACT = "+ZString(VK_SUBTRACT)+";\n"
      "const VK_DECIMAL = "+ZString(VK_DECIMAL)+";\n"
      "const VK_DIVIDE = "+ZString(VK_DIVIDE)+";\n"
      "const VK_F1 = "+ZString(VK_F1)+";\n"
      "const VK_F2 = "+ZString(VK_F2)+";\n"
      "const VK_F3 = "+ZString(VK_F3)+";\n"
      "const VK_F4 = "+ZString(VK_F4)+";\n"
      "const VK_F5 = "+ZString(VK_F5)+";\n"
      "const VK_F6 = "+ZString(VK_F6)+";\n"
      "const VK_F7 = "+ZString(VK_F7)+";\n"
      "const VK_F8 = "+ZString(VK_F8)+";\n"
      "const VK_F9 = "+ZString(VK_F9)+";\n"
      "const VK_F10 = "+ZString(VK_F10)+";\n"
      "const VK_F11 = "+ZString(VK_F11)+";\n"
      "const VK_F12 = "+ZString(VK_F12)+";\n"
      "const VK_F13 = "+ZString(VK_F13)+";\n"
      "const VK_F14 = "+ZString(VK_F14)+";\n"
      "const VK_F15 = "+ZString(VK_F15)+";\n"
      "const VK_F16 = "+ZString(VK_F16)+";\n"
      "const VK_F17 = "+ZString(VK_F17)+";\n"
      "const VK_F18 = "+ZString(VK_F18)+";\n"
      "const VK_F19 = "+ZString(VK_F19)+";\n"
      "const VK_F20 = "+ZString(VK_F20)+";\n"
      "const VK_F21 = "+ZString(VK_F21)+";\n"
      "const VK_F22 = "+ZString(VK_F22)+";\n"
      "const VK_F23 = "+ZString(VK_F23)+";\n"
      "const VK_F24 = "+ZString(VK_F24)+";\n"
      "const VK_NUMLOCK = "+ZString(VK_NUMLOCK)+";\n"
      "const VK_SCROLL = "+ZString(VK_SCROLL)+";\n"
      "const VK_LSHIFT = "+ZString(VK_LSHIFT)+";\n"
      "const VK_RSHIFT = "+ZString(VK_RSHIFT)+";\n"
      "const VK_LCONTROL = "+ZString(VK_LCONTROL)+";\n"
      "const VK_RCONTROL = "+ZString(VK_RCONTROL)+";\n"
      "const VK_LMENU = "+ZString(VK_LMENU)+";\n"
      "const VK_RMENU = "+ZString(VK_RMENU)+";\n"
      "const VK_PROCESSKEY = "+ZString(VK_PROCESSKEY)+";\n"
      "const VK_ATTN = "+ZString(VK_ATTN)+";\n"
      "const VK_CRSEL = "+ZString(VK_CRSEL)+";\n"
      "const VK_EXSEL = "+ZString(VK_EXSEL)+";\n"
      "const VK_EREOF = "+ZString(VK_EREOF)+";\n"
      "const VK_PLAY = "+ZString(VK_PLAY)+";\n"
      "const VK_ZOOM = "+ZString(VK_ZOOM)+";\n"
      "const VK_NONAME = "+ZString(VK_NONAME)+";\n"
      "const VK_PA1 = "+ZString(VK_PA1)+";\n"
      "const VK_OEM_CLEAR = "+ZString(VK_OEM_CLEAR)+";\n"
   );
   istrstream str((char*)init);
   aCsl->loadScript(iFile, &str);

   (*aCsl)

      .addFunc(iFile,
         "winActivate("                  // bring window to foreground
            "const handle)",                // windows handle
         winActivate)

      .addFunc(iFile,
         "winClose("                     // close window
            "const handle)",                // windows handle
         winClose)

      .addFunc(iFile,
         "winCapsLock("                  // query or change caps lock state
            "[const mode])",                // none=query, 0=clear, 1=set, 2=toggle
         winCapsLock)

      .addFunc(iFile,
         "winFind("                      // find windows by name
            "const name,"                   // name or regular expression pattern
            "var &win[][],"                 // handle & name of each match
            "[const regular,"               // use regular expression (default = false)
             "const level])",               // subwindow search level (default = 0)
         winFind)                           // return = # of matches found

      .addFunc(iFile,
         "winHide("                      // hide window
            "const handle)",                // windows handle
         winHide)

      .addFunc(iFile,
         "winIsMaximized("               // is windows maximized?
            "const handle)",                // windows handle
         winIsMaximized)

      .addFunc(iFile,
         "winIsMinimized("               // is windows minimized?
            "const handle)",                // windows handle
         winIsMinimized)

      .addFunc(iFile,
         "winIsVisible("                 // is windows visible?
            "const handle)",                // windows handle
         winIsVisible)

      .addFunc(iFile,
         "winMaximize("                  // maximize window
            "const handle)",                // windows handle
         winMaximize)

      .addFunc(iFile,
         "winMinimize("                  // minimize window
            "const handle)",                // windows handle
         winMinimize)

      .addFunc(iFile,
         "winNumLock("                  // query or change num lock state
            "[const mode])",                // none=query, 0=clear, 1=set, 2=toggle
         winNumLock)

      .addFunc(iFile,
         "winPostText("                  // post a text
            "const text)",                  // text
         winPostText)

      .addFunc(iFile,
         "winPostVKey("                  // post virtual keys
            "const vkey1,"                  // virtual key code VK_....
            "[const vkey2,"                 // virtual key code VK_....
            " const vkey3])",               // virtual key code VK_....
         winPostVKey)

      .addFunc(iFile,
         "winPrintScreen("               // copy screen or acrive window to clipboard
            "[const mode])",                // none/0 = screen, 1=active window
         winPrintScreen)

      .addFunc(iFile,
         "winRestore("                   // restore window
            "const handle)",                // windows handle
         winRestore)

      .addFunc(iFile,
         "winScrollLock("                // query or change scroll lock state
            "[const mode])",                // none=query, 0=clear, 1=set, 2=toggle
         winScrollLock)

      .addFunc(iFile,
         "winShow("                      // show window
            "const handle)",                // windows handle
         winShow);

} // ZCslInitLib

ZCslCleanupLib(aCsl)
{
} // ZCslCleanupLib
