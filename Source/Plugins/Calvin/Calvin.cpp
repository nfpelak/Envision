// Calvin.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"
#include "Calvin.h"

#include <Maplayer.h>
#include <tixml.h>
#include <Path.h>

#include <direct.h>
#include <EnvExtension.h>
#include <FDataObj.h>
#include <Vdataobj.h>

#ifdef _DEBUG
#undef _DEBUG
#include <python.h>
#define _DEBUG
#else
#include <python.h>
#endif
using namespace std;
#ifdef _DEBUG
#define new DEBUG_NEW
#endif

extern "C" _EXPORT EnvExtension* Factory(EnvContext*) { return (EnvExtension*) new Calvin; }
void CaptureException();

// C function extending python for capturing stdout from python
// = spam_system in docs
static PyObject *Redirection_stdoutredirect(PyObject *self, PyObject *args)
{
	const char *str;
	if (!PyArg_ParseTuple(args, "s", &str))
		return NULL;

	Report::Log(str);

	Py_INCREF(Py_None);
	return Py_None;
}

// create a "Method Table" for Redirection methods
// = SpamMethods in example
static PyMethodDef RedirectionMethods[] = {
	{"stdoutredirect", Redirection_stdoutredirect, METH_VARARGS,
		"stdout redirection helper"},
	{NULL, NULL, 0, NULL}
};

// define an "Module Definition" structure.
// this attaches the method table to this module
// = spammodule in example
static struct PyModuleDef redirectionmodule =
{
PyModuleDef_HEAD_INIT,
"redirecton", /* name of module */
"stdout redirection helper", /* module documentation, may be NULL */
-1,          /* size of per-interpreter state of the module, or -1 if the module keeps state in global variables. */
RedirectionMethods
};


PyMODINIT_FUNC PyInit_redirection(void)
{
	return PyModule_Create(&redirectionmodule);
}



Calvin::Calvin()
	: EnvModelProcess()
	//, m_pResData(NULL)
   , m_numRes(1)
   , m_reservoirList("")
   , m_outflowLinks("")
	, m_gwList("")
	, m_gwOutflowLinks("")
	, m_gwInflowLinks("")
	, m_gwBasinArray()
	, m_reservoirNameArray()
	//, m_randNormal(1, 1, 0)
	//, m_randUniform(0.0, 1.0, 0)

{ }



Calvin::~Calvin(void)
{
	// Clean up
	Py_Finalize();   // clean up python instance
}



bool Calvin::Init(EnvContext *pEnvContext, LPCTSTR initStr)
{
	bool ok = LoadXml(pEnvContext, initStr);
	if (!ok)
		return FALSE;

	MapLayer *pIDULayer = (MapLayer*)pEnvContext->pMapLayer;

	InitPython();

   //// Step 3.  Write monthly data into the Calvin input spreadsheet
   //VDataObj *pExchange = new VDataObj(6, 0, U_UNDEFINED);
   //pExchange->ReadAscii("c:\\envision\\studyareas\\calfews\\calvin\\linksWY1922a.csv", ',');
   //pExchange->WriteAscii("c:\\envision\\studyareas\\calfews\\calvin\\linksWY1922.csv");



   for (int i = 0; i < m_reservoirNameArray.GetSize(); i++)
      {
      FDataObj *pData = new FDataObj(4,0);

      pData->SetLabel(0, _T("Time"));
      pData->SetLabel(1, _T("Inflow (m3/s)"));
      pData->SetLabel(2, _T("Storage (acre/feet)"));
      pData->SetLabel(3, _T("Outflow (m3/s)"));

      this->AddOutputVar(m_reservoirNameArray.GetAt(i), pData, "");
      m_reservoirDataArray.Add(pData);
      }

	for (int i = 0; i < m_gwBasinArray.GetSize(); i++)
	   {
		FDataObj* pData = new FDataObj(4+ m_gwInflowLinkArray.GetSize(), 0);

		pData->SetLabel(0, _T("Time"));
		pData->SetLabel(1, _T("Inflow"));
		pData->SetLabel(2, _T("Storage (acre/feet)"));
		pData->SetLabel(3, _T(m_gwOutflowLinkArray[0]));
		for (int k = 0; k < m_gwInflowLinkArray.GetSize(); k++)
			pData->SetLabel(4 + k, _T(m_gwInflowLinkArray[k]));
		this->AddOutputVar(m_gwBasinArray.GetAt(i), pData, "");
		m_gwDataArray.Add(pData);
	   }
	
	return TRUE;
}

