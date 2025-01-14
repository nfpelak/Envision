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


#include "stdafx.h"
#pragma hdrstop

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


#include "Flood.h"
#include <Maplayer.h>
#include <Report.h>
#include <PathManager.h>
#include <UNITCONV.H>
#include <COLORS.HPP>

#include <EnvisionAPI.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//--------------------------------------------------
// FloodArea ---------------------------------------
//--------------------------------------------------

//bool FloodArea::Init(EnvContext *pContext)
//   {
//   //bool ok = LoadXml(TiXmlNode *pNode);
//
//   MapLayer *pLayer = (MapLayer*)pContext->pMapLayer;
//   Map *pMap = pLayer->GetMapPtr();
//
//   InitGrids( pMap );
//
//   return true;
//   }

bool FloodArea::InitRun(EnvContext *pContext)
   {
   return true;
   }


bool FloodArea::Run(EnvContext *pContext)
   {
   StartYear(pContext);

   // run a flood for this year
   m_floodedArea = (float) GenerateFloodMap();

   // classify outputs
   m_pWaterDepthGrid->ClassifyData();
   m_pCumWaterDepthGrid->ClassifyData();
   m_pDischargeGrid->ClassifyData();

   return true;
   }


bool FloodArea::StartYear(EnvContext *pContext)
   {
   // set the water surface elevations = bed elevations at the beginning of the year
   // for all modeled cells
   for (int row = 0; row < m_rows; row++)
      {
      for (int col = 0; col < m_cols; col++)
         {
         int value;
         m_pCellTypeGrid->GetData(row, col, value);
         if (value == CT_MODEL)
            {
            float elev = 0;
            bool onElevGrid = GetCellElevation(row, col, elev);
            
            m_pWaterSurfaceElevationGrid->SetData(row, col, elev);
            m_pDischargeGrid->SetData(row, col, 0);
            m_pWaterDepthGrid->SetData(row, col, 0);
            }
         }
      }

   return true;
   }



//-------------------------------------------------------------
// FloodArea::CreateInternalGrids() 
//  - called at the end of LoadXml(), creates, sets up
//    the various grids used in this project
//-------------------------------------------------------------

int FloodArea::CreateInternalGrids( Map *pMap )
   {
   // currently called from CoastalHazards::LoadXml()

   // required grids
   m_pElevationGrid = pMap->GetLayer(m_elevGridName);
   //ASSERT(m_pElevationGrid != NULL);
   if (m_pElevationGrid == NULL)
      return -1;

   // autogenerate cell grid?
   if (this->m_cellTypeGridName.GetLength() < 1)
      GenerateCellTypeGridFromElevations();
   else
      m_pCellTypeGrid = pMap->GetLayer(m_cellTypeGridName);

   ASSERT(m_pCellTypeGrid != NULL);
   if (m_pCellTypeGrid == NULL)
      return -2;
      
   m_rows = m_pCellTypeGrid->GetRowCount();
   m_cols = m_pCellTypeGrid->GetColCount();

   // optional grids
   if ( m_manningsGridName.GetLength() > 0 )
      m_pManningsGrid = pMap->GetLayer(m_manningsGridName);

   // internally generated grids (match cell type grid)
   m_pWaterSurfaceElevationGrid = pMap->CloneLayer(*m_pCellTypeGrid);
   m_pWaterSurfaceElevationGrid->CreateDataTable(m_rows, m_cols, DOT_FLOAT);
   m_pWaterSurfaceElevationGrid->m_name = m_siteName + " Water Elevation Grid";
    
   m_pDischargeGrid = pMap->CloneLayer(*m_pCellTypeGrid);
   m_pDischargeGrid->CreateDataTable(m_rows, m_cols, DOT_FLOAT);
   m_pDischargeGrid->m_name = m_siteName + " Discharge Grid";
   
   m_pWaterDepthGrid = pMap->CloneLayer(*m_pCellTypeGrid);
   m_pWaterDepthGrid->CreateDataTable(m_rows, m_cols, DOT_FLOAT);
   m_pWaterDepthGrid->m_name = m_siteName + " Water Depth Grid";

   m_pCumWaterDepthGrid = pMap->CloneLayer(*m_pCellTypeGrid);
   m_pCumWaterDepthGrid->CreateDataTable(m_rows, m_cols, DOT_FLOAT);
   m_pCumWaterDepthGrid->m_name = m_siteName + " Cum. Water Depth Grid";

   // get offsets for cell grid, elevation grid
   ASSERT(m_pElevationGrid != NULL);
   ASSERT(m_pCellTypeGrid != NULL);

   REAL exMin, exMax, eyMin, eyMax;
   m_pElevationGrid->GetExtents(exMin, exMax, eyMin, eyMax);

   REAL cxMin, cxMax, cyMin, cyMax;
   m_pCellTypeGrid->GetExtents(cxMin, cxMax, cyMin, cyMax);

   REAL width, height;
   m_pCellTypeGrid->GetCellSize(width, height);

   REAL xDelta = cxMin - exMin;
   REAL yDelta = eyMax - cyMax;

   if (xDelta >= 0)
      m_colOffset = int((xDelta / width) + 0.1f);
   else
      m_colOffset = int((xDelta / width) - 0.1f);

   if (yDelta >= 0)
      m_rowOffset = int((yDelta / height) + 0.1f);
   else
      m_rowOffset = int((yDelta / height) - 0.1f);

   // initialize internally managed grids
   m_pWaterSurfaceElevationGrid->SetNoDataValue(-99);
   m_pDischargeGrid->SetNoDataValue(-99);
   m_pWaterDepthGrid->SetNoDataValue(-99);
   m_pCumWaterDepthGrid->SetNoDataValue(-99);

   //ASSERT(m_rowOffset >= 0);
   //ASSERT(m_colOffset >= 0);

   // classify the CellType grid
   // grid will have one BinArray.  Replace it with an appropriate one
   BinArray *pBinArray = (BinArray*)m_pCellTypeGrid->GetBinArray(0);
   ASSERT(pBinArray != NULL);

   pBinArray->SetSize(5);
   pBinArray->m_type = TYPE_INT;
   pBinArray->m_binMin = 0;
   pBinArray->m_binMax = 4;
   COLORREF colors[5] = { LTBLUE, RED, GREEN, LTCYAN, WHITE };
   TCHAR* labels[5] = { "Model Domain", "Fixed Height Source", "Hard Boundary", "Soft Boundary", "Off Grid" };

   for (int i = 0; i < 5; i++)
      {
      Bin &bin = pBinArray->ElementAt(i);
      bin.m_minVal = bin.m_maxVal =  (float) i;
      bin.SetColor(colors[i]);
      bin.SetLabel(labels[i]);

      if (i == 4) // off grid 
         bin.m_transparency = 100;
      }

   pBinArray->m_hasTransparentBins = true;

   m_pCellTypeGrid->ClassifyData();
   m_pCellTypeGrid->Show();

   // init other grid bins?
   float minVal = 0;
   float maxVal = 5;

   // water depth grid
   m_pWaterDepthGrid->SetBinColorFlag(BCF_BLUE_INCR);
   m_pWaterDepthGrid->SetBins(0, minVal, maxVal, 20, TYPE_FLOAT);
   m_pWaterDepthGrid->ClassifyData();
   m_pWaterDepthGrid->Hide();
   m_pWaterDepthGrid->GetBin(0, 0).SetTransparency(100); // 0 bin is transparent
   pBinArray = (BinArray*)m_pWaterDepthGrid->GetBinArray(0);
   pBinArray->m_hasTransparentBins = true;


   // cumulative water depth grid
   m_pCumWaterDepthGrid->SetBinColorFlag(BCF_BLUE_INCR);
   m_pCumWaterDepthGrid->SetBins(0, minVal, maxVal, 20, TYPE_FLOAT);
   m_pCumWaterDepthGrid->ClassifyData();
   m_pCumWaterDepthGrid->Hide();
   m_pCumWaterDepthGrid->GetBin(0, 0).SetTransparency(100); // 0 bin is transparent
   pBinArray = (BinArray*)m_pCumWaterDepthGrid->GetBinArray(0);
   pBinArray->m_hasTransparentBins = true;

   maxVal = 10;
   // water surface elevation grid
   m_pWaterSurfaceElevationGrid->SetBinColorFlag(BCF_BLUE_INCR);
   m_pWaterSurfaceElevationGrid->SetBins(0, minVal, maxVal, 10, TYPE_FLOAT);
   m_pWaterSurfaceElevationGrid->ClassifyData();
   m_pWaterSurfaceElevationGrid->Hide();

   maxVal = 10;
   // discharge grid
   m_pDischargeGrid->SetBinColorFlag(BCF_BLUE_INCR);
   m_pDischargeGrid->SetBins(0, minVal, maxVal, 10, TYPE_FLOAT);
   m_pDischargeGrid->ClassifyData();
   m_pDischargeGrid->Hide();
   m_pDischargeGrid->GetBin(0, 0).SetTransparency(100); // 0 bin is transparent
   pBinArray = (BinArray*)m_pDischargeGrid->GetBinArray(0);
   pBinArray->m_hasTransparentBins = true;

   m_pElevationGrid->Hide();
   m_pElevationGrid->GetBin(0, 0).SetTransparency(100); // 0 bin is transparent
   pBinArray = (BinArray*)m_pElevationGrid->GetBinArray(0);
   pBinArray->m_hasTransparentBins = true;

   return 1;
   }


