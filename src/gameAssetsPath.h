
#pragma once

#ifndef _ASSETS_PATH_H_
#define _ASSETS_PATH_H_

#include <cstring>

#if defined (__APPLE__) && defined (__OBJC__)
# import <Foundation/Foundation.h>
#endif

#define TLOC_ASSETS_PATH "D:/Users/Altelus/Documents/Visual Studio 2013/Projects/IFNR3110/hw_h_12_100515125/assets/"

#if defined(TLOC_OS_WIN)

inline const char* GetAssetsPath()
{
static const char* assetPath = "D:/Users/Altelus/Documents/Visual Studio 2013/Projects/IFNR3110/hw_h_12_100515125/assets/";
  return assetPath;
}
#elif defined(TLOC_OS_IPHONE)
inline const char* GetAssetsPath()
{
  static char assetPath[1024];
  strcpy(assetPath, [[[NSBundle mainBundle] resourcePath]
                     cStringUsingEncoding:[NSString defaultCStringEncoding]]);
  strcat(assetPath, "/assets/");

  return assetPath;
}
#endif

#endif // _ASSETS_PATH_H_

