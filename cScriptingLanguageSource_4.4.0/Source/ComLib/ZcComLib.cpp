/* Copyright (c) 2001-2002 IBK-Landquart-Switzerland. All rights reserved.
 *
 * Module      :  ZcComLib.cpp
 * Application :  CSL async communications library
 * Purpose     :  Implementation
 *
 * Date        Description                                 Who
 * --------------------------------------------------------------------------
 * 2001.05.31  First implementation                        P.Koch, IBK
 * 2001.07.07  Renaming from css to csl                    P.Koch, IBK
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

#include <ZAsync.hpp>

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

static ZPointerlist* iDevList = 0;
static long iUseCount(0);

ZAsyncDevice* getDev(ZCsl* aCsl)
{
   ZAsyncDevice* dev = (ZAsyncDevice*)(aCsl->get("handle").asUnsignedLong());
   if (iDevList->find(dev) < 0)
      ZTHROWEXC("invalid com handle");
   return dev;
} // getDev

static ZString comOpen(ZCsl* aCsl)
{
   ZString logFile;
   if (aCsl->get("argCount").asInt() == 2)
      logFile = aCsl->get("logfile");

   ZAsyncDevice* dev = new ZAsyncDevice(aCsl->get("devname"), logFile);
   iDevList->addAsLast(dev);
   return (unsigned long)dev;
} // comOpen

static ZString comClose(ZCsl* aCsl)
{
   ZAsyncDevice* dev = getDev(aCsl);
   iDevList->removeFromPos(iDevList->find(dev));
   delete dev;
   return 1;
} // comClose

static ZString comRead(ZCsl* aCsl)
{
   int maxChars(1);
   if (aCsl->get("argCount").asInt() == 2)
      maxChars = aCsl->get("maxchars").asInt();
   return getDev(aCsl)->read(maxChars);
} // comRead

static ZString comReadChar(ZCsl* aCsl)
{
   char ch;
   if (getDev(aCsl)->readChar(ch))
      return ch;
   else
      return ZString();
} // comReadChar

static ZString comWrite(ZCsl* aCsl)
{
   getDev(aCsl)->write(aCsl->get("data"));
   return 1;
} // comWrite

static ZString comWaitForOutput(ZCsl* aCsl)
{
   getDev(aCsl)->waitForOutput();
   return 1;
} // comWaitForOutput

static ZString comPurgeInput(ZCsl* aCsl)
{
   getDev(aCsl)->purgeInput();
   return 1;
} // comPurgeInput

static ZString comInputChars(ZCsl* aCsl)
{
   return getDev(aCsl)->inputChars();
} // comInputChars

static ZString comReadTimeout(ZCsl* aCsl)
{
   ZAsyncDevice* dev = getDev(aCsl);
   if (aCsl->get("argCount").asInt() > 1)
      dev->setReadTimeout(aCsl->get("millisecs").asInt());
   return dev->readTimeout();
} // comReadTimeout

static ZString comBps(ZCsl* aCsl)
{
   ZAsyncDevice* dev = getDev(aCsl);
   if (aCsl->get("argCount").asInt() > 1)
      dev->setBps(aCsl->get("bps").asInt());
   return dev->bps();
} // comBps

static ZString comBits(ZCsl* aCsl)
{
   ZAsyncDevice* dev = getDev(aCsl);
   if (aCsl->get("argCount").asInt() > 1) {
      int bits = aCsl->get("bits").asInt();
      if (bits < 5 || bits > 8)
         ZTHROWEXC("invalid # of bits per char");
      dev->setBits((ZAsyncDevice::DataBits)bits);
   } // if
   return (int)dev->bits();
} // comBits

static ZString comParity(ZCsl* aCsl)
{
   ZAsyncDevice* dev = getDev(aCsl);
   if (aCsl->get("argCount").asInt() > 1) {
      ZString par = aCsl->get("parity").upperCase();
      if (par == "O")
         dev->setParity(ZAsyncDevice::parityOdd);
      else
      if (par == "E")
         dev->setParity(ZAsyncDevice::parityEven);
      else
      if (par == "M")
         dev->setParity(ZAsyncDevice::parityMark);
      else
      if (par == "S")
         dev->setParity(ZAsyncDevice::paritySpace);
      else
      if (par == "N")
         dev->setParity(ZAsyncDevice::parityNone);
      else
        ZTHROWEXC("invalid parity code: "+par);
   } // if
   switch (dev->parity()) {
      case ZAsyncDevice::parityOdd: return "O";
      case ZAsyncDevice::parityEven: return "E";
      case ZAsyncDevice::parityMark: return "M";
      case ZAsyncDevice::paritySpace: return "S";
      default:;
   } // switch
   return "N";
} // comParity

static ZString comStops(ZCsl* aCsl)
{
   ZAsyncDevice* dev = getDev(aCsl);
   if (aCsl->get("argCount").asInt() > 1) {
      double stops = aCsl->get("stopbits").asDouble();
      if (stops == 1.0)
         dev->setStops(ZAsyncDevice::stopBits1);
      else
      if (stops == 1.5)
         dev->setStops(ZAsyncDevice::stopBits1x5);
      else
      if (stops == 2.0)
         dev->setStops(ZAsyncDevice::stopBits2);
      else
         ZTHROWEXC("invalid # of stopbits");
   } // if
   switch (dev->stops()) {
      case ZAsyncDevice::stopBits1: return 1;
      case ZAsyncDevice::stopBits1x5: return 1.5;
      default:;
   } // switch
   return 2;
} // comStops

static ZString comFlow(ZCsl* aCsl)
{
   ZAsyncDevice* dev = getDev(aCsl);
   if (aCsl->get("argCount").asInt() > 1) {
      ZString par = aCsl->get("flow").upperCase();
      if (par == "N")
         dev->setFlow(ZAsyncDevice::flowNone);
      else
      if (par == "H")
         dev->setFlow(ZAsyncDevice::flowHard);
      else
      if (par == "S")
         dev->setFlow(ZAsyncDevice::flowSoft);
      else
      if (par == "B")
         dev->setFlow(ZAsyncDevice::flowBoth);
      else
        ZTHROWEXC("invalid flow control mode: "+par);
   } // if
   switch (dev->flow()) {
      case ZAsyncDevice::flowHard: return "H";
      case ZAsyncDevice::flowSoft: return "S";
      case ZAsyncDevice::flowBoth: return "B";
      default:;
   } // switch
   return "N";
} // comFlow

ZCslInitLib(aCsl)
{
   if (iUseCount++ == 0 && !iDevList)
      iDevList = new ZPointerlist;

   ZString iFile("ZcComLib");

   ZString init(ZString(
      "const comVersion = '4.04';\n"
      "const comCompiler = '")+ZC_COMPILER+"';\n"
      "const comLibtype = '"+ZC_CSLLIBTYPE+"';\n"
      "const comBuilt = '"+ZString(__DATE__)+" "+__TIME__+"';\n"
   );
   istrstream str((char*)init);
   aCsl->loadScript(iFile, &str);

   (*aCsl)
      .addFunc(iFile,
         "comOpen("
            "const devname, "
            "[const logfile])",
          comOpen)

      .addFunc(iFile,
         "comClose("
            "const handle)",
          comClose)

      .addFunc(iFile,
         "comRead("
            "const handle, "
            "[const maxchars])",
          comRead)

      .addFunc(iFile,
         "comReadChar("
            "const handle)",
          comReadChar)

      .addFunc(iFile,
         "comWrite("
            "const handle, "
            "const data)",
          comWrite)

      .addFunc(iFile,
         "comWaitForOutput("
            "const handle)",
          comWaitForOutput)

      .addFunc(iFile,
         "comPurgeInput("
            "const handle)",
          comPurgeInput)

      .addFunc(iFile,
         "comInputChars("
            "const handle)",
          comInputChars)

      .addFunc(iFile,
         "comReadTimeout("
            "const handle, "
            "[const millisecs])",
          comReadTimeout)

      .addFunc(iFile,
         "comBps("
            "const handle, "
            "[const bps])",
          comBps)

      .addFunc(iFile,
         "comBits("
            "const handle, "
            "[const bits])",
          comBits)

      .addFunc(iFile,
         "comParity("
            "const handle, "
            "[const parity])",
          comParity)

      .addFunc(iFile,
         "comStops("
            "const handle, "
            "[const stopbits])",
          comStops)

      .addFunc(iFile,
         "comFlow("
            "const handle, "
            "[const flow])",
          comFlow);
} // ZCslInitLib

ZCslCleanupLib(aCsl)
{
   if (--iUseCount == 0 && iDevList) {
      delete iDevList;
      iDevList = 0;
   } // if
} // ZCslCleanupLib