double FloodArea::GenerateFloodMap()
   {
   int floodedCount = 0;
   CString method;

   if (m_method & BATES_MANNING )
      {
      floodedCount = RunBatesModel();
      method = "Bates (Manning) ";
      }

   if (m_method & BATES_2005)
      {
      floodedCount = RunBatesModel();
      method = "Bates (2005) ";
      }

    if (m_method & BATHTUB)
      {
      floodedCount = RunBathtubModel();
      method += "BathTub";
      }

   REAL width, height;
   m_pCellTypeGrid->GetCellSize(width, height);
   double floodedArea = floodedCount * (width * height);  // in sq meters

   CString msg;
   msg.Format("  FloodArea (%s): Flooded %i cells, (%.2f ha) using the %s model",
                         (LPCTSTR) m_siteName, floodedCount, floodedArea / M2_PER_HA, (LPCTSTR)method);
   Report::Log(msg);

   return floodedArea;
   }


// Runs the Bates flooding model returning the number of grid cells flooded and the area flooded
int FloodArea::RunBatesModel()
   {
   // Determine the area of the grid that was flooded

   //clock_t start = clock();

   // Run Bates flooding model 

   double time = 0.0f;
   int steps = 0;

   // basic idea - iterate though time, then the grid
   while (time < (m_timeDuration + (m_timeStep/2)))
      {
      CString msg;
      msg.Format("%s - Time: %f", (LPCTSTR) m_siteName, time);
      Report::StatusMsg(msg);
      m_pDischargeGrid->SetAllData(0, false);    // reset the discharge grid to start the time step

      double dtMax = 0;

      //-----------------------------------------------------------------------
      // first pass through the grid, calculate discharge for each model cell
      //-----------------------------------------------------------------------
///// #pragma omp parallel for
      for (int row = 0; row < m_rows; row++)
         {
         for (int col = 0; col < m_cols; col++)
            {
            CELL_TYPE cellType = GetCellType(row, col);
            if ( cellType != CT_MODEL )
               continue;

            // get the discharge at this location, based on the current water surface elevation grid
            // and the bed surface elevation grid. Look at four neighbors to determine 
            // net flux for this row/col in the model.
            double dh_dt = 0.0;
            CalculateQ(row, col, dh_dt, dtMax );
            
            if ( dh_dt > 0.0 )
               m_pDischargeGrid->SetData(row, col, (float) dh_dt);
            }  // end of: for each col
         }  // end of: for each row

      //--------------------------------------------------
      // Second pass, update water surface elevation grid
      //
      // We have discharge for each cell, do an Euler 
      // integration to update the surface elevation based
      // on the discharges calculated above
      //--------------------------------------------------
      //#pragma omp parallel for
      for (int row = 0; row < m_rows; row++)
         {
         for (int col = 0; col < m_cols; col++)
            {
            CELL_TYPE cellType = GetCellType(row, col);

            switch (cellType)
               {
               case CT_MODEL:
                  {
                  float discharge = 0.0f;
                  m_pDischargeGrid->GetData(row, col, discharge); // m/sec (dt_dt)

                  if (discharge != 0)
                     {
                     float waterElevation = 0.0f;
                     m_pWaterSurfaceElevationGrid->GetData(row, col, waterElevation);
                     m_pWaterSurfaceElevationGrid->SetData(row, col, waterElevation + (discharge * m_timeStep));

                     float depth = 0.0f;
                     m_pWaterDepthGrid->GetData(row, col, depth);
                     m_pWaterDepthGrid->SetData(row, col, depth + (discharge*m_timeStep));
                     }
                  }
                  break;

               case CT_FIXEDHEIGHTSOURCE:
                  {
                  float waterElevation = 0.0f;
                  m_pWaterSurfaceElevationGrid->GetData(row, col, waterElevation);
                  m_pWaterSurfaceElevationGrid->SetData(row, col, waterElevation + m_fixedSourceHeightDelta);

                  float depth = 0.0f;
                  m_pWaterDepthGrid->GetData(row, col, depth);
                  m_pWaterDepthGrid->SetData(row, col, depth + m_fixedSourceHeightDelta);
                  }
                  break;

               default:
                  break;
               }  // end of: switch( cellType)
            }  // end of: for each col
         } // end setting new water surface elevations, water depths grids (for output)
            

      //CString msg;
      //Report::StatusMsg(msg);

//      LogGrid(m_pDischargeGrid);
//      LogGrid(m_pWaterSurfaceElevationGrid);
      //
      if (m_updateDisplay)
         {
         UpdateWaterDepthGrids();
         ::EnvRedrawMap();
         //LogGrid(m_pWaterDepthGrid,215,160,10);
         }

      //msg.Format("  FloodArea: Maximum Delta T: %f", (float) dtMax);
      //Report::Log(msg);

      steps++;
      time = m_timeStep * steps;
      } // end time duration 

        // update timings
        //clock_t finish = clock();
        //float iduration = (float)(finish - start) / CLOCKS_PER_SEC;
        //
        //CString msg;
        //msg.Format("Calc4Q generated in %i seconds", (int)iduration);
        //Report::Log(msg);


   // finish up
   int floodedCount = UpdateWaterDepthGrids();
   return floodedCount;
   } // end RunBatesModel()


