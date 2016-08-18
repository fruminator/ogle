/*=============================================================================
  GLIntercept - OpenGL intercept/debugging tool
  Copyright (C) 2004  Damian Trebilco

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
=============================================================================*/
#ifndef __COMMON_ERROR_LOG_H_
#define __COMMON_ERROR_LOG_H_

//Defines how logging is handled in Plugins
// This file must be defined and be in the include path so the common
// tools know how to log error messages. Other projects only need to define
// the following #defines to use the common tools. (can be empty defines for no logging)
#include "..\\..\\InterceptPluginInterface.h"

#define USING_ERRORLOG extern LOGERRPROC errorLog;
#define LOGMSG(string) { if(errorLog){errorLog string;}}  
#define LOGERR(string) { if(errorLog){errorLog string;}}  

#endif // __COMMON_ERROR_LOG_H_
