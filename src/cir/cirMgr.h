/****************************************************************************
  FileName     [ cirMgr.h ]
  PackageName  [ cir ]
  Synopsis     [ Define circuit manager ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-2013 LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef CIR_MGR_H
#define CIR_MGR_H

#include <vector>
#include <string>
#include <fstream>
#include <iostream>

using namespace std;

#include "cirDef.h"

extern CirMgr *cirMgr;

// TODO: Define your own data members and member functions
class CirMgr
{
public:
   CirMgr() : _simmed( false ) {}
   ~CirMgr() {}

   // Access functions
   // return '0' if "gid" corresponds to an undefined gate.
   CirGate* getGate(unsigned gid) const {       
      if ( gid < _AllList.size() ) { return _AllList[gid]; }
	 else { return 0; }
   }

   // Member functions about circuit construction
   bool readCircuit(const string&);

   // Member functions about circuit optimization
   void sweep();
   void optimize();

   // Member functions about simulation
   void randomSim();
   void fileSim(ifstream&);
   void setSimLog(ofstream *logFile) { _simLog = logFile; }
   const IdList* getFecGrp( unsigned i ) const {
      return _fecGrps[i];
   }


   // Member functions about fraig
   void strash();
   void printFEC() const;
   void fraig();

   // Member functions about circuit reporting
   void printSummary() const;
   void printNetlist() const;
   void printPIs() const;
   void printPOs() const;
   void printFloatGates() const;
   void printFECPairs() const;
   void writeAag(ostream&) const;

private:
   void dfsTraversal();

   void gateFuneral( unsigned );
   void cleanDeadFECs();
   void cleanDeadFloating() const;
   void locateUnused() const;
   void cleanLists();

   //simulation private
   void initPIs( vector<unsigned>& );
   bool initFECs();
   void firstSim();
   bool justSim();
   bool updateFECs();
   void printSimLog( unsigned til = 8 * sizeof(unsigned) );
   unsigned maxFail();

   //void debugSim();

   //fraig private
   void fraigBFS();
   void fraigByDFS();

   void genProofModel( SatSolver& );
   bool checkEqv( SatSolver& s, unsigned, unsigned, bool isInv ) const;
   bool packInputs( const SatSolver& );
   void killFecGrp( unsigned id );
   void mergeStrashGates( CirGate* persistG, CirGate* dyingG );
   void mergeEqvGates( unsigned persist, unsigned dying );

   //fec manipulation in fraig
   void addGateToFec( unsigned f, unsigned gid );
   void addGateToLastFec( unsigned gid );
   void kickGateFromFec( unsigned f, unsigned gNum );
   bool judgeFecDeath( unsigned f );

   unsigned ptrV2Lit( PtrV<CirGate> ) const;
   PtrV<CirGate> lit2ptrV( unsigned ) const;
   //static bool ptrVIdComp( PtrV<CirGate> a, PtrV<CirGate> b ) ;

   vector<Const0Gate> _Const0s;
   vector<PIGate> _PIs;
   vector<POGate> _POs;
   vector<AigGate> _Aigs;
   vector<UndefGate> _Undefs;

   vector<CirGate*> _AllList;
   vector<CirGate*> _DFSList;
   mutable IdList _FloatingList;
   mutable IdList _UnusedList;

   vector< IdList* > _fecGrps;
   bool _simmed;

   ofstream *_simLog;
   unsigned _maxId;
   unsigned _piNum;
   unsigned _latNum;
   unsigned _poNum;
   unsigned _aigNum;
   unsigned _aigInDfsNum;
};

template <class T>
void eraseNoOrder( vector<T>& arr, unsigned id )
{
	if ( id >= arr.size() ) { return; }
	swap( arr.back(), arr[id] );
	arr.pop_back();
}


#endif // CIR_MGR_H