//----------------------------------------------------------------------------
// FloodArea::CalculateQ()
//  - calculates the net flux for cell[row,col].  
//  - ONLY CALLED FOR CELL TYPE=CT_MODEL
//    calculates fluxes for each of four cell edges (N/S/E/W) and returns
//    the net flux (m3/?)
//  dtMax = calcuated max time step for the conditions?
//----------------------------------------------------------------------------

bool FloodArea::CalculateQ(int row, int col, double &flux, double &dtMax )
   {
   //	CString msg;
   float manningCoeff = 0.0f;

   float neighborElevation = 0.0f;
   float neighborWaterElevation = 0.0f;

   flux = 0;
   REAL cellSize;
   m_pCellTypeGrid->GetCellSize(cellSize, cellSize);

   // we assume mannings N is specfied as sec/m^(1/3)
   if (m_pManningsGrid != NULL)
      m_pManningsGrid->GetData(row, col, manningCoeff);  // ignore masked areas
   else
      manningCoeff = m_manningsN;

   if (manningCoeff <= 0.0f)
      return false;

   // get ground, water surface elevations (meters)
   float thisElevation = 0.0f;  // meters
   float thisWaterElevation = 0.0f;   // meters

   bool inElevGrid = GetCellElevation(row, col, thisElevation);
   if (thisElevation < 0.0f)
      return false;

   m_pWaterSurfaceElevationGrid->GetData(row, col, thisWaterElevation);   // ignore areas with no water (WaterElevation = 0)
   if (thisWaterElevation < 0.0f)
      return false;

   for (int direction = 0; direction < 4; direction++)
      {
      int neighborRow = row;
      int neighborCol = col;

      switch (direction)
         {
         case 0:  //NORTH
            neighborRow--;		//Step to the north one grid cell 
            break;

         case 1:  //EAST
            neighborCol++;		//Step to the east one grid cell
            break;

         case 2:  //SOUTH
            neighborRow++;		//Step to the south one grid cell
            break;

         case 3:  //WEST
            neighborCol--;		//Step to the west one grid cell
            break;

         default:
            ASSERT(0);
            break;

         } // end switch

      CELL_TYPE neighborCellType = GetCellType(neighborRow, neighborCol);

      switch (neighborCellType)
         {
         case CT_MODEL:
         case CT_FIXEDHEIGHTSOURCE:
         case CT_SOFTBOUNDARY:
            {
            float neighborElevation = 0;
            bool inElevGrid = GetCellElevation( neighborRow, neighborCol, neighborElevation );

            if (inElevGrid)
               {
               float neighborWaterElevation = 0;

               if (neighborCellType == CT_MODEL || neighborCellType == CT_FIXEDHEIGHTSOURCE)
                  {
                  m_pWaterSurfaceElevationGrid->GetData(neighborRow, neighborCol, neighborWaterElevation);
                  
                  if (neighborCellType == CT_FIXEDHEIGHTSOURCE)
                     neighborWaterElevation += m_fixedSourceHeightDelta;
                  }
               else //if (neighborCellType == CT_SOFTBOUNDARY)
                  neighborWaterElevation = thisWaterElevation;

               float Q = 0;

               if (m_method & BATES_MANNING)
                  {
                  // form 1: from Bates 1995, 2000

                  // determine flow depth (highest water surface - highest bed elevation) (m)
                  double flowDepth = fmax(thisWaterElevation, neighborWaterElevation) - fmax(thisElevation, neighborElevation);   // meters
                  //double flowDepth = fabs(thisWaterElevation - thisElevation) - (neighborWaterElevation - neighborElevation);

                  // cross-sectional flow area (m2)
                  double csa = flowDepth * cellSize;

                  // hydraulic radius (flow area/wetted perimeter) (m)
                  double r = csa / (cellSize + (2 * flowDepth));

                  // water surface slope (positive if slope toward this, negative if sloping away from this) (m/m)
                  double slope = (neighborWaterElevation - thisWaterElevation) / cellSize;

                  // solve mannings equation
                  Q = float(csa * pow(r, (2.0 / 3.0))*sqrt(fabs(slope))) / manningCoeff;  // 1.49 for english units
                  if (slope < 0)
                     Q = -Q;
                  }
               else if (m_method & BATES_2005)
                  {
                  // form 2: from Bates 2005
                  double flowDepth = fmax(thisWaterElevation, neighborWaterElevation) - fmax(thisElevation, neighborElevation);   // meters

                  if (flowDepth > 0)
                     {
                     double deltaH = neighborWaterElevation - thisWaterElevation;

                     const double fiveThirds = 5.0 / 3.0;
                     double fd53 = pow(flowDepth, fiveThirds);
                     float sqrt_dhdx = (float) sqrt(fabs(deltaH / cellSize));

                     Q = float((fd53/manningCoeff) * sqrt_dhdx*cellSize);
                     if (deltaH < 0)  // flow leaving this cell?
                        Q = -Q;

                     /// not sure what this is?
                     double _dtMax = ((cellSize*cellSize) / 4)*(2 * manningCoeff*sqrt_dhdx) / fd53;
                     if (_dtMax > dtMax)
                        dtMax = _dtMax;
                     }
                  }

               flux += Q;

//                  CString msg;
//                  msg.Format("    FloodArea: Flux: This=(%i,%i),Neighbor=(%i,%i),flux=%.2f",
//                     row, col, neighborRow, neighborCol, (float)flux);
//                  Report::Log(msg);
               }
            }
         break;


         case CT_HARDBOUNDARY:
         case CT_OFFGRID:
            break;
         }  // end of :switch( neighborCellType)
      }// end of: for each direction

   flux /= (cellSize * cellSize);   // dh/dt, len/time
   return true;
   } // end CalculateFourQs



// Runs the flooding model returning the number of grid cells flooded and the area flooded
int FloodArea::RunBathtubModel()
   {
   if (m_pCellTypeGrid == NULL)
      return -1;

   // Value: 0 if not flooded, TWL if flooded
   float flooded = 0.0f;

   //clock_t start = clock();

   // For any fixed-point sources, propagate water as bathtub
   // Basic idea:  for each fixed-height point, propagate water surface elevations
   // Note: the discharge grid is not used in this case
   // Note: prior to this call, the fixed-point heights should
   // have been set in teh cell type grid, and the water surface elevations
   // should be set to the DEM elevations for all "dry" areas.
   int cellCount = 0;
   for (int row = 0; row < m_rows; row++)
      {
      for (int col = 0; col < m_cols; col++)
         {
         CELL_TYPE cellType = GetCellType(row, col);
         if (cellType != CT_FIXEDHEIGHTSOURCE)
            continue;

         // cell is a source, propagate
         float elev = 0;
         m_pWaterSurfaceElevationGrid->GetData(row, col, elev);
         int cells = PropagateBathtub(row, col, elev, 0);
         cellCount += cells;
         }
      }

   return cellCount;
   }


