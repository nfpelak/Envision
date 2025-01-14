#pragma once

using namespace std;

#include <vector>
#include <string>
#include <Maplayer.h>
#include <QueryEngine.h>
#include <MapExprEngine.h>
#include <tixml.h>
#include "VSMB.h"
#include <afxtempl.h> 


class Farm;

enum CSKEYWORD { CS_TMIN, CS_TMEAN, CS_TMAX, CS_PRECIP, CS_HPRECIP, CS_AVC };



struct CSField
   {
   CString name;
   TYPE type;
   VData defaultValue;

   int col;

   CSField(): type(TYPE_NULL), col(-1) {}
   };


struct CSAction 
   {
   CString outcome;
   CString outcomeTarget;
   CString outcomeExpr;
   VData outcomeValue;
   MapExpr* pOutcomeExpr;   // memory managed by MapExprEngine?
   int col;
 
   CSAction() : pOutcomeExpr(NULL), col(-1) {}
 
   MapExpr* CompileOutcome(MapExprEngine* pME, LPCTSTR name, MapLayer*);

   bool TakeAction(EnvContext* pContext, int idu);
   };


struct CSWhen 
   {
   CString when;
   Query* pWhen;

   CSWhen() : pWhen(NULL) {}
   Query* CompileWhen(QueryEngine* pQE, LPCTSTR className);  // memory managed by QueryEngine?
   };


// a "Crop Event" includes When and (optional) Action functionalities.
// name: name of event
// id: event identifer specified in input file
// yrf: yield reduction factor associated with event
struct CSCropEvent : public CSWhen, public CSAction
   {
   CString name;
   int id;
   float yrf;

   CString yrfExpr;
   MapExpr* pYrfExpr;  // memory managed by ??

   CSCropEvent() : id(-1), yrf(0), pYrfExpr(NULL) {}
   
   float GetYRF(int idu) {
      if (pYrfExpr == NULL)
         return yrf;
      else 
         {
         pYrfExpr->Evaluate(idu);
         return (float) pYrfExpr->GetValue();
         }
      }
   };



struct CSTransition : public CSWhen, public CSAction
   {
   CString toStage;
   };

struct CSEvalExpr : public CSWhen, public CSAction
   {
   CString name;
   };


class CSCropStage
   {
   public:
      int m_id;   // 1-based offset in FarmModel::m_crops array
      int m_vsmbStage;
      CString m_name;

      PtrArray<CSCropEvent> m_events;
      PtrArray<CSTransition> m_transitions;
      PtrArray<CSEvalExpr> m_evalExprs;

      CSCropStage(LPCTSTR name) : m_id(-1), m_name(name) , m_vsmbStage(0){}
   };


class CSCrop
   {
   public:
      CString m_name;
      int m_id;
      CString m_code;
      bool m_isRotation;
      int m_harvestStartYr;
      int m_harvestFreq;
      FDataObj* m_rootCoefficentTable;
      float m_yrfThreshold;

      PtrArray<CSCropStage> m_cropStages;
      PtrArray<CSField> m_fields;

      ~CSCrop() { if (m_rootCoefficentTable != NULL) delete m_rootCoefficentTable;  }
      CSCrop() : m_id(-1), m_isRotation(true), m_harvestFreq(0), 
         m_harvestStartYr(0), m_rootCoefficentTable(NULL),
         m_yrfThreshold(-1.0f) {}
   };


class CSModel
   {
   public:
      // QExternals
      static int m_doy;
      static int m_year;
      static int m_idu;

      // available climate variables
      static float m_pDays;
      static float m_precip;
      static float m_hMeanPrecip;
      static float m_tMean;
      static float m_tMin;
      static float m_tMax;
      static float m_gdd0;
      //static float m_gdd5;
      static float m_gdd0Apr15;
      static float m_gdd5Apr1;
      static float m_gdd5May1;
      static float m_chuMay1;
      static float m_pET;

      // available soil variables
      static float m_pctAWC;
      static float m_pctSat;
      static float m_swc;
      static float m_swe;

      // other stuff
      static MapLayer* m_pMapLayer;

      static ClimateStation *m_pClimateStation; // refererence to FarmModel climate station
      static VSMBModel* m_pVSMB;  // memory managed elsewhere

      PtrArray<CSCrop> m_crops;
      CMap<int, int, CSCrop*, CSCrop*> m_cropLookup;

   public:
      int Init(FarmModel* pFarmModel,MapLayer *pIDULayer, QueryEngine* pQE, MapExprEngine* pME);
      int InitRun(EnvContext *);   // called at start of run
      int Run(EnvContext*, bool) { return 1; }   // called at start of year

      //int SolveWhen(TCHAR* expr);

      static float Avg(int kw, int period);
      static float AbovePeriod(int kw, int threshold);
      static float BelowPeriod(int kw, int threshold);
      static float DOYFromCHU(int idu, int chu);

      float UpdateCropStatus(EnvContext* pContext, FarmModel* pFarmModel, Farm* pFarm, 
         ClimateStation* pStation, MapLayer* pLayer, int idu, float areaHa, int doy, int year,
         int lulc, int cropStage, float priorCumYRF);

      bool LoadXml(TiXmlElement* pRoot, LPCTSTR path, MapLayer *pLayer);

      CSModel() {}  // 
   };