bool Calvin::InitRun(EnvContext *pEnvContext, bool useInitialSeed)
   {   
	return TRUE;
   }
bool Calvin::StartYear(EnvContext* pEnvContext)
   {


	return TRUE;
   }
bool Calvin::EndYear(EnvContext *pEnvContext)
   {
   // The Calvin simulated reservoir height and discharge have already been written to the output files.
   //  Here we write those data to the Calvin DataObjs.  This allows inflow to show up in Envision Output
//Step 1.  Get monthly data from Calvin
//   VDataObj *pExchange = new VDataObj(U_UNDEFINED);SR_PNF-C51
   
   int numRes = (int)m_reservoirNameArray.GetSize();
	int numGW = (int)m_gwBasinArray.GetSize();
   float *row = new float[4];//date, inflow, outflow, storage

   FDataObj *pExchange = new FDataObj();
   FDataObj *pStorage = new FDataObj();

   pStorage->ReadAscii("c:\\envision\\studyareas\\calfews\\calvin\\example-results\\storage.csv");
   pExchange->ReadAscii("c:\\envision\\studyareas\\calfews\\calvin\\example-results\\flow.csv");
   int yearOffset = pEnvContext->currentYear - pEnvContext->startYear;
   int inflowOffset = 0;
   int outflowOffset = 0;
   int storageOffset = 0;
	int gwOffset = 0;



   for (int i = 0; i < numRes; i++)
      {
      CString msg;
      CString name = m_reservoirNameArray.GetAt(i);
      FDataObj *pResData = m_reservoirDataArray.GetAt(i);
      //Step1.  Find Inflow column
      msg.Format(_T("INFLOW-%s"), (LPCTSTR)name);
      for (int j = 0; j < pExchange->GetColCount(); j++)//search columns to find correct name
         {
         CString lab = pExchange->GetLabel(j);
         inflowOffset = j;
         if (lab == msg)
            break;
         }
      //Step 2.  Find Outflow column     
      name = m_outflowLinkArray.GetAt(i);
      for (int j = 0; j < pExchange->GetColCount(); j++)//search columns to find correct name
         {
         CString lab = pExchange->GetLabel(j);
         outflowOffset = j;
         if (lab == name)
            break;
         }

      //Step 3.  Find Storage column
      name = m_reservoirNameArray.GetAt(i);
      for (int j = 0; j < pStorage->GetColCount(); j++)//search columns to find correct name
         {
         CString lab = pStorage->GetLabel(j);
         storageOffset = j;
         if (lab == name)
            break;
         }

		//Step 3.  Find any other inflow columns
		name = m_reservoirNameArray.GetAt(i);
		for (int j = 0; j < pStorage->GetColCount(); j++)//search columns to find correct name
		   {
			CString lab = pStorage->GetLabel(j);
			storageOffset = j;
			if (lab == name)
				break;
		   }

      for (int k = 0; k < 12; k++)//the output adds blank rows...This assumes the model was run for 1 year (so 12 rows total)
         {
         //float dat = pExchange->GetAsFloat(inflowOffset, k);//This is storage data
         //pResData->Add(2,k+(yearOffset*12),dat);
         row[0] = k+(12*yearOffset);
         row[1] = pExchange->GetAsFloat(inflowOffset, k);
         row[3] = pExchange->GetAsFloat(outflowOffset, k);
         row[2] = pStorage->GetAsFloat(storageOffset, k);
         pResData->AppendRow(row,4);
         }     
      }
   delete [] row;
	
	int* inflowOff = new int[m_gwInflowLinkArray.GetSize()];
	float* row_gw = new float[4 + m_gwInflowLinkArray.GetSize()];//date, inflow, outflow, storage, plus any other inflows
	for (int i = 0; i < numGW; i++)
	   {
		CString msg;
		CString name = m_gwBasinArray.GetAt(i);
		FDataObj* pGWData = m_gwDataArray.GetAt(i);
		//Step1.  Find Inflow column
		msg.Format(_T("INFLOW-%s"), (LPCTSTR)name);
		for (int j = 0; j < pExchange->GetColCount(); j++)//search columns to find correct name
		   {
			CString lab = pExchange->GetLabel(j);
			inflowOffset = j;
			if (lab == msg)
				break;
		   }
		//Step 2.  Find Outflow column     
		name = m_gwOutflowLinkArray.GetAt(i);
		for (int j = 0; j < pExchange->GetColCount(); j++)//search columns to find correct name
		   {
			CString lab = pExchange->GetLabel(j);
			outflowOffset = j;
			if (lab == name)
				break;
		   }

		//Step 3.  Find Storage column
		name = m_gwBasinArray.GetAt(i);
		for (int j = 0; j < pStorage->GetColCount(); j++)//search columns to find correct name
		   {
			CString lab = pStorage->GetLabel(j);
			storageOffset = j;
			if (lab == name)
				break;
		   }
		//Step 4.  Find other Inflow columns  

		for (int m=0;m<m_gwInflowLinkArray.GetSize();m++)
			{ 
			name = m_gwInflowLinkArray.GetAt(i);
			for (int j = 0; j < pExchange->GetColCount(); j++)//search columns to find correct name
			   {
				CString lab = pExchange->GetLabel(j);
				
				if (lab == name)
				   {
					inflowOff[m] = j;
					break;
					}
			   }
			}

		for (int k = 0; k < 12; k++)//the output adds blank rows...This assumes the model was run for 1 year (so 12 rows total)
		   {
			row_gw[0] = k + (12 * yearOffset);
			row_gw[1] = pExchange->GetAsFloat(inflowOffset, k);
			row_gw[2] = pStorage->GetAsFloat(storageOffset, k);
			row_gw[3] = pExchange->GetAsFloat(outflowOffset, k);
			for (int m = 0; m < m_gwInflowLinkArray.GetSize();m++)
				row_gw[4+m]= pExchange->GetAsFloat(inflowOff[m], k);

			pGWData->AppendRow(row_gw, 4 + (int)m_gwInflowLinkArray.GetSize());
		   }
	   }
   delete  pExchange;
   delete [] row_gw;
	delete [] inflowOff;

	//Write Storage values from LAST year into INITIAL values in the links file for next year
	CString nam;
	nam.Format(_T("%s%i%s"), "C:\\Envision\\StudyAreas\\CalFEWS\\calvin\\annual\\linksWY", pEnvContext->currentYear + 1, ".csv");//next years links file

	if ( pEnvContext->currentYear < pEnvContext->endYear)//Don't write values into the next years initial values if this is the last year
		{ 
		VDataObj* pLinks = new VDataObj(0, 0, U_UNDEFINED);
		pLinks->ReadAscii(nam, ',');
		int numStorage = 0;
		for (int link = 0; link < pLinks->GetRowCount(); link++)
			{
			CString b = pLinks->GetAsString(0, link);
			CString bb = b.Left(6);
			int b1 = strcmp(bb, "INITIA");
			if (b1 == 0)
				{
				float store= pStorage->Get(numStorage + 1, 11);
				pLinks->Set(5, link, store);
				pLinks->Set(6, link,store);
				numStorage++;
				}
			}
		pLinks->WriteAscii(nam);//this file will be read as input for next year's Calvin run
		delete pLinks;
		}
	delete pStorage;
   return TRUE;
   }