int FloodArea::PropagateBathtub(int row, int col, float waterElev, int steps )
   {
   if (m_stepLimit > 0 && steps >= m_stepLimit)
      return 0;

   int floodedCount = 0;

   CELL_TYPE cellType = GetCellType(row, col);
   if (cellType != CT_FIXEDHEIGHTSOURCE && cellType != CT_MODEL)
      return 0;

   // don't set water levels for fixed-height sources
   bool propagate = cellType == CT_FIXEDHEIGHTSOURCE ? true : false;

   if (cellType == CT_MODEL)
      {
      float thisWaterElev = 0;
      m_pWaterSurfaceElevationGrid->GetData(row, col, thisWaterElev);

      if (thisWaterElev > waterElev)
         {
         propagate = true;
         m_pWaterSurfaceElevationGrid->SetData(row, col, waterElev);

         // have we seen this one before?
         float bedElev = 0;
         GetCellElevation(row, col, bedElev);

         if ( thisWaterElev > bedElev )
            floodedCount++;
         }
      }

   if (propagate == false)
      return floodedCount;

   // propagate to neighbors
   for (int direction = 0; direction < 8; direction++)
      {
      int neighborRow = row;
      int neighborCol = col;

      switch (direction)
         {
         case 0:  //NORTH
            neighborRow--;		//Step to the north one grid cell 
            break;
         
         case 1:  //NORTHEAST
            neighborRow--;		//Step to the north one grid cell
            neighborCol++;		//Step to the east one grid cell
            break;
         
         case 2:  //EAST
            neighborCol++;		//Step to the east one grid cell
            break;
         
         case 3:  //SOUTHEAST
            neighborRow++;		//Step to the south one grid cell
            neighborCol++;		//Step to the east one grid cell
            break;
         
         case 4:  //SOUTH
            neighborRow++;		//Step to the south one grid cell
            break;
         
         case 5:  //SOUTHWEST
            neighborRow++;		//Step to the south one grid cell
            neighborCol--;		//Step to the west one grid cell
            break;
         
         case 6:  //WEST
            neighborCol--;		//Step to the west one grid cell
            break;
         
         case 7:  //NORTHWEST
            neighborRow--;		//Step to the north one grid cell
            neighborCol--;		//Step to the west one grid cell
            break;
         } // end switch
         
      CELL_TYPE neighborCellType = GetCellType(neighborRow, neighborCol);

      if (neighborCellType == CT_MODEL)
         floodedCount += PropagateBathtub(neighborRow, neighborCol, waterElev, steps+1);

      }  // end of: for each direction

   return floodedCount;
   }




