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
#pragma once

#include <envcontext.h>
#include <MAP.h>
#include <Vdataobj.h>
#include <PtrArray.h>
#include <misc.h>
#include <FDATAOBJ.H>
#include <tixml.h>


//#include <randgen\Randunif.hpp>

//#include <QueryEngine.h>
//#include <Query.h>
//#include <MapExprEngine.h>

// ---------------------------------------------------------------------------
// FloodArea defines and manages a flood simulation for a given area. 
//  The area to be simulated is defined by an integer grid MapLayer with CELL_TYPE's
//  as the grid values.  Cell type indicates how specific grid cells are managed
//  by the model treats, and allow for a grid cell to be one of:
//    1) a hydraulic cell included in the flood simulation
//    2) a flux source/sink,
//    3) a hard and soft boundary
//    4) an off-grid area (excluded from the model)
//
//  required grids
//    MapLayer *m_pCellTypeGrid;    // see cell types - defines simulation extent
//                                  // Note: this can be autogenerated based on IDUs, elevation grid
//    MapLayer *m_pElevationGrid;   // DEM elevations, cell size/extents should match CellType grid
//    
//  optional grids
//    MapLayer *m_pManningsGrid;    // mask + coefficient - input
//    
//  internally generated grids (same dimensions as CellType grid)
//    MapLayer *m_pWaterSurfaceElevationGrid;  //(m) water surface elevations (updated by model)
//    MapLayer *m_pDischargeGrid;      // (m/sec) net cell discharge (change in height/time) (updatd by the model)
//    MapLayer *m_pWaterDepthGrid;     // (m) output only
//    MapLayer *m_pCumWaterDepthGrid;  //(m) output only
//
//  Note on units:
//     elevations: m above mean sea level
//     time:  preferably seconds, but not an absolute requirement
//     manningsN is in SI units.  To convert English units to SI, divide by 1.486?

//
// ---------------------------------------------------------------------------


enum CELL_TYPE { 
   CT_MODEL=0,             // included in the model 
   CT_FIXEDHEIGHTSOURCE=1, // flux source at a fixed height
   CT_HARDBOUNDARY=2,      // no-flow boundary
   CT_SOFTBOUNDARY=3,      // allows flow to pass
   CT_OFFGRID=4            // not modeled
   };

enum {  // model methods
   BATES_MANNING=1, 
   BATES_2005=2, 
   BATHTUB=4 
   };  


class FlowSource
   {
   public:
      float m_rate;   // currently not used
      float m_elevation;
      REAL m_xLocation;
      REAL m_yLocation;

      FlowSource() : m_rate(0), m_elevation(0), m_xLocation(0), m_yLocation(0) {}
   };


class FloodArea
   {
   public:
      CString m_elevGridName;
      CString m_cellTypeGridName;
      CString m_manningsGridName;
      CString m_siteName;
      float m_timeStep;        // seconds  
      float m_timeDuration;    // seconds
      int   m_method;
      float m_fixedSourceHeightDelta;
      int   m_updateDisplay;

      
      // Bates model
      float m_manningsN;      // m^(1/3)/sec
      
      // bathtub model
      int m_stepLimit;
      
      // general
      int m_rows;       // for cellType grid
      int m_cols;
      int m_rowOffset;    // subtract from elev row to get cellType row
      int m_colOffset;    // subtract from elev col to get cellType col

      // required grids
      MapLayer *m_pElevationGrid;   // input
      MapLayer *m_pCellTypeGrid;    // see cell types

      // optional grids
      MapLayer *m_pManningsGrid;    // mask + coefficient - input
      
      // internally generated grids (same dimensions as CellType grid)
      MapLayer *m_pWaterSurfaceElevationGrid;  // updated by model
      MapLayer *m_pDischargeGrid;      // calculated by the model
      MapLayer *m_pWaterDepthGrid;     // output only
      MapLayer *m_pCumWaterDepthGrid;  // output only

      // externally defined flow sources
      PtrArray<FlowSource> m_sourceArray;
      
      // output data
      float m_floodedArea;

      FloodArea() :
         m_timeStep(0.01f),
         m_timeDuration(6),
         m_method(BATES_2005),
         m_fixedSourceHeightDelta(0.0f),
         m_updateDisplay(0),
         m_stepLimit(100),
         m_manningsN(0.025f),
         m_rows(-1),
         m_cols(-1),
         m_rowOffset(0),
         m_colOffset(0),
         m_pCellTypeGrid(NULL),    // see cell types
         m_pManningsGrid(NULL),    // mask + coefficient - input
         m_pElevationGrid(NULL),   // input
         m_pWaterSurfaceElevationGrid(NULL),  // updated by model
         m_pDischargeGrid(NULL),
         m_pWaterDepthGrid(NULL),
         m_floodedArea(0)
         {}

      //bool Init(EnvContext *pContext);
      bool InitRun(EnvContext *pContext);
      bool Run(EnvContext *pContext);
   
      bool StartYear(EnvContext *pContext);
      bool CalculateQ(int row, int col, double &flux, double &dtMax);
      void GetElevRowColFromCellTypeGrid(int &row, int &col){row += m_rowOffset; col += m_colOffset;}
      void GetCellTypeRowColFromElevGrid(int &row, int &col){row -= m_rowOffset; col -= m_colOffset;}
      bool GetCellElevation(int row, int col, float &elev);
      CELL_TYPE GetCellType(int row, int col) {
         // check the mask - is this a cell in the model?
         if (row < 0 || row >= m_rows)
            return CT_OFFGRID;
         if (col < 0 || col >= m_cols)
            return CT_OFFGRID;

         int cellType = 0;
         m_pCellTypeGrid->GetData(row, col, cellType);

         if (cellType < 0)
            return CT_OFFGRID;

         return (CELL_TYPE)cellType;
         }
      int UpdateWaterDepthGrids();
      int CreateInternalGrids( Map* );
      void LogGrid(MapLayer *pGrid, int row, int col, int radius);
      static FloodArea *LoadXml(TiXmlElement *pNode, LPCTSTR filename, Map*);

      // Runs the Bates flooding model returning the number of grid cells flooded and the area flooded
      double GenerateFloodMap( void );

   protected:
      int RunBatesModel();
      int RunBathtubModel();
      int PropagateBathtub(int row, int col, float elev, int steps );

      int GenerateCellTypeGridFromElevations();
   };

inline
bool FloodArea::GetCellElevation(int row, int col, float &elev)
   {
   elev = 0.0f;

   int elevRow = row;
   int elevCol = col;
   GetElevRowColFromCellTypeGrid(elevRow, elevCol);

   if (elevRow < 0 || elevRow >= m_pElevationGrid->GetRowCount())
      return false;
   if (elevCol < 0 || elevCol >= m_pElevationGrid->GetColCount())
      return false;

   m_pElevationGrid->GetData(elevRow, elevCol, elev );
   return true;
   }