bool Calvin::Run(EnvContext *pEnvContext)
   {

   int retVal2;
	CString nam ;
	nam.Format(_T("%s%s.py"), (LPCTSTR)m_pyModulePath, (LPCTSTR)m_pyModuleName);
   FILE *file = _Py_fopen(nam, "r+");

	CString link;
	link.Format(_T("%s%i%s"), "C:\\Envision\\StudyAreas\\CalFEWS\\calvin\\annual\\linksWY", pEnvContext->currentYear, ".csv");
   if (file != NULL) 
	   {
		int argc;
		char* argv[1];
		argc = 1;

		//argv[0] = nam.GetBuffer(nam.GetLength());
		//wchar_t* pyfilepath = Py_DecodeLocale(argv[0], NULL);

		argv[0] = link.GetBuffer(link.GetLength());	
		wchar_t* linkpath = Py_DecodeLocale(argv[0], NULL);

		//PySys_SetArgvEx(0, &pyfilepath,0);
		PySys_SetArgvEx(1, &linkpath,0);

      retVal2 = PyRun_SimpleFile(file, nam);
		link.ReleaseBuffer();
		nam.ReleaseBuffer();
      }

   EndYear(pEnvContext);
	return TRUE;
   }


//Need to move data from Calvin files to Calvin DataObjs