/////////   for (MapLayer::Iterator point = m_pNewDuneLineLayer->Begin(); point < m_pNewDuneLineLayer->End(); point++)
/////////      {
/////////      int beachType = 0;
/////////      m_pNewDuneLineLayer->GetData(point, m_colBeachType, beachType);
/////////
/////////      int duneIndx = -1;
/////////      m_pNewDuneLineLayer->GetData(point, m_colDuneIndx, duneIndx);
/////////
/////////      REAL xCoord = 0.0;
/////////      REAL yCoord = 0.0;
/////////
/////////      int startRow = -1;
/////////      int startCol = -1;
/////////      float minElevation = 0.0f;
/////////      float yearlyMaxTWL = 0.0f;
/////////      float yearlyAvgTWL = 0.0f;
/////////      double duneCrest = 0.0;
/////////
/////////      if (beachType == BchT_BAY)
/////////         {
/////////         // get twl of inlet seed point
/////////         m_pNewDuneLineLayer->GetData(point, m_colYrFMaxTWL, yearlyMaxTWL);
/////////
/////////         //if (pEnvContext->yearOfRun < m_windowLengthTWL - 1)
/////////         //   m_pNewDuneLineLayer->GetData(point, m_colYrAvgTWL, yearlyMaxTWL);
/////////         //else
/////////         //   m_pNewDuneLineLayer->GetData(point, m_colMvMaxTWL, yearlyMaxTWL);
/////////
/////////         // traversing along bay shoreline, determine if twl is more than the elevation 
/////////
/////////         //for (int inletRow = 0; inletRow < m_numBayPts; inletRow++)
/////////         //{
/////////         //	// bay trace can be beachtype of bay or river
/////////         //	// do not calculate flooding for river type
/////////         //	/*beachType = m_pInletLUT->GetAsInt(3, inletRow);				
/////////         //	if (beachType == BchT_BAY)
/////////         //	{
/////////         //		xCoord = m_pInletLUT->GetAsDouble(1, inletRow);
/////////         //		yCoord = m_pInletLUT->GetAsDouble(2, inletRow);
/////////         //		int bayFloodedCount = 0;
/////////         //		bayFloodedCount = m_pInletLUT->GetAsInt(4, inletRow);
/////////
/////////         int colNorthing;
/////////         int colEasting;
/////////
/////////         CheckCol(m_pBayTraceLayer, colNorthing, "NORTHING", TYPE_DOUBLE, CC_MUST_EXIST);
/////////         CheckCol(m_pBayTraceLayer, colEasting, "EASTING", TYPE_DOUBLE, CC_MUST_EXIST);
/////////
/////////         // traversing along bay shoreline, determine is more than the elevation 
/////////         for (MapLayer::Iterator bayPt = m_pBayTraceLayer->Begin(); bayPt < m_pBayTraceLayer->End(); bayPt++)
/////////            {
/////////            m_pBayTraceLayer->GetData(bayPt, colEasting, xCoord);
/////////            m_pBayTraceLayer->GetData(bayPt, colNorthing, yCoord);
/////////
/////////            m_pElevationGrid->GetGridCellFromCoord(xCoord, yCoord, startRow, startCol);
/////////
/////////            //      m_pInletLUT->Set(4, inletRow, startRow);
/////////            //      m_pInletLUT->Set(5, inletRow, startCol);
/////////            //
/////////            //      /*m_pElevationGrid->SetData(5255, 4373, 3.8f);
/////////            //      m_pElevationGrid->SetData(5256, 4372, 3.8f);
/////////            //      m_pElevationGrid->SetData(5260, 4369, 3.8f);
/////////
/////////
/////////            // within grid ?
/////////            if ((startRow >= 0 && startRow < m_numRows) && (startCol >= 0 && startCol < m_numCols))
/////////               {
/////////               m_pElevationGrid->GetData(startRow, startCol, minElevation);
/////////               m_pFloodedGrid->GetData(startRow, startCol, flooded);
/////////
/////////               bool isFlooded = (flooded > 0.0f) ? true : false;
/////////
/////////               //	m_pInletLUT->Set(5, inletRow, yearlyMaxTWL);
/////////               //			m_pInletLUT->Set(6, inletRow, minElevation);
/////////
/////////               // for the cell corresponding to bay point coordinate
/////////               if (!isFlooded && (yearlyMaxTWL > duneCrest) && (yearlyMaxTWL > minElevation) && (minElevation >= 0.0))
/////////                  {
/////////                  m_pFloodedGrid->SetData(startRow, startCol, yearlyMaxTWL);
/////////                  floodedCount++;
/////////                  floodedCount += VisitNeighbors(startRow, startCol, yearlyMaxTWL); //, minElevation);
/////////                                                                                    //		m_pInletLUT->Set(4, inletRow, ++bayFloodedCount);							
/////////                  int dummybreak = 10;
/////////                  }
/////////               //	}
/////////               } // end inner bay type 
/////////            } // end bay inlet pt
/////////         } // end bay inlet flooding 
/////////      else if (beachType != BchT_UNDEFINED && beachType != BchT_RIVER)
/////////         {
/////////         m_pNewDuneLineLayer->GetData(point, m_colDuneCrest, duneCrest);
/////////         //m_pNewDuneLineLayer->GetData(point, m_colEastingCrest, xCoord);
/////////         //m_pNewDuneLineLayer->GetData(point, m_colNorthingCrest, yCoord);
/////////
/////////         // use Northing location of Dune Toe and Easting location of the Dune Crest
/////////         m_pOrigDuneLineLayer->GetPointCoords(point, xCoord, yCoord);
/////////         m_pNewDuneLineLayer->GetData(point, m_colEastingCrest, xCoord);
/////////
/////////         m_pElevationGrid->GetGridCellFromCoord(xCoord, yCoord, startRow, startCol);
/////////         m_pNewDuneLineLayer->GetData(point, m_colYrFMaxTWL, yearlyMaxTWL);
/////////         //yearlyMaxTWL -= 0.23f;
/////////
/////////         // Limit flooding to swl + setup for dune backed beaches (no swash)
/////////         float swl = 0.0;
/////////         m_pNewDuneLineLayer->GetData(point, m_colYrMaxSWL, swl);
/////////         float eta = 0.0;
/////////         m_pNewDuneLineLayer->GetData(point, m_colYrMaxSetup, eta);
/////////         float STK_IG = 0.0;
/////////
/////////         yearlyMaxTWL = swl + 1.1f*(eta + STK_IG / 2.0f);
/////////
/////////         //if (duneIndx == 3191)
/////////         //{
/////////         // within grid ?
/////////         if ((startRow >= 0 && startRow < m_numRows) && (startCol >= 0 && startCol < m_numCols))
/////////            {
/////////            m_pElevationGrid->GetData(startRow, startCol, minElevation);
/////////
/////////            m_pFloodedGrid->GetData(startRow, startCol, flooded);
/////////
/////////            bool isFlooded = (flooded > 0.0f) ? true : false;
/////////
/////////            //	if (duneIndx == 1950 || duneIndx == 1951 || duneIndx == 1952 || duneIndx == 1953)
/////////            //   {
/////////            //      yearlyMaxTWL -= 0.49f;
/////////            //   }
/////////
/////////            //if (duneIndx > 3020 && duneIndx < 3100)
/////////            //{
/////////            //	yearlyMaxTWL -= 0.49f;
/////////            //}
/////////            // for the cell corresponding to dune line coordinate
/////////            if (!isFlooded && (yearlyMaxTWL > duneCrest) && (yearlyMaxTWL > minElevation) && (minElevation >= 0))
/////////               {
/////////               m_pFloodedGrid->SetData(startRow, startCol, yearlyMaxTWL);
/////////               floodedCount++;
/////////               //	floodedCount += VisitNeighbors(startRow, startCol, yearlyMaxTWL, minElevation);
/////////               floodedCount += VisitNeighbors(startRow, startCol, yearlyMaxTWL);
/////////               }
/////////            }
/////////         //}
/////////         } // end shoreline pt
/////////      } // end all duneline pts
/////////
/////////   m_pFloodedGrid->ResetBins();
/////////   m_pFloodedGrid->AddBin(0, RGB(255, 255, 255), "Not Flooded", TYPE_FLOAT, -0.1f, 0.09f);
/////////   m_pFloodedGrid->AddBin(0, RGB(0, 0, 255), "Flooded", TYPE_FLOAT, 0.1f, 100.0f);
/////////   m_pFloodedGrid->ClassifyData();
/////////   //  m_pFloodedGrid->Show();
/////////   m_pFloodedGrid->Hide();
/////////
/////////   //clock_t start = clock();
/////////
/////////   // ***************    Lookup IDUs associated with FloodedGrid and set IDU attributes accordingly   ****************
/////////   ComputeFloodedIDUStatistics();
/////////
/////////   //for (MapLayer::Iterator point = m_pBldgLayer->Begin(); point < m_pBldgLayer->End(); point++)
/////////   //{
/////////   //   double xCoord = 0.0f;
/////////   //   double yCoord = 0.0f;
/////////
/////////   //   int flooded = 0;
/////////
/////////   //   int startRow = -1;
/////////   //   int startCol = -1;
/////////
/////////   //   m_pBldgLayer->GetPointCoords(point, xCoord, yCoord);
/////////   //   m_pFloodedGrid->GetGridCellFromCoord(xCoord, yCoord, startRow, startCol);
/////////   // 
/////////   //   // within grid ?
/////////   //   if ((startRow >= 0 && startRow < m_numRows) && (startCol >= 0 && startCol < m_numCols))
/////////   //   {
/////////   //      m_pFloodedGrid->GetData(startRow, startCol, flooded);
/////////   //      bool isFlooded = (flooded == 1) ? true : false;
/////////
/////////   //      if (isFlooded)
/////////   //      {
/////////   //         m_floodedBldgCount++;
/////////   //         // m_pBldgLayer->SetData(point, m_colFlooded, flooded);
/////////   //      }
/////////   //   }
/////////   //}
/////////
/////////   //for (int row = 0; row < m_numRows; row++)
/////////   //{
/////////   //   for (int col = 0; col < m_numCols; col++)
/////////   //   {
/////////   //      int flooded = 0;
/////////   //      int noBuildings = 0;
/////////   //      int size = m_pPolygonGridLkUp->GetPolyCntForGridPt(row, col);
/////////   //      m_pFloodedGrid->GetData(row, col, flooded);
/////////
/////////   //      //
/////////   //      bool isFlooded = (flooded == 1) ? true : false;
/////////
/////////   //      if (isFlooded)
/////////   //      {
/////////   //         // m_pBuildingGrid->GetData(row, col, buildings);
/////////   //         int tempbreak = 10;
/////////   //      }
/////////   //      int *polyIndxs = new int[size];
/////////   //      int polyCount = m_pPolygonGridLkUp->GetPolyNdxsForGridPt(row, col, polyIndxs);
/////////   //      float *polyFractionalArea = new float[polyCount];
/////////
/////////   //      m_pPolygonGridLkUp->GetPolyProportionsForGridPt(row, col, polyFractionalArea);
/////////
/////////   //      for (int idu = 0; idu < polyCount; idu++)
/////////   //      {
/////////   //         m_floodFreq.AddValue((float)isFlooded);
/////////   //         m_pIDULayer->SetData(polyIndxs[idu], m_colFlooded, (int)isFlooded);
/////////   //         //   if (pEnvContext->yearOfRun >= m_avgTime)
/////////   //         //{
/////////   //         //m_pNewDuneLineLayer->SetData(polyIndxs[idu], m_colFloodFreq, m_floodFreq.GetValue() * m_avgTime);
/////////   //         //}
/////////
/////////   //         //Number of buildings that are flooded
/////////   //         //         m_pIDULayer->SetData(polyIndxs[idu], m_colnumFbuildings, ++noBuildings);
/////////   //      }
/////////
/////////   //      delete[] polyIndxs;
/////////   //   } //end columns of grid
/////////   //} // end rows of grid
/////////
/////////
/////////   //clock_t finish = clock();
/////////   //float duration = (float)(finish - start) / CLOCKS_PER_SEC;
/////////   //
/////////   //msg.Format("CoastalHazards: Flooded grid generated in %i seconds", (int)duration);
/////////   //Report::Log(msg);
/////////   //
/////////   floodedArea = m_cellWidth * m_cellHeight * floodedCount;
/////////   return floodedArea;
/////////   return 0;
/////////   }


