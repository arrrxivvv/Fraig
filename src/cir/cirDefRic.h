/****************************************************************************
  FileName     [ cirDef.h ]
  PackageName  [ cir ]
  Synopsis     [ Define basic data or var for cir package ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2010-2013 LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef CIR_DEF_H
#define CIR_DEF_H

#include <vector>

using namespace std;

class CirPValue;
class CirGateV;
class CirGate;
class CirPiGate;
class CirPoGate;
class CirMgr;
class SatSolver;

typedef vector<CirGate*>           GateList;
typedef vector<CirGateV>           GateVList;
typedef vector<unsigned>           IdList;
typedef CirGate**                  GateArray;
typedef CirPiGate**                PiArray;
typedef CirPoGate**                PoArray;

enum GateType
{
   UNDEF_GATE = 0,
   PI_GATE    = 1,
   PO_GATE    = 2,
   AIG_GATE   = 3,
   CONST_GATE = 4,

   TOT_GATE
};

#endif // CIR_DEF_H
