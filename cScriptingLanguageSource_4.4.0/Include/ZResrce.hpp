/* Copyright (c) 2001 IBK-Landquart-Switzerland. All rights reserved.
 *
 * Module      :  ZResrce.cpp
 * Application :  IBK Open Class Library
 * Purpose     :  Resource locking classes:
 *                ZPrivateResource:
 *                  Resource for serialization within current process.
 *                ZSharedResource:
 *                  Resource for serialization of multiple processes.
 *                ZResourceLock:
 *                  Utility lock class to manage lock/unlock in a block of
 *                  code in conjunction with ZPrivateResource or
 *                  ZSharedResource.
 *                ZResource:
 *                  Virtual class ZPrivateResource and ZSharedResource are
 *                  derived of. Not intended for application use.
 *
 * Date        Description                                 Who
 * --------------------------------------------------------------------------
 * 2001.05.21  First implementation                        P.Koch, IBK
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

#ifndef _ZRESRCE_
#define _ZRESRCE_

#include <ZBase2.hpp>
#include <ZExcept.hpp>

class ZResourceLock;
class ZPrivateResource;
class ZSharedResource;

typedef void* ZResourceHandle;

class ZResource : public ZBase {
   public:
      ZBase2Link0 ZResource();
      virtual ZBase2Link0 ~ZResource();

      ZBase2Link(ZResource&) lock(long timeOut = -1);
      ZBase2Link(ZResource&) unlock();

   protected:
      virtual ZBase2Link(ZResourceHandle) handle() const = 0;

   private:
      ZResource(const ZResource& aResource);
      ZResource& operator=(const ZResource& aResource);

      ZResourceLock* iReslockGate;
      long iRefCount;

      friend class ZResourceLock;
      friend class ZPrivateResource;
      friend class ZSharedResource;
}; // ZResource

class ZResourceLock : public ZBase
{
   public:
      ZBase2Link0 ZResourceLock(ZResource* aResource, long aTimeOut = -1);
      ZBase2Link0 ~ZResourceLock();

   private:
      ZResourceLock(const ZResourceLock& aResourceLock);
      ZResourceLock& operator=(const ZResourceLock& aResourceLock);
      ZResourceLock& setLock(long aTimeOut = -1);
      ZResourceLock& clearLock();

      ZResource* iResource;

      friend class ZResource;
}; // ZResourceLock

class ZPrivateResource : public ZResource
{
   public:
      ZBase2Link0 ZPrivateResource();
      virtual ZBase2Link0 ~ZPrivateResource();

   protected:
      virtual ZResourceHandle handle() const { return iResourceHandle; }

   private:
      ZPrivateResource(const ZPrivateResource& aPrivateResource);
      ZPrivateResource& operator=(const ZPrivateResource& aPrivateResource);

      ZResourceHandle iResourceHandle;

      friend class ZResourceLock;
}; // ZPrivateResource

class ZSharedResource : public ZResource
{
   public:
      ZBase2Link0 ZSharedResource(const ZString& aKeyName);
      virtual ZBase2Link0 ~ZSharedResource();

      ZString keyName() const { return iKeyName; }

   protected:
      virtual ZResourceHandle handle() const { return iResourceHandle; }

   private:
      ZSharedResource(const ZSharedResource& aSharedResource);
      ZSharedResource& operator=(const ZSharedResource& aSharedResource);

      ZResourceHandle iResourceHandle;
      ZString iKeyName;

      friend class ZResourceLock;
}; // ZSharedResource

ZEXCLASSDECLARE(ZBase2Link, ZResourceLockTimeout, ZException)

#endif // _ZRESRCE_