////////   // Recursively called method that visits a grid cells, eight neighboring grid cells and determines whether they are flooded
////////   // returns the number of flooded grid cells
////////   int CoastalHazards::VisitNeighbors(int row, int col, float twl, float &minElevation)
////////      {
////////      float noDataValue = m_pElevationGrid->GetNoDataValue();
////////      float flooded = 0.0f;
////////      int cellCount = 0;
////////      //float minElevation = 0.0f;
////////
////////      // Bathtub Flooding Model
////////      // walk in 8 directions from grid cell position, determine if TWL exceeds dune crest height and elevation 
////////      for (int direction = 0; direction < 8; direction++)
////////         {
////////         //int trow = row;
////////         //int tcol = col;
////////
////////         switch (direction)
////////            {
////////            case 0:  //NORTH
////////               row--;		//Step to the north one grid cell 
////////               break;
////////
////////            case 1:  //NORTHEAST
////////               row--;		//Step to the north one grid cell
////////               col++;		//Step to the east one grid cell
////////               break;
////////
////////            case 2:  //EAST
////////               col++;		//Step to the east one grid cell
////////               break;
////////
////////            case 3:  //SOUTHEAST
////////               row++;		//Step to the south one grid cell
////////               col++;		//Step to the east one grid cell
////////               break;
////////
////////            case 4:  //SOUTH
////////               row++;		//Step to the south one grid cell
////////               break;
////////
////////            case 5:  //SOUTHWEST
////////               row++;		//Step to the south one grid cell
////////               col--;		//Step to the west one grid cell
////////               break;
////////
////////            case 6:  //WEST
////////               col--;		//Step to the west one grid cell
////////               break;
////////
////////            case 7:  //NORTHWEST
////////               row--;		//Step to the north one grid cell
////////               col--;		//Step to the west one grid cell
////////               break;
////////            } // end switch
////////
////////              // get elevation, if value is OK and within grid, determine flooded
////////         if ((row >= 0 && row < m_numRows) && (col >= 0 && col < m_numCols))
////////            {
////////            //if ((row < 0 || row >= m_numRows) || (col < 0 || col >= m_numCols))
////////            //   return cellCount;
////////
////////            //if (minElevation == noDataValue)
////////            //   return cellCount;
////////
////////            m_pElevationGrid->GetData(row, col, minElevation);
////////            if (minElevation != noDataValue && minElevation >= 0)
////////               //if (minElevation >= 0)
////////               {
////////               m_pFloodedGrid->GetData(row, col, flooded);
////////
////////               bool isFlooded = (flooded > 0.0f) ? true : false;
////////
////////               if (!isFlooded && twl > minElevation)
////////                  {
////////                  cellCount++;
////////                  m_pFloodedGrid->SetData(row, col, twl);
////////                  cellCount += VisitNeighbors(row, col, twl, minElevation);
////////                  }
////////               } // end good value
////////            } // valid location with elevation in grid
////////         } // end direction 
////////
////////      return cellCount;
////////
////////      }
////////
////////   int CoastalHazards::VisitNeighbors(int row, int col, float twl)
////////      {
////////      float noDataValue = m_pElevationGrid->GetNoDataValue();
////////      float minElevation = 0.0f;
////////      int cellCount = 0;
////////      float flooded = 0.0f;
////////
////////      // Bathtub Flooding Model
////////      // walk in 8 directions from grid cell position, determine if TWL exceeds dune crest height and elevation 
////////      for (int direction = 0; direction < 8; direction++)
////////         {
////////         switch (direction)
////////            {
////////            case 0:  //NORTH
////////               row--;		//Step to the north one grid cell 
////////               break;
////////
////////               //case 1:  //NORTHEAST
////////               //	row--;		//Step to the north one grid cell
////////               //	col++;		//Step to the east one grid cell
////////               //	break;
////////
////////            case 2:  //EAST
////////               col++;		//Step to the east one grid cell
////////               break;
////////
////////               //case 3:  //SOUTHEAST
////////               //	row++;		//Step to the south one grid cell
////////               //	col++;		//Step to the east one grid cell
////////               //	break;
////////
////////            case 4:  //SOUTH
////////               row++;		//Step to the south one grid cell
////////               break;
////////
////////               //case 5:  //SOUTHWEST
////////               //	row++;		//Step to the south one grid cell
////////               //	col--;		//Step to the west one grid cell
////////               //	break;
////////
////////            case 6:  //WEST
////////               col--;		//Step to the west one grid cell
////////               break;
////////
////////               //case 7:  //NORTHWEST
////////               //	row--;		//Step to the north one grid cell
////////               //	col--;		//Step to the west one grid cell
////////               //	break;
////////            } // end switch
////////
////////              // get elevation, if value is OK and within grid, determine flooded
////////         if (row < 0 || row >= m_numRows || col < 0 || col >= m_numCols)
////////            return cellCount;
////////
////////         m_pElevationGrid->GetData(row, col, minElevation);
////////
////////         //if (twl < minElevation)
////////         //   return cellCount;
////////
////////         if (minElevation == noDataValue || minElevation <= 0)
////////            return cellCount;
////////
////////         m_pFloodedGrid->GetData(row, col, flooded);
////////         bool isFlooded = (flooded > 0.0f) ? true : false;
////////
////////         if (!isFlooded && twl > minElevation)
////////            {
////////            cellCount++;
////////            m_pFloodedGrid->SetData(row, col, twl);
////////            cellCount += VisitNeighbors(row, col, twl);
////////
////////            //cellCount += VisitNeighbors(row - 1, col, twl);			// NORTH				
////////            //cellCount += VisitNeighbors(row, col + 1, twl);			// EAST					
////////            //cellCount += VisitNeighbors(row + 1, col, twl);			// SOUTH				
////////            //cellCount += VisitNeighbors(row, col - 1, twl);			// WEST
////////
////////            //cellCount += VisitNeighbors(row - 1, col + 1, twl);		// NORTHEAST
////////            //cellCount += VisitNeighbors(row - 1, col - 1, twl);		// NORTHWEST
////////            //cellCount += VisitNeighbors(row + 1, col + 1, twl);		// SOUTHEAST
////////            //cellCount += VisitNeighbors(row + 1, col - 1, twl);		// SOUTHWEST
////////            } // end good value
////////         }
////////      return cellCount;
////////      }
////////
////////   int CoastalHazards::VNeighbors(int row, int col, float twl)
////////      {
////////      float noDataValue = m_pElevationGrid->GetNoDataValue();
////////      float minElevation = 0.0f;
////////      int cellCount = 0;
////////      float flooded = 0.0f;
////////
////////      // Bathtub Flooding Model
////////      // walk in 8 directions from grid cell position, determine if TWL exceeds dune crest height and elevation 
////////      for (int direction = 0; direction < 8; direction++)
////////         {
////////         switch (direction)
////////            {
////////            case 0:  //NORTH
////////               row--;		//Step to the north one grid cell 
////////               break;
////////
////////            case 1:  //NORTHEAST
////////               row--;		//Step to the north one grid cell
////////               col++;		//Step to the east one grid cell
////////               break;
////////
////////            case 2:  //EAST
////////               col++;		//Step to the east one grid cell
////////               break;
////////
////////            case 3:  //SOUTHEAST
////////               row++;		//Step to the south one grid cell
////////               col++;		//Step to the east one grid cell
////////               break;
////////
////////            case 4:  //SOUTH
////////               row++;		//Step to the south one grid cell
////////               break;
////////
////////            case 5:  //SOUTHWEST
////////               row++;		//Step to the south one grid cell
////////               col--;		//Step to the west one grid cell
////////               break;
////////
////////            case 6:  //WEST
////////               col--;		//Step to the west one grid cell
////////               break;
////////
////////            case 7:  //NORTHWEST
////////               row--;		//Step to the north one grid cell
////////               col--;		//Step to the west one grid cell
////////               break;
////////            } // end switch
////////
////////         if ((row >= 0 && row < m_numRows) && (col >= 0 && col < m_numCols))
////////            {
////////            //if ((row < 0 || row >= m_numRows) || (col < 0 || col >= m_numCols))
////////            //return cellCount;
////////            //
////////            ///if (minElevation == noDataValue)
////////            //return cellCount;
////////
////////            m_pElevationGrid->GetData(row, col, minElevation);
////////
////////            if (minElevation != noDataValue && minElevation >= 0)
////////               //if (minElevation >= 0)
////////               {
////////               m_pFloodedGrid->GetData(row, col, flooded);
////////
////////               bool isFlooded = (flooded > 0.0f) ? true : false;
////////
////////               if (!isFlooded && twl > minElevation)
////////                  {
////////                  cellCount++;
////////                  m_pFloodedGrid->SetData(row, col, twl);
////////                  cellCount += VisitNeighbors(row, col, twl, minElevation);
////////                  }
////////               } // end good value
////////            } // valid location with elevation in grid
////////         } // end direction 
////////
////////      return cellCount;
////////      }
////////