/*
	int currentYear = pEnvContext->currentYear;

	// output the current IDU dataset as a DBF. Note that providing a bitArray would reduce size subtantially!
	//MapLayer *pIDULayer = (MapLayer*)pEnvContext->pMapLayer;
	//pIDULayer->SaveDataDB(this->m_envOutputPath, NULL);   // save entire IDU as DBF.  

	char cwd[512];
	_getcwd(cwd, 512);

	_chdir("c:/Envision/studyAreas/CalFEWS/Calvin");

	//CString code;
	//code.Format("sys.path.append(\"C:\\Envision\studyAreas\CalFEWS\Calvin\")");
	int retVal = PyRun_SimpleString("sys.path.append(\"C:\\Envision\\studyAreas\\CalFEWS\\Calvin\")");

	//int retVal2 = PyRun_SimpleString("sys.path.append(\"C:\\Program Files (x86)\\Microsoft Visual Studio\\Shared\\Anaconda2_64\\pkgs\\numpy-base-1.14.3-py27h917549b_1\\Lib\\site-packages\\numpy\")");

	//PyObject *pName = PyUnicode_DecodeFSDefault((LPCTSTR)this->m_pyModuleName);   // "e.g. "test1" (no .py)
	
	
	int retVal2;
	
	//PyObject *obj = Py_BuildValue("s", "C:\\Envision\\studyareas\\CalFEWS\\Calvin\\RunCalvin.py");
	//FILE *file = _Py_fopen_obj(obj, "r+");
	FILE *file = _Py_fopen("hello.py", "r+");

	if (file != NULL) {
		retVal2 = PyRun_SimpleFile(file, "hello.py");
	}
	
	//Dont use test.py as it actually searches sub module test>>py
	//PyObject * moduleName = PyUnicode_FromString("hello");
	//PyObject * pluginModule = PyImport_Import(moduleName);

	//if (pluginModule == nullptr)
	//{
	//	PyErr_Print();
	//	return "";
	//}
	
	PyObject *pNameC = PyUnicode_FromString("calvin");
	PyObject *pModuleC = NULL;

	try
	{
		pModuleC = PyImport_Import(pNameC);
	}
	catch (...)
	{
		CaptureException();
	}
	
	
	
	
	PyObject *pName = PyUnicode_FromString("RunCalvin");
	PyObject *pModule = NULL;

	try
	{
		pModule = PyImport_Import(pName);
	}
	catch (...)
	{
		CaptureException();
	}
	
	Py_DECREF(pName);

	if (pModule == nullptr)
		CaptureException();

	else
	   {
		CString msg("Calvin:  Running Python module ");
		msg += this->m_pyModuleName;
		Report::Log(msg);

		// pDict is a borrowed reference 
		PyObject *pDict = PyModule_GetDict(pModule);
		//pFunc is also a borrowed reference
		PyObject *pFunc = PyObject_GetAttrString(pModule, (LPCTSTR)m_pyFunction);
		if (pFunc && PyCallable_Check(pFunc))
		   {


//			PyObject* pScenario = PyUnicode_FromString((LPCTSTR)this->m_hazardScenario);/
//			PyObject* pInDBFPath = PyUnicode_FromString((LPCTSTR)this->m_envOutputPath);
//			PyObject* pOutCSVPath = PyUnicode_FromString((LPCTSTR)this->m_envInputPath);
			//PyObject *pName1 = PyUnicode_FromString("linksWY1922.csv");
			PyObject* pRetVal = PyObject_CallFunction(pFunc, NULL);
			
			if (pRetVal != NULL)
			{

				Py_DECREF(pRetVal);
			}
			else
			{
				CaptureException();
			}

			//Py_DECREF(pScenario);
			//Py_DECREF(pInDBFPath);
			//Py_DECREF(pOutCSVPath);
		}
		else
		{
			PyErr_Print();
		}

		Py_DECREF(pModule);
	}

	// grab input file generated from damage model (csv)
	//Report::Log("Acute Hazards: reading building damage parameter file");
	//this->m_hazData.ReadAscii(this->m_envInputPath, ',', 0);

	// propagate event outcomes to IDU's
	//this->Propagate(pEnvContext);

	//this->m_status = AHS_POST_EVENT;

	_getcwd(cwd, 512);

	_chdir(cwd);

	return TRUE;
}
*/

