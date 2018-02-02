/* Copyright (c) 2001-2002 IBK-Landquart-Switzerland. All rights reserved.
 *
 * Module      :  ZcMathLb.cpp
 * Application :  CSL math library
 * Purpose     :  Implementation
 *
 * Date        Description                                 Who
 * --------------------------------------------------------------------------
 * 2001.05.30  First implementation                        P.Koch, IBK
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

#include <stdlib.h>
#include <math.h>
#include <errno.h>
#include <ZExcept.hpp>

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

static ZString mathAbs(ZCsl* csl)
{
   double x(csl->get("x").asDouble());
   if (x < 0) x = -x;
   return x;
} // mathAbs

static ZString mathAcos(ZCsl* csl)
{
   double x(csl->get("x").asDouble());
   if (x < -1 || x > 1)
      ZTHROWEXC("x must be in -1...1");
   return acos(x);
} // mathAcos

static ZString mathAsin(ZCsl* csl)
{
   double x(csl->get("x").asDouble());
   if (x < -1 || x > 1)
      ZTHROWEXC("x must be in -1...1");
   return asin(x);
} // mathAsin

static ZString mathAtan(ZCsl* csl)
{
   return atan(csl->get("x").asDouble());
} // mathAtan

static ZString mathCeil(ZCsl* csl)
{
   return ceil(csl->get("x").asDouble());
} // mathCeil

static ZString mathCos(ZCsl* csl)
{
   return cos(csl->get("x").asDouble());
} // mathCos

static ZString mathCosh(ZCsl* csl)
{
   return cosh(csl->get("x").asDouble());
} // mathCosh

static ZString mathExp(ZCsl* csl)
{
   return exp(csl->get("x").asDouble());
} // mathExp

static ZString mathFloor(ZCsl* csl)
{
   return floor(csl->get("x").asDouble());
} // mathFloor

static ZString mathLog(ZCsl* csl)
{
   double x(csl->get("x").asDouble());
   if (x <= 0)
      ZTHROWEXC("x must be positive");
   return log(x);
} // mathLog

static ZString mathLog10(ZCsl* csl)
{
   double x(csl->get("x").asDouble());
   if (x <= 0)
      ZTHROWEXC("x must be positive");
   return log10(x);
} // mathLog10

static ZString mathMax(ZCsl* csl)
{
   double x(csl->get("x").asDouble());
   double y(csl->get("y").asDouble());
   return y > x ? y : x;
} // mathMax

static ZString mathMin(ZCsl* csl)
{
   double x(csl->get("x").asDouble());
   double y(csl->get("y").asDouble());
   return y < x ? y : x;
} // mathMin

static ZString mathPow(ZCsl* csl)
{
   errno = 0;
   double p(pow(
      csl->get("x").asDouble(),
      csl->get("y").asDouble()
   ));
   if ( errno == EDOM )
      ZTHROWEXC("invalid arguments for x and y");
   if ( errno == ERANGE )
      ZTHROWEXC("result underrun/overrun");
   return p;
} // mathPow

static ZString mathSqrt(ZCsl* csl)
{
   double x(csl->get("x").asDouble());
   if (x < 0)
      ZTHROWEXC("x must be >= 0");
   return sqrt(x);
} // mathSqrt

static ZString mathSin(ZCsl* csl)
{
   return sin(csl->get("x").asDouble());
} // mathSin

static ZString mathSinh(ZCsl* csl)
{
   return sinh(csl->get("x").asDouble());
} // mathSinh

static ZString mathTan(ZCsl* csl)
{
   return tan(csl->get("x").asDouble());
} // mathTan

static ZString mathTanh(ZCsl* csl)
{
   return tanh(csl->get("x").asDouble());
} // mathTanh

ZCslInitLib(aCsl)
{
   ZString iFile("ZcMathLb");
   ZString init(ZString(
      "const mathVersion = '4.04';\n"
      "const mathCompiler = '")+ZC_COMPILER+"';\n"
      "const mathLibtype = '"+ZC_CSLLIBTYPE+"';\n"
      "const mathBuilt = '"+ZString(__DATE__)+" "+__TIME__+"';\n"
   );
   istrstream str((char*)init);
   aCsl->loadScript(iFile, &str);
   (*aCsl)
      .addFunc(iFile, "abs(const x)",          mathAbs)
      .addFunc(iFile, "acos(const x)",         mathAcos)
      .addFunc(iFile, "asin(const x)",         mathAsin)
      .addFunc(iFile, "atan(const x)",         mathAtan)
      .addFunc(iFile, "ceil(const x)",         mathCeil)
      .addFunc(iFile, "cos(const x)",          mathCos)
      .addFunc(iFile, "cosh(const x)",         mathCosh)
      .addFunc(iFile, "exp(const x)",          mathExp)
      .addFunc(iFile, "floor(const x)",        mathFloor)
      .addFunc(iFile, "log(const x)",          mathLog)
      .addFunc(iFile, "log10(const x)",        mathLog10)
      .addFunc(iFile, "max(const x, const y)", mathMax)
      .addFunc(iFile, "min(const x, const y)", mathMin)
      .addFunc(iFile, "pow(const x, const y)", mathPow)
      .addFunc(iFile, "sqrt(const x)",         mathSqrt)
      .addFunc(iFile, "sin(const x)",          mathSin)
      .addFunc(iFile, "sinh(const x)",         mathSinh)
      .addFunc(iFile, "tan(const x)",          mathTan)
      .addFunc(iFile, "tanh(const x)",         mathTanh);
} // ZCslInitLib

ZCslCleanupLib(aCsl)
{
} // ZCslCleanupLib