int FloodArea::UpdateWaterDepthGrids()
   {
   int floodedCount = 0;

   for (int row = 0; row < m_rows; row++)
      {
      for (int col = 0; col < m_cols; col++)
         {
         CELL_TYPE cellType = GetCellType(row, col);
         if (cellType != CT_MODEL && cellType != CT_FIXEDHEIGHTSOURCE)
            continue;

         float bedElevation = 0.0f;
         bool onElevGrid = GetCellElevation(row, col, bedElevation);
         if (!onElevGrid)
            continue;

         float waterElevation = 0.0f;
         m_pWaterSurfaceElevationGrid->GetData(row, col, waterElevation);

         float depth = waterElevation - bedElevation;

         if (depth > 0.0001f)
            {
            m_pWaterDepthGrid->SetData(row, col, depth);

            float cumDepth = 0;
            m_pCumWaterDepthGrid->GetData(row, col, cumDepth);

            if (depth > cumDepth)
               m_pCumWaterDepthGrid->SetData(row, col, depth);

            floodedCount++;
            }
         }
      } // end setting flooding grid

   return floodedCount;
   }


void FloodArea::LogGrid(MapLayer *pGrid, int row, int col, int radius )
   {
   Report::Log(pGrid->m_name);
   for (int r = row-radius; r < row+radius+1; r++)
      {
      CString msg = "";

      for (int c = col-radius; c < col+radius+1; c++)
         {
         float depth;
         pGrid->GetData(r, c, depth);

         CString _msg;
         _msg.Format(" %10.7f", depth);
         msg += _msg;
         }
      Report::Log(msg);
      }
   }


int FloodArea::GenerateCellTypeGridFromElevations()
   {
   if (m_pElevationGrid == NULL)
      return -1;

   Map *pMap = m_pElevationGrid->m_pMap;

   MapLayer *pIDULayer = pMap->GetLayer(0);

   if (m_pCellTypeGrid != NULL)
      pMap->RemoveLayer(m_pCellTypeGrid, true);
   
   int rows = m_pElevationGrid->GetRowCount();
   int cols = m_pElevationGrid->GetColCount();

   REAL cellWidth, cellHeight;
   m_pElevationGrid->GetCellSize(cellWidth, cellHeight);

   REAL xMin, xMax, yMin, yMax;
   m_pElevationGrid->GetExtents(xMin, xMax, yMin, yMax);

   float noDataValue = m_pElevationGrid->GetNoDataValue();

   m_pCellTypeGrid = pMap->AddLayer(LT_GRID);
   int gridRslt = m_pCellTypeGrid->CreateGrid(rows, cols, xMin, yMin, cellWidth, cellHeight, noDataValue, DOT_INT, false, true);

   // at this point, we have a cellType grid that matches the elevation grid.
   // set it up.
   //
   // Rules:
   //   1) if elev >= 10m - OFF GRID
   //   2) else if elev <= 0:
   //      a) if location is an idu: MODEL
   //      b) else if location is outside IDUs:
   //         1. if adjacent to an IDU: FIXED HEIGHT SOURCE
   //         2. else OFF_GRID
   //   3) else MODEL

   for ( int row=0; row < rows; row++)
      {
      if (row % 10 == 0)
         {
         int pctComplete = (100 * row) / rows;
         CString msg;
         msg.Format("Flood: Generating Cell Type Grid: %i percent complete.", pctComplete);
         Report::StatusMsg(msg);
         }

      for (int col = 0; col < cols; col++)
         {
         float elev = 0;
         m_pElevationGrid->GetData(row, col, elev);

         CELL_TYPE cellType = CT_MODEL;
         if (elev > 10)  // meters
            cellType = CT_OFFGRID;

         else //if (elev <= 0.0f)
            {
            REAL x, y;
            m_pElevationGrid->GetGridCellCenter(row, col, x, y);

            // are we "under" an IDU?
            if (pIDULayer->GetPolygonFromCoord(x, y) != NULL)
               cellType = CT_MODEL;
            else   // not under, are we adjacent to an IDU?
               {
               // are we adjacent to an IDU?
               bool isAdjacent = false;
               m_pElevationGrid->GetGridCellCenter(row + 1, col, x, y);
               if (pIDULayer->GetPolygonFromCoord(x, y) != NULL)
                  isAdjacent = true;
               else
                  {
                  m_pElevationGrid->GetGridCellCenter(row - 1, col, x, y);
                  if (pIDULayer->GetPolygonFromCoord(x, y) != NULL)
                     isAdjacent = true;
                  else
                     {
                     m_pElevationGrid->GetGridCellCenter(row, col + 1, x, y);
                     if (pIDULayer->GetPolygonFromCoord(x, y) != NULL)
                        isAdjacent = true;
                     else
                        {
                        m_pElevationGrid->GetGridCellCenter(row, col - 1, x, y);
                        if (pIDULayer->GetPolygonFromCoord(x, y) != NULL)
                           isAdjacent = true;
                        }
                     }
                  }

               if (isAdjacent)
                  cellType = CT_FIXEDHEIGHTSOURCE;
               else
                  cellType = CT_OFFGRID;
               }  // end of: outside of an IDU
            }
         //else  // 0 < elev < 10
         //   {
         //   REAL x, y;
         //   m_pElevationGrid->GetGridCellCenter(row, col, x, y);
         //
         //   // are we under an IDU?
         //   if (pIDULayer->GetPolygonFromCoord(x, y) != NULL)
         //      cellType = CT_MODEL;
         //   else
         //      {
         //      outsde an IDU
         //      }
         //   }

         m_pCellTypeGrid->SetData(row, col, (int)cellType);
         }
      }  // end of double for loop

   // write data to an output file
   CString site(m_siteName);
   site.Replace(' ', '_');
   
   CString path(PathManager::GetPath(PM_IDU_DIR));
   path += "ct_";
   path += site;
   path += ".asc";

   m_pCellTypeGrid->SaveGridFile(path);
   m_pCellTypeGrid->m_name = "ct_" + site;
   m_pCellTypeGrid->m_path = path;
   m_cellTypeGridName = site;
   return 1;
   }


