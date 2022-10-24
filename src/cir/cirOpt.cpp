/****************************************************************************
  FileName     [ cirSim.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir optimization functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-2013 LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <cassert>
#include "cirMgr.h"
#include "cirGate.h"
#include "util.h"

using namespace std;

/*******************************/
/*   Global variable and enum  */
/*******************************/

/**************************************/
/*   Static varaibles and functions   */
/**************************************/

/**************************************************/
/*   Public member functions about optimization   */
/**************************************************/
// Remove unused gates
void
CirMgr::sweep()
{
	CirGate::setGlobalRef();
	_UnusedList.clear();
	for ( unsigned i = 0; i < _DFSList.size(); ++i ) {
		_DFSList[i]->setToGlobalRef();
	}

	for ( unsigned i = 1; i < _AllList.size(); ++i ) {
		if ( !_AllList[i] ) {
			continue;
		}
		else if ( !_AllList[i]->isGlobalRef() ) {
			if ( _AllList[i]->getTypeStr() == PIGate::typeName() ) {
				_UnusedList.push_back(i);
			}
			else {
				_AllList[i]->selfIsolate();
				cout << "Sweeping: " << _AllList[i]->getTypeStr() 
					<< "(" << i << ")" << " removed..." << endl;
				_AllList[i] = 0;
			}
		}
	}

	IdList tempFl;
	tempFl.reserve( _FloatingList.size() );
	for ( unsigned i = 0; i < _FloatingList.size(); ++i ) {
		if ( _AllList[ _FloatingList[i] ] ) {
			tempFl.push_back( _FloatingList[i] );
		}
	}
	_FloatingList = tempFl;

	_aigNum = _aigInDfsNum;
	cleanDeadFECs();
}

// Recursively simplifying from POs;
// _dfsList needs to be reconstructed afterwards
void
CirMgr::optimize()
{
	for ( size_t i = 0; i < _DFSList.size(); ++i ) {
		if ( _DFSList[i]->selfOptimize( _AllList[0] ) ) {
			_AllList[ _DFSList[i]->getId() ] = 0;
			--_aigNum;
		}
	}
	cleanLists();
	dfsTraversal();
}

/***************************************************/
/*   Private member functions about optimization   */
/***************************************************/

