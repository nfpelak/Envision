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


#include "GlobalMethods.h"
#include "Flow.h"
#include <Report.h>
#include <EnvisionAPI.h>


CArray< float, float > GlobalMethod::m_iduMuniDemandArray;
CArray< float, float > GlobalMethod::m_iduIrrRequestArray;
CArray< float, float > GlobalMethod::m_iduAnnAvgETArray;
CArray< float, float > GlobalMethod::m_iduAnnAvgMaxETArray;
CArray< float, float > GlobalMethod::m_iduYrIrrigArray;
CArray< float, float > GlobalMethod::m_iduYrMuniArray;
CArray< float, float > GlobalMethod::m_iduSeasonalAvgETArray;
CArray< float, float > GlobalMethod::m_iduSeasonalAvgMaxETArray;

CArray< float, float > GlobalMethod::m_iduAllocationArray;
CArray< float, float > GlobalMethod::m_iduIrrAllocationArray;
CArray< float, float > GlobalMethod::m_iduNonIrrAllocationArray;
CArray< int,   int   > GlobalMethod::m_iduQueryStatusArray;
CArray< int,   int   > GlobalMethod::m_hruQueryStatusArray;
CArray< HRU*,  HRU*  > GlobalMethod::m_iduHruPtrArray;

PtrArray< GlobalMethod > GlobalMethodManager::m_gmArray;

int GlobalMethod::m_nextID = 1;


//CArray< bool, bool > GlobalMethod::m_hruIsT30FilledArray;
//CArray< int, int > GlobalMethod::m_hruT30OldestIndexArray;
//FDataObj GlobalMethod::m_hruT30Array;

bool GlobalMethod::Init( FlowContext *pFlowContext )
   {
   CompileQuery( pFlowContext );

   //Run( pFlowContext );

   return true;
   }


bool GlobalMethod::Step( FlowContext *pFlowContext )
   {
   if ( m_use == false )
      return true;

   switch( m_method )
      {
      case GM_NONE:
         return true;

      case GM_EXTERNAL:
         {
         if ( m_extFn == NULL )
            return true;

         if ( ( m_timing & pFlowContext->timing ) == 0 )
            return true;

         m_extFn( pFlowContext );
         }
         return true;

      default:
         return false;
      }

   return false;
   }



bool GlobalMethod::CompileQuery( FlowContext *pFlowContext )
   {
   if ( m_query.GetLength() > 0 && m_pQuery == NULL )
      {
      EnvContext *pEnvContext = pFlowContext->pEnvContext;
      m_pQuery = pEnvContext->pQueryEngine->ParseQuery( m_query, 0, m_name );

      if ( m_pQuery == NULL )
         {
         CString msg;
         msg.Format( _T("Flow:  Error compiling global method '%s' query '%s' - this query will be ignored"), (LPCTSTR) m_name, (LPCTSTR) m_query );
         Report::ErrorMsg( msg );
         return false;
         }
      }

   return true;
   }



///////////////////////////////////////////////////////////////////////////
// GlobalMethodManager
///////////////////////////////////////////////////////////////////////////