FloodArea *FloodArea::LoadXml(TiXmlElement *pXmlElement, LPCTSTR filename, Map *pMap)
   {
   FloodArea *pFA = new FloodArea;

   XML_ATTR attrs[] = {
   { "siteName",              TYPE_CSTRING, &pFA->m_siteName,               true,  0 },
   { "elevGridName",          TYPE_CSTRING, &pFA->m_elevGridName,           true,  0 },
   { "method",                TYPE_INT,     &pFA->m_method,                 true,  0 },
   { "cellTypeGridName",      TYPE_CSTRING, &pFA->m_cellTypeGridName,       true,  0 },
   { "manningsGridName",      TYPE_CSTRING, &pFA->m_manningsGridName,       false, 0 },
   { "timeStep",              TYPE_FLOAT,   &pFA->m_timeStep,               true,  0 },
   { "timeDuration",          TYPE_FLOAT,   &pFA->m_timeDuration,           true,  0 },
   { "manningsN",             TYPE_FLOAT,   &pFA->m_manningsN,              false, 0 },
   { "fixedSourceHeightDelta",TYPE_FLOAT,   &pFA->m_fixedSourceHeightDelta, false, 0 },
   { "updateDisplay",         TYPE_INT,     &pFA->m_updateDisplay,          false, 0 },
   { NULL, TYPE_NULL, NULL, false, 0 } };

   bool ok = TiXmlGetAttributes(pXmlElement, attrs, filename);
   if (!ok)
      {
      CString msg;
      msg.Format(_T("FloodArea: Misformed element reading <flood_area> attributes in input file %s"), filename);
      Report::ErrorMsg(msg);
      delete pFA;
      return NULL;
      }

   TiXmlElement *pSourceXml = pXmlElement->FirstChildElement("flow_source");
   while ( pSourceXml != NULL )
      {
      FlowSource *pSource = new FlowSource;

      XML_ATTR attrs[] = {
      { "rate",          TYPE_FLOAT,   &pSource->m_rate,       true, 0 },
      { "elev",          TYPE_FLOAT,   &pSource->m_elevation,  true, 0 },
      { "x",             TYPE_FLOAT,   &pSource->m_xLocation,  true, 0 },
      { "y",             TYPE_FLOAT,   &pSource->m_yLocation,  true, 0 },
      { NULL, TYPE_NULL, NULL, false, 0 } };

      bool ok = TiXmlGetAttributes(pSourceXml, attrs, filename);
      if (!ok)
         delete pSource;
      else
         pFA->m_sourceArray.Add(pSource);
      
      pSourceXml = pSourceXml->NextSiblingElement("flow_source");
      }

   //pFA->CreateInternalGrids(pMap);
   //
   //// set any user-defined sources
   //for (int i = 0; i < pFA->m_sourceArray.GetSize(); i++)
   //   {
   //   FlowSource *pSource = pFA->m_sourceArray[i];
   //
   //   int row, col;
   //   bool ok = pFA->m_pCellTypeGrid->GetGridCellFromCoord(pSource->m_xLocation, pSource->m_xLocation, row, col); // note: coords are virtual
   //   ASSERT(ok);
   //
   //   // have row, col, set into water elevation grid
   //   pFA->m_pWaterSurfaceElevationGrid->SetData(row, col, pSource->m_elevation);
   //   pFA->m_pCellTypeGrid->SetData(row, col, (int)CT_FIXEDHEIGHTSOURCE);
   //   }

   return pFA;
   }
      



/*

Solution of the 2D Laplace equation for heat conduction in a square plate

CUDA version

written by: Abhijit Joshi (joshi1974@gmail.com)
*/
/*
#include <iostream>

// global variables

const int NX = 256;      // mesh size (number of node points along X)
const int NY = 256;      // mesh size (number of node points along Y)

const int MAX_ITER = 1000;  // number of Jacobi iterations

// device function to update the array (pGrid) T_new based on the values in array T_old
// note that all locations are updated simultaneously on the GPU 
__global__ void SolveGridGPU(float *pGrid)
   {
   // compute the "i" and "j" location of the node point
   // handled by this thread

   int col = blockIdx.x * blockDim.x + threadIdx.x;
   int row = blockIdx.y * blockDim.y + threadIdx.y;

   // get the natural index values of node (i,j) and its neighboring nodes
                               //                         N 
   int P = i + j * NX;           // node (i,j)              |
   int N = i + (j + 1)*NX;       // node (i,j+1)            |
   int S = i + (j - 1)*NX;       // node (i,j-1)     W ---- P ---- E
   int E = (i + 1) + j * NX;       // node (i+1,j)            |
   int W = (i - 1) + j * NX;       // node (i-1,j)            |
                               //                         S 

   // only update "interior" node points
   if (i > 0 && i < NX - 1 && j>0 && j < NY - 1)
      {
      T_new[P] = 0.25*(T_old[E] + T_old[W] + T_old[N] + T_old[S]);
      }
   }

// initialization

void Initialize(float *TEMPERATURE)
   {
   for (int i = 0; i < NX; i++)
      {
      for (int j = 0; j < NY; j++)
         {
         int index = i + j * NX;
         TEMPERATURE[index] = 0.0;
         }
      }

   // set left wall to 1

   for (int j = 0; j < NY; j++)
      {
      int index = j * NX;
      TEMPERATURE[index] = 1.0;
      }
   }

int main(int argc, char **argv)
   {
   float *T;          // pointer to host (CPU) memory
   float *_T1, *_T2;  // pointers to device (GPU) memory

   // allocate a "pre-computation" T array on the host
   float *T = new float[NX*NY];

   // initialize array on the host
   Initialize(T);

   // allocate storage space on the GPU
   cudaMalloc((void **)&_T1, NX*NY * sizeof(float));
   cudaMalloc((void **)&_T2, NX*NY * sizeof(float));

   // copy (initialized) host arrays to the GPU memory from CPU memory
   cudaMemcpy(_T1, T, NX*NY * sizeof(float), cudaMemcpyHostToDevice);
   cudaMemcpy(_T2, T, NX*NY * sizeof(float), cudaMemcpyHostToDevice);

   // assign a 2D distribution of CUDA "threads" within each CUDA "block"    
   int ThreadsPerBlock = 16;
   dim3 dimBlock(ThreadsPerBlock, ThreadsPerBlock);

   // calculate number of blocks along X and Y in a 2D CUDA "grid"
   dim3 dimGrid(ceil(float(NX) / float(dimBlock.x)), ceil(float(NY) / float(dimBlock.y)), 1);

   // begin Jacobi iteration
   int k = 0;
   while (k < MAX_ITER)
      {
      Laplace << <dimGrid, dimBlock >> > (_T1, _T2);   // update T1 using data stored in T2
      Laplace << <dimGrid, dimBlock >> > (_T2, _T1);   // update T2 using data stored in T1
      k += 2;
      }

   // copy final array to the CPU from the GPU 
   cudaMemcpy(T, _T2, NX*NY * sizeof(float), cudaMemcpyDeviceToHost);
   cudaThreadSynchronize();
   /////
   /////  // print the results to screen
   /////  for (int j=NY-1;j>=0;j--) {
   /////      for (int i=0;i<NX;i++) {
   /////          int index = i + j*NX;
   /////          std::cout << T[index] << " ";
   /////      }
   /////      std::cout << std::endl;
   /////  }
   /////
   // release memory on the host 
   delete T;

   // release memory on the device 
   cudaFree(_T1);
   cudaFree(_T2);

   return 0;
   }

   */