bool Calvin::InitPython()
{

	//VDataObj* pExchange = new VDataObj(0, 0, U_UNDEFINED);
	//pExchange->ReadAscii("C:\\Envision\\StudyAreas\\CalFEWS\\calvin\\annual\\linksWY1922.csv", ',');
	//pExchange->WriteAscii("C:\\Envision\\StudyAreas\\CalFEWS\\calvin\\annual\\linksWY1922b.csv");
	//VDataObj* pExchange1 = new VDataObj(0, 0, U_UNDEFINED);
	//pExchange1->ReadAscii("C:\\Envision\\StudyAreas\\CalFEWS\\calvin\\annual\\linksWY1922b.csv", ',');
	//pExchange1->WriteAscii("C:\\Envision\\StudyAreas\\CalFEWS\\calvin\\annual\\linksWY1922c.csv");
	//VDataObj* pExchange2 = new VDataObj(0, 0, U_UNDEFINED);
	//pExchange2->ReadAscii("C:\\Envision\\StudyAreas\\CalFEWS\\calvin\\annual\\linksWY1922c.csv", ',');
	//pExchange2->WriteAscii("C:\\Envision\\StudyAreas\\CalFEWS\\calvin\\annual\\linksWY1922d.csv");
	//delete pExchange;
	//delete pExchange1;
	//delete pExchange2;

	
	Report::Log("Calvin:  Initializing embedded Python interpreter...");

	char cwd[512];
	_getcwd(cwd, 512);


	// launch a python instance and run the model
	wchar_t path[1024];

	Py_SetProgramName(L"C:\\Users\\kelli\\anaconda3\\envs\\calvin35d\\python");  // argv[0]
//	Py_SetProgramName(L"python");
//	Py_SetPythonHome(L"Envision");
//	Py_SetProgramName(L"Envision");  // argv[0]
	/*int cx = swprintf(path, 512, L"%hs;%hsbin;%hsScripts;%hsLibrary\\bin;%hsLibrary\\usr\\bin;%hsLibrary\\mingw-w64\\bin;%hsLibrary\\lib;%hsLibrary\\include;%hsDLLs;%hslib;%hs;%hslib\\site-packages;%hsScripts",
		(LPCTSTR)m_pyModulePath, (LPCTSTR)m_pythonPath, (LPCTSTR)m_pythonPath, (LPCTSTR)m_pythonPath,
		(LPCTSTR)m_pyModulePath, (LPCTSTR)m_pythonPath, (LPCTSTR)m_pythonPath,
		(LPCTSTR)m_pyModulePath,(LPCTSTR)m_pythonPath, (LPCTSTR)m_pythonPath, 
		(LPCTSTR)m_pythonPath, (LPCTSTR)m_pythonPath, (LPCTSTR)m_pythonPath);
		*/
	int cx = swprintf(path, 512, L"%hs;%hsLibrary\\bin;%hsDLLs;%hslib;%hslib\\site-packages;%hslibrary",
		(LPCTSTR)m_pyModulePath, (LPCTSTR)m_pythonPath, (LPCTSTR)m_pythonPath,
		(LPCTSTR)m_pythonPath, (LPCTSTR)m_pythonPath, (LPCTSTR)m_pythonPath);
	
	Py_SetPath(path);
	PyImport_AppendInittab("redirection", PyInit_redirection);

	Py_Initialize();


	// add Python function for redirecting output on the Python side
	int retV = PyRun_SimpleString("\
import redirection\n\
import sys\n\
class StdoutCatcher:\n\
    def write(self, stuff):\n\
        redirection.stdoutredirect(stuff)\n\
sys.stdout = StdoutCatcher()");
	/////////////

	int retValy = PyRun_SimpleString("import sys");

	_chdir("/Envision/studyAreas/Calfews/Calvin");

	CString code;
	//code.Format("sys.path.append('%s')", (LPCTSTR)this->m_pyModulePath);
	code.Format("sys.path.append('%s')", (LPCTSTR)"C:\\Users\\kelli\\anaconda3\\envs\\calvin35d\\Library\\bin");
	int retVal = PyRun_SimpleString(code);
	FILE* file = _Py_fopen("test.py", "r+");
	int retVal23 = PyRun_SimpleFile(file, "test.py");
	int retVa1 = PyRun_SimpleString("import numpy");
	int retV1 = PyRun_SimpleString("from pyomo.opt import *");
	int retV9 = PyRun_SimpleString("from pyomo.opt.base.solvers import _extract_version");
	int rV9 = PyRun_SimpleString("from pyomo.opt.solver import SystemCallSolver");


	int ret = PyRun_SimpleString("import pyutilib.subprocess");
	int re1 = PyRun_SimpleString("from pyutilib.misc import Bunch, Options");
	int reV9 = PyRun_SimpleString("from pyutilib.services import register_executable, registered_executable");
	int r9 = PyRun_SimpleString("from pyutilib.services import TempfileManager");




	Report::Log((LPCTSTR)"Calvin: Python initialization complete...");

	const char* comp = Py_GetCompiler();
	const char* comp2 = Py_GetVersion();
	const char* comp4 = Py_GetCopyright();
	wchar_t* comp3 = Py_GetProgramFullPath();
	wchar_t* comp8 = Py_GetPythonHome();
	return true;
}