// called by Flow during RunGlobalMethods() GMT_START_YEAR
// sets the following arrays
//   m_iduIrrRequestArray        
//   m_iduAnnAvgETArray  
//   m_iduAnnAvgMaxETArray  
//   m_iduASeasonalAvgETArray  
//   m_iduSeasonalAvgMaxETArray  
//   m_iduAllocationArray
//   m_iduIrrAllocationArray
//   m_iduNonIrrAllocationArray
//   m_iduMuniDemandArray
bool GlobalMethodManager::Init( FlowContext *pFlowContext )
   {
   pFlowContext->timing = GMT_INIT;
   FlowModel *pModel = pFlowContext->pFlowModel;
   int iduCount = pModel->m_pCatchmentLayer->GetRecordCount();

   // allocate shared IDU arrays
   GlobalMethod::m_iduMuniDemandArray.SetSize( iduCount );
   GlobalMethod::m_iduIrrRequestArray.SetSize( iduCount );
   GlobalMethod::m_iduAnnAvgETArray.SetSize( iduCount );
   GlobalMethod::m_iduAnnAvgMaxETArray.SetSize( iduCount );
	GlobalMethod::m_iduSeasonalAvgETArray.SetSize(iduCount);
	GlobalMethod::m_iduSeasonalAvgMaxETArray.SetSize(iduCount);
   GlobalMethod::m_iduAllocationArray.SetSize( iduCount );
	GlobalMethod::m_iduIrrAllocationArray.SetSize(iduCount);
	GlobalMethod::m_iduNonIrrAllocationArray.SetSize(iduCount);
   GlobalMethod::m_iduYrIrrigArray.SetSize( iduCount );
	GlobalMethod::m_iduYrMuniArray.SetSize(iduCount);

   GlobalMethod::m_iduQueryStatusArray.SetSize( iduCount );

   GlobalMethod::m_iduHruPtrArray.SetSize( iduCount );

   // initialize them to zeros

   for ( int i=0; i < iduCount; i++ )
      {
      GlobalMethod::m_iduIrrRequestArray        [ i ] = 0;
      GlobalMethod::m_iduAnnAvgETArray  [ i ] = 0;
		GlobalMethod::m_iduAnnAvgMaxETArray[i] = 0;
		GlobalMethod::m_iduSeasonalAvgETArray[i] = 0;
		GlobalMethod::m_iduSeasonalAvgMaxETArray[i] = 0;
      GlobalMethod::m_iduAllocationArray[ i ] = 0;
		GlobalMethod::m_iduIrrAllocationArray[i] = 0;
		GlobalMethod::m_iduNonIrrAllocationArray[i] = 0;
      GlobalMethod::m_iduYrIrrigArray[ i ] = 0;
		GlobalMethod::m_iduYrMuniArray[i] = 0;
      GlobalMethod::m_iduMuniDemandArray[ i ] = 0;
      GlobalMethod::m_iduQueryStatusArray[ i ] = 0;
      GlobalMethod::m_iduHruPtrArray[ i ] = NULL;
      }

   // initialize HRU lookup array
   int hruCount = pModel->GetHRUCount();
   for ( int i=0; i < hruCount; i++ )
      {
      HRU *pHRU = pModel->GetHRU( i );

      for ( int j=0; j < pHRU->m_polyIndexArray.GetSize(); j++ )
         {
         int idu = pHRU->m_polyIndexArray[ j ];
         GlobalMethod::m_iduHruPtrArray[ idu ] = pHRU;   
         }
      }

   // HRU-level arrays
   GlobalMethod::m_hruQueryStatusArray.SetSize( hruCount );
   memset( GlobalMethod::m_hruQueryStatusArray.GetData(), 0, hruCount*sizeof( int ) );
   
   // call init for each method
   for ( int m=0; m < (int) m_gmArray.GetSize(); m++ )
      {
      GlobalMethod *pMethod = m_gmArray[ m ];
      
      if ( pMethod != NULL )
         {
         clock_t start = clock();
         
         pMethod->Init( pFlowContext );
         
         clock_t stop = clock();
         float duration = (float)(stop - start) / CLOCKS_PER_SEC;   
         pMethod->m_runTime += duration;         
         }
      }

   return true;
   }


bool GlobalMethodManager::InitRun( FlowContext *pFlowContext )
   {
   pFlowContext->timing = GMT_INITRUN;

   // call initRun for each method
   for ( int m=0; m < m_gmArray.GetSize(); m++ )
      {
      GlobalMethod *pMethod = m_gmArray[ m ];

      if ( pMethod != NULL )
         {
         clock_t start = clock();

         pMethod->InitRun( pFlowContext );

         clock_t stop = clock();
         float duration = (float)(stop - start) / CLOCKS_PER_SEC;   
         pMethod->m_runTime = duration;         // restart at begining of a run
         }
      }

   return true;
   }


// called by Flow::RunGlobalMethods() during GMT_START_YEAR 
//
// tasks include:
//   1) reset any annual arrays
//   2) update queries (these change on year boundaries
//   3) reset any interal HRU accumulators
//   4) call StartYear for each method

