// LPJGuess.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"
#pragma hdrstop

#include "LPJGuess.h"
#include "framework.h"
#include <..\Plugins\Flow\Flow.h>

#include <DATE.HPP>
#include "guess.h"
#include "commandlinearguments.h"
#include "guessserializer.h"
#include "parallel.h"

#include "inputmodule.h"
#include "driver.h"
#include "commonoutput.h"

#include "bvoc.h"
#include "landcover.h"


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
bool LPJGuess::Guess_Standalone(FlowContext *pFlowContext, LPCTSTR initStr)
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

bool LPJGuess::Guess_Flow(FlowContext *pFlowContext, bool useInitialSeed)
   {
	if (pFlowContext->timing & GMT_INIT) // Init()
		return LPJGuess::Init_Guess(pFlowContext, "envision", "C:\\envision\\studyareas\\CalFEWS\\LPJGuess\\input\\global_cru_new.ins");

	if (pFlowContext->timing & GMT_CATCHMENT) // Init()
		return LPJGuess::Run_Guess(pFlowContext, "envision", "C:\\envision\\studyareas\\CalFEWS\\LPJGuess\\input\\global_cru_new.ins");
	
   return TRUE;
   }

bool LPJGuess::Init_Guess_Complete(FlowContext *pFlowContext, const char* input_module_name, const char* instruction_file)
{
	//need to pass name of *.ins file. {insfile="C:\\LPJ_GUESS\\input\\global_cru.ins" help=false parallel=false ...}	CommandLineArguments
	//int argc=2, char* argv[]
   //CommandLineArguments args(arg.argc, arg.argv);
		// Call the framework
	//framework(args);

	set_shell(new CommandLineShell(file_log));

	// The 'mission control' of the model, responsible for maintaining the
	// primary model data structures and containing all explicit loops through
	// space (grid cells/stands) and time (days and years).
	using std::auto_ptr;
	m_input_module = InputModuleRegistry::get_instance().create_input_module(input_module_name);
	//GuessOutput::OutputModuleContainer output_modules;
	GuessOutput::OutputModuleRegistry::get_instance().create_all_modules(m_output_modules);

	read_instruction_file(instruction_file);
	m_input_module->init();
	m_output_modules.init();

	print_logfile_heading();


	if (ifnlim && !ifcentury) {
		fail("\n\nIf nitrogen limitation is switched on then century soil module also needs to be switched on!");
	}


	if (ifbvoc) {
		initbvoc();
	}

	//auto_ptr<GuessSerializer> serializer;
	//auto_ptr<GuessDeserializer> deserializer;

	if (save_state) {
		m_serializer = new GuessSerializer(state_path, GuessParallel::get_rank(), GuessParallel::get_num_processes());
	}

	if (restart) {
		m_deserializer = new GuessDeserializer(state_path);
	}


	FlowModel *pFlow = pFlowContext->pFlowModel;
	EnvModel *pEnvModel = pFlowContext->pEnvContext->pEnvModel;

	MapLayer *pIDULayer = (MapLayer*)pFlowContext->pEnvContext->pMapLayer;

	m_hruGridCells.SetSize(pFlow->GetHRUCount());
	for (int i = 0; i < pFlow->GetHRUCount(); i++)
		m_hruGridCells[i] = new GridCellIndexArray;

	UINT gridCellIndex = 0;
	UINT hruGridCellCount = 0;
	UINT hruAddedCount = 0;
	CUIntArray added;
	added.SetSize(pFlow->GetHRUCount());
	for (int p = 0; p < added.GetSize(); p++)
		added[p] = 0;

	while (true) {

		// START OF LOOP THROUGH GRID CELLS

		// Initialise global variable date
		// (argument nyear not used in this implementation)
		date.init(1);

		// Create and initialise a new Gridcell object for each locality
		Gridcell *pGridcell = new Gridcell;
		gridCellIndex++;
		//	read_instruction_file(instruction_file);
		  // Call input module to obtain latitude and driver data for this grid cell.
		if (!m_input_module->getgridcell(*pGridcell))
		{
			delete pGridcell;
			break;
		}

		// Initialise certain climate and soil drivers
		pGridcell->climate.initdrivers(pGridcell->get_lat());

		if (run_landcover && !restart)
		{
			// Read landcover and cft fraction data from 
			// data files for the spinup period and create stands
			landcover_init(*pGridcell, m_input_module);
		}

		if (restart) {
			// Get the whole grid cell from file...
			m_deserializer->deserialize_gridcell(*pGridcell);
			// ...and jump to the restart year
			date.year = state_year;
		}

		float y = pGridcell->get_lat();
		float x = pGridcell->get_lon();

		int hruCount = pFlow->GetHRUCount();
		HRU *pHRU = nullptr;

		//Check that this gridcell is contained by the map. 
		//Add it to the m_gridCellArray if it is.
		//If it isn't, we do not want to include it in the simulation
		for (int h = 0; h < hruCount; h++)
		   {
			pHRU = pFlowContext->pFlowModel->GetHRU(h);
			// if center of grid cell is in an hru poly 
			bool found = false;

			for (int i = 0; i < pHRU->m_polyIndexArray.GetSize(); i++)
				{
				Poly *pPoly = pIDULayer->GetPolygon(pHRU->m_polyIndexArray[i]);

				if (pPoly->IsPointInPoly(Vertex(x, y)))
					{
					m_gridCellArray.Add(pGridcell);
					hruGridCellCount++;
					found = true;
					break;
					}
				}
		   }


		//Find all the HRUs that will be represented by gridcells
	//	for (int z = 0; z < m_gridCellArray.GetSize(); z++)
		 //   {
			for (int h = 0; h < hruCount; h++)
			   {
				pHRU = pFlowContext->pFlowModel->GetHRU(h);
				Vertex cen = pHRU->m_centroid;
				float dist = sqrt((cen.x - x)*(cen.x - x) + (cen.y - y)*(cen.y - y));
				if (dist < 3000.0f && added[h] != 1)//then this hru is close to this LPJ gridcell
				    {
					pGridcell->m_hruArray.Add(pHRU);//then add the HRU to the gridcell
					added[h] = 1;//and flag this HRU so we don't add it to another gridcell
					hruAddedCount++;
				    }
				
			    }
			
		  //  }

	}// end of while

	CString msgStart;
	msgStart.Format(_T("LPJGuess:  %i gridcells read. %i gridcells associated with %i HRUs"), gridCellIndex, hruGridCellCount, hruAddedCount);
	Report::Log(msgStart);
	return TRUE;
}

