// LPJGuess.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"
#pragma hdrstop

#include "LPJGuess.h"
#include "framework.h"
#include <..\Plugins\Flow\Flow.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#endif

xtring file_log = "guess.log";
//extern "C" _EXPORT EnvExtension* Factory(EnvContext*) { return (EnvExtension*) new LPJGuess; }


// constructor
//LPJGuess::~LPJGuess(void)
 //  {
  // }

// override API Methods
bool LPJGuess::Init_Guess(FlowContext *pFlowContext, LPCTSTR initStr)
   {
	//need to pass name of *.ins file. {insfile="C:\\LPJ_GUESS\\input\\global_cru.ins" help=false parallel=false ...}	CommandLineArguments
	//int argc=2, char* argv[]
   //CommandLineArguments args(arg.argc, arg.argv);
		// Call the framework
	//framework(args);
	set_shell(new CommandLineShell(file_log));
	framework(pFlowContext, "cru_ncep", "C:\\envision\\studyareas\\CalFEWS\\LPJGuess\\input\\global_cru_new.ins");

   return TRUE;
   }

bool LPJGuess::InitRun(FlowContext *pFlowContext, bool useInitialSeed)
   {
   return TRUE;
   }

bool LPJGuess::Run(FlowContext *pFlowContext)
   {
   return TRUE;
   }
