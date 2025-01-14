// Flood.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


/*
This file is part of Envision.

Envision is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Envision is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Envision.  If not, see <http://www.gnu.org/licenses/>

Copywrite 2012 - Oregon State University

*/
// Flood.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"

#include "Flood.h"
#include <Maplayer.h>
#include <Map.h>
#include <Report.h>
#include <tixml.h>
#include <PathManager.h>
//#include <UNITCONV.H>
//#include <..\EnvEngine\envconstants.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


extern "C" _EXPORT EnvExtension* Factory(EnvContext*) { return (EnvExtension*) new Flood; }



Flood::Flood()
   : EnvModelProcess()
   , m_pMap(NULL)
   { }


bool Flood::Init(EnvContext *pContext, LPCTSTR initStr)
   {
   m_pMap = pContext->pMapLayer->GetMapPtr();
   LoadXml(initStr);
   return TRUE;
   }


bool Flood::InitRun(EnvContext *pContext, bool useInitialSeed)
   {
    
   return TRUE;
   }


// Allocates new DUs, Updates N_DU and NEW_DU
bool Flood::Run(EnvContext *pContext)
   {
   MapLayer *pLayer = (MapLayer*)pContext->pMapLayer;
   int iduCount = pLayer->GetRecordCount();

   for (int i = 0; i < m_floodAreaArray.GetSize(); i++)
      {
      FloodArea *pFloodArea = m_floodAreaArray[i];

      InitializeFloodGrids(pContext, pFloodArea);

      pFloodArea->Run(pContext);
      }

   return true;
   }

//------------------------------------------------------------
// Flood::initializeFloodGrids()
//  - called at the beginning of each year, to set up the 
//    FloodArea grids for running a flood simulation.
//------------------------------------------------------------

bool Flood::InitializeFloodGrids(EnvContext *pContext, FloodArea *pFloodArea)
   {
   // first, make sure required grids are available
   if (pFloodArea->m_pElevationGrid == NULL)
      return false;

   if (pFloodArea->m_pCellTypeGrid == NULL)
      return false;

   if (pFloodArea->m_pWaterSurfaceElevationGrid == NULL)
      return false;

   if (pFloodArea->m_pDischargeGrid == NULL)
      return false;

   if (pFloodArea->m_pWaterDepthGrid == NULL)
      return false;

   ///// Step 1: set the FloodArea's water elevation grid to zero initially
   pFloodArea->m_pWaterSurfaceElevationGrid->SetAllData(0, false);

   // set up a point source
   //pFloodArea->m_pCellTypeGrid->SetAllData(CT_MODEL, false);
   //pFloodArea->m_pCellTypeGrid->SetData(215, 160, CT_FIXEDHEIGHTSOURCE);
   //pFloodArea->m_pElevationGrid->SetAllData(1, false);
   //pFloodArea->m_pWaterSurfaceElevationGrid->SetAllData(1, false);
   //pFloodArea->m_pWaterSurfaceElevationGrid->SetData(215, 160, 8.0f);

   return true;
   } // end InitializeWaterElevationGrid


bool Flood::LoadXml(LPCTSTR filename)
   {
   CString fullPath;
   PathManager::FindPath(filename, fullPath);

   TiXmlDocument doc;
   bool ok = doc.LoadFile(fullPath);

   if (!ok)
      {
      Report::ErrorMsg(doc.ErrorDesc());
      return false;
      }

   // start iterating through the nodes
   TiXmlElement *pXmlRoot = doc.RootElement();  // coastalhazards
   if (pXmlRoot == NULL)
      return false;

   // flooded areas
   TiXmlElement *pXmlFloodAreas = pXmlRoot->FirstChildElement("flood_areas");
   if (pXmlFloodAreas != NULL)
      {
      // take care of any constant expressions
      TiXmlElement *pXmlFloodArea = pXmlFloodAreas->FirstChildElement(_T("flood_area"));
      while (pXmlFloodArea != NULL)
         {
         FloodArea *pFloodArea = FloodArea::LoadXml(pXmlFloodArea, filename, m_pMap);

         if (pFloodArea != NULL)
            {
            m_floodAreaArray.Add(pFloodArea);

            CString msg("  FloodArea: Loaded Flood Grid for layer ");
            msg += pFloodArea->m_cellTypeGridName;
            Report::Log(msg);
            }

         pXmlFloodArea = pXmlFloodArea->NextSiblingElement(_T("flood_area"));
         }
      }

   return true;
   }






