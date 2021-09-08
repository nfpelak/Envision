#pragma once

#include <EnvExtension.h>

#include <vdataobj.h>
#include <AttrIndex.h>

#define _EXPORT __declspec( dllexport )

struct LULCINFO {
   LPCTSTR label; 
   int minImpactValue; 
   int maxImpactValue; 
   LULCINFO(LPCTSTR _label, int _min, int _max) : label(label), minImpactValue(_min), maxImpactValue(_max) {};
   };


enum TABLE {
   NULL_TABLE=-1,
   WQ_DB_TABLE,
   WQ_M1_TABLE,
   WQ_RP_TABLE,

   WF_DB1_TABLE,
   WF_DB2_TABLE,
   WF_M1_TABLE,
   WF_M2_TABLE,
   WF_RP_TABLE,

   HAB_TERR_TABLE
   };

struct TABLECOL {
   TABLE table;
   VDataObj* pTable;
   int   col;
   LPCTSTR field;
   };


struct HAB_SCORE {
   int col;
   int valOld;
   int valNew;
   float score;
   };

enum LSGROUP { LG_NULL=-1,LG_COASTAL=0, LG_LOWLAND, LG_MOUNTAINOUS, LG_LAKE, LG_DELTA, LG_COUNT };


class _EXPORT PSWCP : public  EnvModelProcess
   {
   public:
      PSWCP();
      ~PSWCP();

      virtual bool Init(EnvContext* pEnvContext, LPCTSTR initStr);
      virtual bool InitRun(EnvContext* pEnvContext, bool useInitialSeed);
      virtual bool Run(EnvContext* pContext);

      bool LoadXml(EnvContext* pEnvContext, LPCTSTR filename);

      // idu columns
      MapLayer* m_pIDULayer;
      int m_col_IDU_AUWIndex;    // water AU index
      int m_col_IDU_WqS_rp;
      int m_col_IDU_WqP_rp;
      int m_col_IDU_WqMe_rp;
      int m_col_IDU_WqN_rp;
      int m_col_IDU_WqPa_rp;

      int m_col_IDU_AUHIndex;    // hab AU index
      int m_col_IDU_Hab_IntIndex;
      int m_col_IDU_Hab_PHS;
      int m_col_IDU_Hab_OakGrove;
      int m_col_IDU_Hab_OverallIndex;
      int m_col_IDU_LULC_A;
      int m_col_IDU_LULC_B;
      int m_col_IDU_CONSERVE;


      // AU coverage for geometry
      MapLayer* m_pAUWLayer;
      MapLayer* m_pAUHLayer;   // not currently used
      MapLayer* m_pTerrHabLayer;

      // data tables with model data
      VDataObj* m_pWqDbTable;
      VDataObj* m_pWqM1Table;
      VDataObj* m_pWqRpTable;
      VDataObj* m_pWfDb1Table;
      VDataObj* m_pWfDb2Table;
      VDataObj* m_pWfM1Table;
      VDataObj* m_pWfM2Table;
      VDataObj* m_pWfRpTable;

      VDataObj* m_pHabTerrTable;
      // 

      AttrIndex m_AUWIndex_IDU;  // for IDUs, key=AUWIndex,value=IDU rows containing key  
      AttrIndex m_AUHIndex_IDU;  // for IDUs, key=AUHIndex,value=IDU rows containing key  

      // water flow assessment 

      // habitat assessment

      // HCI assessment



      // methods
      bool InitWaterAssessments(EnvContext*);
      bool InitHabAssessments(EnvContext*);
      bool InitHCIAssessment(EnvContext*);

      bool RunWFAssessment(EnvContext*);
      bool RunWQAssessment(EnvContext*);
      bool RunHabAssessment(EnvContext*);
      bool RunHCIAssessment(EnvContext*);


      //virtual bool EndRun(EnvContext *pContext) { return true; }
      //virtual bool Setup(EnvContext *pContext, HWND hWnd) { return false; }
      //virtual bool GetConfig(std::string &configStr) { return false; }
      //virtual bool SetConfig(EnvContext*, LPCTSTR configStr) { return false; }
      //virtual int  InputVar(int id, MODEL_VAR** modelVar);
      //virtual int  OutputVar(int id, MODEL_VAR** modelVar);

   protected:
      int SolveWqM1Sed();
      int SolveWqM1Phos();
      int SolveWqM1Metals();
      int SolveWqM1Nit();
      int SolveWqM1Path();

      int SolveWfM1WatDel();
      int SolveWfM1SurfStorage();
      int SolveWfM1RechargeDischarge();

      int SolveWfM2WatDel();
      int SolveWfM2SurfStorage();
      int SolveWfM2Recharge();    // NEEDS WORK, DOCS UNCLEAR
      int SolveWfM2Discharge();
      int SolveWfM2EvapTrans();

      int LoadTable(TABLE);
      bool GetTableValue(TABLE t, LPCTSTR field, int row, int& value);
      bool GetTableValue(TABLE t, LPCTSTR field, int row, float& value);
      bool GetTableValue(TABLE t, LPCTSTR field, int row, CString& value);
      bool SetTableValue(TABLE t, LPCTSTR field, int row, int value)    { VData v(value); return SetTableValue(t, field, row, v); }
      bool SetTableValue(TABLE t, LPCTSTR field, int row, float value)  { VData v(value); return SetTableValue(t, field, row, v); }
      bool SetTableValue(TABLE t, LPCTSTR field, int row, LPCTSTR value){ VData v(value); return SetTableValue(t,field,row,v); }

      bool SetTableValue(TABLE, LPCTSTR field, int row, VData& v);

      LSGROUP GetLSGroupIndex(int row);

   };


extern "C" _EXPORT EnvExtension * Factory(EnvContext*);