bool GlobalMethodManager::StartYear( FlowContext *pFlowContext )
   {
   pFlowContext->timing = GMT_START_YEAR;
   FlowModel *pModel = pFlowContext->pFlowModel;

   int hruCount = pModel->GetHRUCount();
   int iduCount = pModel->m_pCatchmentLayer->GetRecordCount();
   int methodCount = (int) m_gmArray.GetSize();

   // 1) Reset annual arrays
   memset( GlobalMethod::m_iduAnnAvgETArray.GetData(), 0, iduCount*sizeof( float ) );
   memset( GlobalMethod::m_iduAnnAvgMaxETArray.GetData(), 0, iduCount*sizeof( float ) );
	memset( GlobalMethod::m_iduSeasonalAvgETArray.GetData(), 0, iduCount*sizeof( float) );
	memset( GlobalMethod::m_iduSeasonalAvgMaxETArray.GetData(), 0, iduCount*sizeof( float) );
   memset( GlobalMethod::m_iduYrIrrigArray.GetData(), 0, iduCount*sizeof( float ) );
	memset( GlobalMethod::m_iduYrMuniArray.GetData(), 0, iduCount*sizeof(float));
   

   // 2) Set method query arrays

   // set to the query status array to zero
   memset( GlobalMethod::m_hruQueryStatusArray.GetData(), 0, hruCount*sizeof( int ) );
   memset( GlobalMethod::m_iduQueryStatusArray.GetData(), 0, iduCount*sizeof( int ) );
   
   // allocate array of ints of length HRUCount that hold HRUs that pass the query
   CArray< int, int > foundArray;
   foundArray.SetSize( methodCount );
   memset( foundArray.GetData(), 0, methodCount*sizeof( int ) );
   
   for ( int h=0; h < hruCount; h++ )
      {
      HRU *pHRU = pModel->GetHRU( h );

      // for each HRU, see if any IDU's pass the this methods' query
      int iduCount = (int) pHRU->m_polyIndexArray.GetSize();
      bool passHRU = false;

      for ( int i=0; i < iduCount; i++ )
         {
         int idu = pHRU->m_polyIndexArray[ i ];
         
         // check each global method
         for ( int m=0; m < methodCount; m++ )
            {
            GlobalMethod *pMethod = m_gmArray[ m ];

            if ( pMethod->GetMethod() == GM_NONE )
               continue;  // skip if no method defined

            bool passIDU = true;

            if ( pMethod->m_pQuery != NULL ) // is there a query?
               {
               passIDU = false;
               bool ok = pMethod->m_pQuery->Run( idu, passIDU );
               }
            else  // no query, so it applies
               ;

            // update the IDU queryStatusArray
            if ( passIDU )
               { 
               int idCode = (1 << pMethod->m_id ); // Left shifting is multiplying by 2K
               GlobalMethod::m_iduQueryStatusArray[ idu ] |= idCode;
               GlobalMethod::m_hruQueryStatusArray[ h ] |= idCode;
               passHRU = true;
               foundArray[ m ]++;
               }
            }  // end of: for each global method
         }  // end of: for each idu in this HRU
      }  // for each HRU

   for ( int m=0; m < methodCount; m++ )
      {
      CString msg;
      msg.Format( _T("Flow Global Method '%s' query found  %i IDUs"), (LPCTSTR) m_gmArray[ m ]->m_name, foundArray[ m ] );
      Report::Log( msg );
      }

   // reset each HRU member vars
   for ( int h=0; h < hruCount; h++ )
      {
      HRU *pHRU = pFlowContext->pFlowModel->GetHRU( h );
      pHRU->m_currentET = 0;
      pHRU->m_currentMaxET = 0;
		pHRU->m_currentCGDD = 0;
      }
  
   // call StartYear for each method
   for ( int m=0; m < (int) m_gmArray.GetSize(); m++ )
      {
      GlobalMethod *pMethod = m_gmArray[ m ];

      if ( pMethod != NULL )
         {
         clock_t start = clock();
      
         pMethod->StartYear( pFlowContext );

         ::EnvApplyDeltaArray(pFlowContext->pEnvContext->pEnvModel);

         clock_t stop = clock();
         float duration = (float)(stop - start) / CLOCKS_PER_SEC;   
         pMethod->m_runTime += duration;
         }
      }

   return true;
   }



// called by Flow::RunGlobalMethods() during GMT_START_STEP 
//
// tasks include:
//   1) reset any timestep-based arrays

bool GlobalMethodManager::StartStep( FlowContext *pFlowContext )
   {
   pFlowContext->timing = GMT_START_STEP;

   FlowModel *pModel = pFlowContext->pFlowModel;

   //int hruCount = pModel->GetHRUCount();
   int iduCount = pModel->m_pCatchmentLayer->GetRecordCount();

   memset( GlobalMethod::m_iduMuniDemandArray.GetData(), 0, iduCount*sizeof( float ) );   // daily values for each IDU (m3/day)
   //memset( GlobalMethod::m_iduIrrRequestArray.GetData(),         0, iduCount*sizeof( float ) );   // daily values for each IDU (mm/day)
   memset( GlobalMethod::m_iduAllocationArray.GetData(), 0, iduCount*sizeof( float ) );   // m3/sec
	memset( GlobalMethod::m_iduIrrAllocationArray.GetData(), 0, iduCount*sizeof(float) );   // m3/sec
	memset( GlobalMethod::m_iduNonIrrAllocationArray.GetData(), 0, iduCount*sizeof(float) );   // m3/sec
   
   // run start step as needed
   for ( int m=0; m < (int) m_gmArray.GetSize(); m++ )
      {
      GlobalMethod *pMethod = m_gmArray[ m ];
      if ( pMethod != NULL )
         {
         clock_t start = clock();

         pMethod->StartStep( pFlowContext );

         ::EnvApplyDeltaArray(pFlowContext->pEnvContext->pEnvModel);

         clock_t stop = clock();
         float duration = (float)(stop - start) / CLOCKS_PER_SEC;   
         pMethod->m_runTime += duration;
         }
      }
   return true;
   }


