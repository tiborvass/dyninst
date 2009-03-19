/*
 * Copyright (c) 1996-2007 Barton P. Miller
 * 
 * We provide the Paradyn Parallel Performance Tools (below
 * described as "Paradyn") on an AS IS basis, and do not warrant its
 * validity or performance.  We reserve the right to update, modify,
 * or discontinue this software at any time.  We shall have no
 * obligation to supply such updates or modifications or any other
 * form of support to you.
 * 
 * By your use of Paradyn, you understand and agree that we (or any
 * other person or entity with proprietary rights in Paradyn) are
 * under no obligation to provide either maintenance services,
 * update services, notices of latent defects, or correction of
 * defects for Paradyn.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#if !defined(DYNTYPES_H)
#define DYNTYPES_H

#if defined(_MSC_VER)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#ifndef FILE__
#define FILE__ strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__
#endif

#if defined (_MSC_VER)
  //**************** Windows ********************
  #include <hash_map>
#if 1
  #define dyn_hash_map stdext::hash_map
#else
  #define dyn_hash_map std::hash_map
#endif
  #define DECLTHROW(x)
#elif defined(__GNUC__)
  #include <functional>
  #define DECLTHROW(x) throw(x)
  //***************** GCC ***********************
   #if (__GNUC__ > 4) || \
      (__GNUC__ == 4 && __GNUC_MINOR__ >= 3)
      //**************** GCC >= 4.3.0 ***********
      #include <tr1/unordered_set>
      #include <tr1/unordered_map>
      #define dyn_hash_set std::tr1::unordered_set
      #define dyn_hash_map std::tr1::unordered_map
   #else
      //**************** GCC < 4.3.0 ************
      #include <ext/hash_map>
      #include <ext/hash_set>
      #include <string>
      #define dyn_hash_set __gnu_cxx::hash_set
      #define dyn_hash_map __gnu_cxx::hash_map    
      using namespace __gnu_cxx;
      namespace __gnu_cxx {
 
        template<> struct hash<std::string> {
           hash<char*> h;
           unsigned operator()(const std::string &s) const 
	   {
	   const char *cstr = s.c_str();
             return h(cstr);
           };
        };
      }

   #endif
#else
   #error Unknown compiler
#endif


namespace Dyninst
{
   typedef unsigned long Address;   
   typedef unsigned long Offset;

   typedef signed int MachRegister;
   const signed int MachRegInvalid = -1;
   const signed int MachRegReturn = -2;    //Virtual register on some systems
   const signed int MachRegFrameBase = -3; //Virtual register on some systems
   const signed int MachRegPC = -4;
   const signed int MachRegStackBase = -5; //Virtual register on some systems
   const signed int ESP = 4;
   const signed int EBP = 5;
   const signed int RBP = 6;
   const signed int RSP = 7;
   typedef unsigned long MachRegisterVal;
   
#if defined(_MSC_VER)
   typedef int PID;
   typedef HANDLE PROC_HANDLE;
   typedef HANDLE LWP;
   typedef HANDLE THR_ID;

#define NULL_PID     INVALID_HANDLE_VALUE
#define NULL_LWP     INVALID_HANDLE_VALUE
#define NULL_THR_ID     INVALID_HANDLE_VALUE

#else
   typedef int PID;
   typedef int PROC_HANDLE;
   typedef int LWP;
   typedef int THR_ID;

#define NULL_PID     -1
#define NULL_LWP     -1
#define NULL_THR_ID     -1
#ifndef INVALID_HANDLE_VALUE
#define INVALID_HANDLE_VALUE -1
#endif
#endif

   int ThrIDToTid(Dyninst::THR_ID id);
}

#endif