bool LPJGuess::Init_Guess(FlowContext *pFlowContext, const char* input_module_name, const char* instruction_file)
{
	//need to pass name of *.ins file. {insfile="C:\\LPJ_GUESS\\input\\global_cru.ins" help=false parallel=false ...}	CommandLineArguments
	//int argc=2, char* argv[]
   //CommandLineArguments args(arg.argc, arg.argv);
		// Call the framework
	//framework(args);

	set_shell(new CommandLineShell(file_log));

	// The 'mission control' of the model, responsible for maintaining the
	// primary model data structures and containing all explicit loops through
	// space (grid cells/stands) and time (days and years).
	using std::auto_ptr;
	m_input_module=InputModuleRegistry::get_instance().create_input_module(input_module_name);
	//GuessOutput::OutputModuleContainer output_modules;
	GuessOutput::OutputModuleRegistry::get_instance().create_all_modules(m_output_modules);

	read_instruction_file(instruction_file);
	m_input_module->init();
	m_output_modules.init();

	print_logfile_heading();


	if (ifnlim && !ifcentury) {
		fail("\n\nIf nitrogen limitation is switched on then century soil module also needs to be switched on!");
	}


	if (ifbvoc) {
		initbvoc();
	}

	//auto_ptr<GuessSerializer> serializer;
	//auto_ptr<GuessDeserializer> deserializer;

	if (save_state) {
		m_serializer = new GuessSerializer(state_path, GuessParallel::get_rank(), GuessParallel::get_num_processes());
	}

	if (restart) {
		m_deserializer =  new GuessDeserializer(state_path);
	}

	
   FlowModel *pFlow = pFlowContext->pFlowModel;
   EnvModel *pEnvModel = pFlowContext->pEnvContext->pEnvModel;

   MapLayer *pIDULayer = (MapLayer*)pFlowContext->pEnvContext->pMapLayer;

   m_hruGridCells.SetSize(pFlow->GetHRUCount());
   for (int i = 0; i < pFlow->GetHRUCount(); i++)
      m_hruGridCells[i] = new GridCellIndexArray;

   UINT gridCellIndex = 0;
   UINT hruGridCellCount = 0;
	
   while (true) {

		// START OF LOOP THROUGH GRID CELLS

		// Initialise global variable date
		// (argument nyear not used in this implementation)
		date.init(1);

		// Create and initialise a new Gridcell object for each locality
		Gridcell *pGridcell = new Gridcell;
	//	read_instruction_file(instruction_file);
      // Call input module to obtain latitude and driver data for this grid cell.
		if (!m_input_module->getgridcell(*pGridcell))
			 {
			 delete pGridcell;
			 break;
			 }
		   
      m_gridCellArray.Add(pGridcell);



		// Initialise certain climate and soil drivers
      pGridcell->climate.initdrivers(pGridcell->get_lat());

		if (run_landcover && !restart) 
         {
			// Read landcover and cft fraction data from 
			// data files for the spinup period and create stands
			landcover_init(*pGridcell, m_input_module);
		   }

		if (restart) {
			// Get the whole grid cell from file...
			m_deserializer->deserialize_gridcell(*pGridcell);
			// ...and jump to the restart year
			date.year = state_year;
		}

		float y = pGridcell->get_lat();
		float x = pGridcell->get_lon();
    
      int hruCount = pFlow->GetHRUCount();
      HRU *pHRU = nullptr;

		for (int h = 0; h < hruCount; h++)
		   {
			pHRU = pFlowContext->pFlowModel->GetHRU(h);

			 // if center of grid cell is in an hru poly 
			 bool found = false;

			 for (int i = 0; i < pHRU->m_polyIndexArray.GetSize(); i++)
				{
				Poly *pPoly = pIDULayer->GetPolygon(pHRU->m_polyIndexArray[i]);
				
				if (pPoly->IsPointInPoly(Vertex(x, y)))
				   {
				   pGridcell->pHRU = pHRU;
				   found = true;
				   break;
				   }
				}

			 if (found)
				{
				// add this gridcell to this HRU's grid cell array
				m_hruGridCells[h]->Add(gridCellIndex);
				hruGridCellCount++;
   				m_gridCellHRUArray.Add(pHRU);
				}
		   }
      gridCellIndex++;

	}// end of while

   CString msgStart;
   msgStart.Format(_T("LPJGuess:  %i gridcells read. %i associated with HRUs"), gridCellIndex, hruGridCellCount);
   Report::Log(msgStart);
	return TRUE;
}