bool GlobalMethodManager::Step( FlowContext *pFlowContext )
   {
   //pFlowContext->timing = GMT_RUN;  // this is set by flow
   FlowModel *pModel = pFlowContext->pFlowModel;

   //int hruCount = pModel->GetHRUCount();
   int iduCount = pModel->m_pCatchmentLayer->GetRecordCount();

   memset( GlobalMethod::m_iduMuniDemandArray.GetData(), 0, iduCount*sizeof( float ) );   // daily values for each IDU (m3/day)
  // memset( GlobalMethod::m_iduIrrRequestArray.GetData(),  0, iduCount*sizeof( float ) );   // daily values for each IDU (mm/day)
   memset( GlobalMethod::m_iduAllocationArray.GetData(), 0, iduCount*sizeof( float ) );   // m3/sec
	memset(GlobalMethod::m_iduIrrAllocationArray.GetData(), 0, iduCount*sizeof(float));   // m3/sec
	memset(GlobalMethod::m_iduNonIrrAllocationArray.GetData(), 0, iduCount*sizeof(float));   // m3/sec
   
   // run Step()as needed
   for ( int m=0; m < (int) m_gmArray.GetSize(); m++ )
      {
      GlobalMethod *pMethod = m_gmArray[ m ];
      if ( pMethod != NULL )
         {
         if ( pMethod->m_timing & pFlowContext->timing )
            {
            clock_t start = clock();

            pMethod->Step( pFlowContext );

            ::EnvApplyDeltaArray(pFlowContext->pEnvContext->pEnvModel);

            clock_t stop = clock();
            float duration = (float)(stop - start) / CLOCKS_PER_SEC;   
            pMethod->m_runTime += duration;
            }
         }
      }

   return true;
   }


bool GlobalMethodManager::EndStep( FlowContext *pFlowContext )
   {
   pFlowContext->timing = GMT_END_STEP;
   FlowModel *pModel = pFlowContext->pFlowModel;

   // run end step as needed
   for ( int m=0; m < (int) m_gmArray.GetSize(); m++ )
      {
      GlobalMethod *pMethod = m_gmArray[ m ];
      if ( pMethod != NULL )
         {
         clock_t start = clock();

         pMethod->EndStep( pFlowContext );

         ::EnvApplyDeltaArray(pFlowContext->pEnvContext->pEnvModel);

         clock_t stop = clock();
         float duration = (float)(stop - start) / CLOCKS_PER_SEC;   
         pMethod->m_runTime += duration;
         }
      }

   return true;
   }


bool GlobalMethodManager::EndYear( FlowContext *pFlowContext )
   {
   pFlowContext->timing = GMT_END_YEAR;
   FlowModel *pModel = pFlowContext->pFlowModel;

   // run end step as needed
   for ( int m=0; m < (int) m_gmArray.GetSize(); m++ )
      {
      GlobalMethod *pMethod = m_gmArray[ m ];

      if ( pMethod != NULL )
         {
         clock_t start = clock();

         pMethod->EndYear( pFlowContext );

         ::EnvApplyDeltaArray(pFlowContext->pEnvContext->pEnvModel);

         clock_t stop = clock();
         float duration = (float)(stop - start) / CLOCKS_PER_SEC;   
         pMethod->m_runTime += duration;
         }
      }

   return true;
   }


bool GlobalMethodManager::EndRun( FlowContext *pFlowContext )
   {
   pFlowContext->timing = GMT_ENDRUN;
   FlowModel *pModel = pFlowContext->pFlowModel;

   // run end step as needed
   for ( int m=0; m < (int) m_gmArray.GetSize(); m++ )
      {
      GlobalMethod *pMethod = m_gmArray[ m ];

      if ( pMethod != NULL )
         {
         clock_t start = clock();

         pMethod->EndRun( pFlowContext );

         ::EnvApplyDeltaArray(pFlowContext->pEnvContext->pEnvModel);

         clock_t stop = clock();
         float duration = (float)(stop - start) / CLOCKS_PER_SEC;   
         pMethod->m_runTime += duration;

         CString msg;
         msg.Format( _T("Flow Global Method '%s' Runtime: %.1f seconds"), (LPCTSTR) pMethod->m_name, pMethod->m_runTime );
         Report::Log( msg );
         }
      }

   return true;
   }


bool GlobalMethodManager::SetTimeStep( float timestep )
   {
   // run end step as needed
   for ( int m=0; m < (int) m_gmArray.GetSize(); m++ )
      {
      GlobalMethod *pMethod = m_gmArray[ m ];

      if ( pMethod != NULL )
         pMethod->SetTimeStep( timestep );
      }

   return true;
   }

//void FlowModel::RunGlobalMethods( void )
//   {
//   clock_t start = clock();
//
//         
//   clock_t finish = clock();
//   float duration = (float)(finish - start) / CLOCKS_PER_SEC;   
//   m_externalMethodsRunTime += (float) duration;   
//   }