// Update is called during Run() processing.
// For any events that are in AHS_POST_EVENT status,
// do any needed update of IDUs recovering from that event 
// Notes: 
//  1) Any building tagged for repair will have "BLDG_DAMAGE" set to a negative value

bool Calvin::Update(EnvContext *pEnvContext)
{
	// basic idea is to loop through IDUs', applying any post-event
	// changes to the landscape, e.g. habitability, recovery period, etc.
	MapLayer *pIDULayer = (MapLayer*)pEnvContext->pMapLayer;

	//int idus = this->m_hazData.GetRowCount();
	int idus = pIDULayer->GetRowCount();

	// loop through IDU's, interpreting the probability information
	// from the damage model into the IDUs
	int coli = 1;
	for (int idu = 0; idu < idus; idu++)
	{
		int damage = 0;
		pIDULayer->GetData(idu, coli, damage);

		switch (damage)
		{
		case 1:  coli++; break;

		}


	}  // end of: for each idu



	return true;
}


bool Calvin::LoadXml(EnvContext *pEnvContext, LPCTSTR filename)
{
	
	// have xml string, start parsing
	TiXmlDocument doc;
	bool ok = doc.LoadFile(filename);

	if (!ok)
	{
		Report::ErrorMsg(doc.ErrorDesc());
		return false;
	}

	// start interating through the nodes
	TiXmlElement *pXmlRoot = doc.RootElement();  // <acute_hazards>
	CString codePath;


	XML_ATTR attrs[] = {
		// attr            type           address            isReq checkCol
		{ "python_path",  TYPE_CSTRING,   &m_pythonPath,     true,  0 },
		{ NULL,           TYPE_NULL,     NULL,               false, 0 } };
	ok = TiXmlGetAttributes(pXmlRoot, attrs, filename, NULL);

	TiXmlElement *pXmlEvent = pXmlRoot->FirstChildElement("event");
	if (pXmlEvent == NULL)
	{
		CString msg("Calvin: no <event>'s defined");
		msg += filename;
		Report::ErrorMsg(msg);
		return false;
	}
	while (pXmlEvent != NULL)
	{
		CString pyModulePath;
		CString pyFunction;
      CString resList;

		XML_ATTR attrs[] = {
			// attr              type           address            isReq checkCol

			{ "py_function",     TYPE_CSTRING,  &m_pyFunction,        true,  0 },
			{ "py_module",       TYPE_CSTRING,  &pyModulePath,        true,  0 },
         { "ReservoirList",   TYPE_CSTRING,  &m_reservoirList,     true,  0 },
         { "OutflowLinks",    TYPE_CSTRING,  &m_outflowLinks,      true,  0 },
			{ "GWList",          TYPE_CSTRING,  &m_gwList,            true,  0 },
			{ "GWOutflowLinks",  TYPE_CSTRING,  &m_gwOutflowLinks,    true,  0 },
			{ "GWInflowLinks",   TYPE_CSTRING,  &m_gwInflowLinks,     true,  0 },

			{ NULL,              TYPE_NULL,     NULL,        false, 0 } };
  
		ok = TiXmlGetAttributes(pXmlEvent, attrs, filename, NULL);

		if (!ok)
		   {
			CString msg;
			msg = "Calvin Model:  Error reading <event> tag in input file ";
			msg += filename;
			Report::ErrorMsg(msg);
		 	return false;
		   }
		else
		   {
			nsPath::CPath path(pyModulePath);
			m_pyModuleName = path.GetTitle();
			m_pyModulePath = path.GetPath();
         // parse reservoir names
         int _res = 0xFFFF;   // assume everything by default
         if (_res != NULL)
            {
            _res = 0;   // if specified, assume nothing by default

            TCHAR *buffer = new TCHAR[lstrlen(m_reservoirList) + 1];
            lstrcpy(buffer, m_reservoirList);
            TCHAR *next = NULL;
            TCHAR *token = _tcstok_s(buffer, _T(",+|:"), &next);

            while (token != NULL)
               {
               m_reservoirNameArray.Add(token);
               token = _tcstok_s(NULL, _T(",+|:"), &next);
               }

            delete[] buffer;
            
            int _out = 0xFFFF;
       //Outflow links
            if (_out != NULL)
               {
               _out = 0;   // if specified, assume nothing by default

               TCHAR *buffer = new TCHAR[lstrlen(m_outflowLinks) + 1];
               lstrcpy(buffer, m_outflowLinks);
               TCHAR *next = NULL;
               TCHAR *token = _tcstok_s(buffer, _T(",+|:"), &next);

               while (token != NULL)
                  {
                  m_outflowLinkArray.Add(token);
                  token = _tcstok_s(NULL, _T(",+|:"), &next);
                  }

               delete[] buffer;
               }

				int _gw = 0xFFFF;
				//Groundwater Storage
				if (_gw != NULL)
				{
					_gw = 0;   // if specified, assume nothing by default

					TCHAR* buffer = new TCHAR[lstrlen(m_gwList) + 1];
					lstrcpy(buffer, m_gwList);
					TCHAR* next = NULL;
					TCHAR* token = _tcstok_s(buffer, _T(",+|:"), &next);

					while (token != NULL)
					{
						m_gwBasinArray.Add(token);
						token = _tcstok_s(NULL, _T(",+|:"), &next);
					}

					delete[] buffer;
				}


				//GW Outflow links
				int _outgw = 0xFFFF;
				if (_outgw != NULL)
				{
					_outgw = 0;   // if specified, assume nothing by default

					TCHAR* buffer = new TCHAR[lstrlen(m_gwOutflowLinks) + 1];
					lstrcpy(buffer, m_gwOutflowLinks);
					TCHAR* next = NULL;
					TCHAR* token = _tcstok_s(buffer, _T(",+|:"), &next);

					while (token != NULL)
					{
						m_gwOutflowLinkArray.Add(token);
						token = _tcstok_s(NULL, _T(",+|:"), &next);
					}

					delete[] buffer;
				}


				//GW Inflow links
				int _ingw = 0xFFFF;
				if (_ingw != NULL)
				{
					_ingw = 0;   // if specified, assume nothing by default

					TCHAR* buffer = new TCHAR[lstrlen(m_gwInflowLinks) + 1];
					lstrcpy(buffer, m_gwInflowLinks);
					TCHAR* next = NULL;
					TCHAR* token = _tcstok_s(buffer, _T(",+|:"), &next);

					while (token != NULL)
					{
						m_gwInflowLinkArray.Add(token);
						token = _tcstok_s(NULL, _T(",+|:"), &next);
					}

					delete[] buffer;
				}

            }
         }

		pXmlEvent = pXmlEvent->NextSiblingElement("event");
	}	
	return true;
}
void CaptureException()
{
	PyObject *exc_type = NULL, *exc_value = NULL, *exc_tb = NULL;
	PyErr_Fetch(&exc_type, &exc_value, &exc_tb);
	PyObject* str_exc_type = PyObject_Repr(exc_type); //Now a unicode object
	PyObject* pyStr = PyUnicode_AsEncodedString(str_exc_type, "utf-8", "Error ~");
	const char *msg = PyBytes_AS_STRING(pyStr);

	Report::LogError(msg);

	Py_XDECREF(str_exc_type);
	Py_XDECREF(pyStr);

	Py_XDECREF(exc_type);
	Py_XDECREF(exc_value);
	Py_XDECREF(exc_tb);
}