bool LPJGuess::Run_Guess(FlowContext *pFlowContext, const char* input_module_name, const char* instruction_file)
{

	//using std::auto_ptr;



	//auto_ptr<InputModule> input_module(InputModuleRegistry::get_instance().create_input_module(input_module_name));

	//GuessOutput::OutputModuleContainer output_modules;
	//GuessOutput::OutputModuleRegistry::get_instance().create_all_modules(output_modules);


	//read_instruction_file(instruction_file);



	//input_module->init();
	//output_modules.init();

//	print_logfile_heading();


//	if (ifnlim && !ifcentury) {
//		fail("\n\nIf nitrogen limitation is switched on then century soil module also needs to be switched on!");
//	}


//	if (ifbvoc) {
//		initbvoc();
//	}

	/*
	auto_ptr<GuessSerializer> serializer;
	auto_ptr<GuessDeserializer> deserializer;

	if (save_state) {
		serializer = auto_ptr<GuessSerializer>(new GuessSerializer(state_path, GuessParallel::get_rank(), GuessParallel::get_num_processes()));
	}

	if (restart) {
		deserializer = auto_ptr<GuessDeserializer>(new GuessDeserializer(state_path));
	}
	*/

	// int catchmentCount = pFlowContext->pFlowModel->GetCatchmentCount();
	int hruCount = pFlowContext->pFlowModel->GetHRUCount();

	ParamTable *pHBVTable = pFlowContext->pFlowModel->GetTable("HBV");   // store this pointer (and check to make sure it's not NULL)

	int doy = pFlowContext->dayOfYear;  // int( fmod( timeInRun, 364 ) );  // zero based day of year
	int _month = 0; int _day; int _year;
	BOOL ok = ::GetCalDate(doy, &_year, &_month, &_day, TRUE);

	// iterate through hrus/hrulayers 

	for (int h = 0; h < hruCount; h++)
	{
		HRU *pHRU = pFlowContext->pFlowModel->GetHRU(h);

		int gridCellCount = m_hruGridCells[h]->GetSize();

		for (int i = 0; i < gridCellCount; i++)
		{
			// Call input/output to obtain climate, insolation and CO2 for this
			// day of the simulation. Function getclimate returns false if last year
			// has already been simulated for this grid cell

			int gridCellIndex = m_hruGridCells[h]->GetAt(i);

			Gridcell *pGridcell = m_gridCellArray[gridCellIndex];

			m_input_module->getclimate(*pGridcell, pFlowContext);

			simulate_day(*pGridcell, m_input_module);

			m_output_modules.outdaily(*pGridcell);

			if (date.islastday && date.islastmonth) {
				// LAST DAY OF YEAR
				// Call output module to output results for end of year
				// or end of simulation for this grid cell
				m_output_modules.outannual(*pGridcell);

				pGridcell->balance.check_year(*pGridcell);

				// Time to save state?
				if (date.year == state_year - 1 && save_state) {
					m_serializer->serialize_gridcell(*pGridcell);
				}

				// Check whether to abort
				if (abort_request_received()) {
					return 99;
				}




				// End of loop through simulation days
			//}	//while (getclimate())

				//pGridcell->balance.check_period(*pGridcell);
			}
			// Advance timer to next simulation day


		}
	}
	date.next();
	//Assume each HRU has a array of pointers to guess gridcells
	//framework(pFlowContext, "cru_ncep", "C:\\envision\\studyareas\\CalFEWS\\LPJGuess\\input\\global_cru_new.ins");
	return TRUE;
}

