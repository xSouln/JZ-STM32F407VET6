//==============================================================================
#ifndef _TERMINAL_H
#define _TERMINAL_H
//------------------------------------------------------------------------------
#ifdef __cplusplus
 extern "C" {
#endif 
//==============================================================================
//includes:

#include "Terminal/Terminal_Types.h"
#include "Terminal/Adapters/Terminal_Adapters.h"
//==============================================================================
//types:

//==============================================================================
//functions:

xResult TerminalInit(void* parent);
void TerminalHandler();
void TerminalTimeSynchronization();
//==============================================================================
//export:

extern TerminalT Terminal;
//==============================================================================
#ifdef __cplusplus
}
#endif
//------------------------------------------------------------------------------
#endif