bool LPJGuess::Run_Guess_Complete(FlowContext *pFlowContext, const char* input_module_name, const char* instruction_file)
{

	int hruCount = pFlowContext->pFlowModel->GetHRUCount();
	int gridCellCount = m_gridCellArray.GetSize();
	ParamTable *pHBVTable = pFlowContext->pFlowModel->GetTable("HBV");   // store this pointer (and check to make sure it's not NULL)

	int doy = pFlowContext->dayOfYear;  // int( fmod( timeInRun, 364 ) );  // zero based day of year
	int _month = 0; int _day; int _year;
	BOOL ok = ::GetCalDate(doy, &_year, &_month, &_day, TRUE);

	// iterate through hrus/hrulayers 

	for (int h = 0; h < gridCellCount; h++)
	   {
			Gridcell *pGridcell = m_gridCellArray[h];

			m_input_module->getclimate(*pGridcell, pFlowContext);

			simulate_day(*pGridcell, m_input_module);

			m_output_modules.outdaily(*pGridcell);

			if (date.islastday && date.islastmonth) {
				// LAST DAY OF YEAR
				// Call output module to output results for end of year
				// or end of simulation for this grid cell
				m_output_modules.outannual(*pGridcell);

				pGridcell->balance.check_year(*pGridcell);

				// Time to save state?
				if (date.year == state_year - 1 && save_state) {
					m_serializer->serialize_gridcell(*pGridcell);
				}

				// Check whether to abort
				if (abort_request_received()) {
					return 99;
				}

			}
	}
	date.next();
	return TRUE;
}