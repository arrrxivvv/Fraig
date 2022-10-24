/****************************************************************************
  FileName     [ cirMgr.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir manager functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-2013 LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <iostream>
#include <iomanip>
#include <cstdio>
#include <ctype.h>
#include <cassert>
#include <cstring>
#include <algorithm>
#include "cirMgr.h"
#include "cirGate.h"
#include "util.h"

using namespace std;

// TODO: Implement memeber functions for class CirMgr

/*******************************/
/*   Global variable and enum  */
/*******************************/
CirMgr* cirMgr = 0;

enum CirParseError {
   EXTRA_SPACE,
   MISSING_SPACE,
   ILLEGAL_WSPACE,
   ILLEGAL_NUM,
   ILLEGAL_IDENTIFIER,
   ILLEGAL_SYMBOL_TYPE,
   ILLEGAL_SYMBOL_NAME,
   MISSING_NUM,
   MISSING_IDENTIFIER,
   MISSING_NEWLINE,
   MISSING_DEF,
   CANNOT_INVERTED,
   MAX_LIT_ID,
   REDEF_GATE,
   REDEF_SYMBOLIC_NAME,
   REDEF_CONST,
   NUM_TOO_SMALL,
   NUM_TOO_BIG,

   DUMMY_END
};

bool gateIdComp( const CirGate* a, const CirGate* b )
{
   return ( a->getId() < b->getId() );
}



/**************************************/
/*   Static varaibles and functions   */
/**************************************/
static unsigned lineNo = 0;  // in printint, lineNo needs to ++
static unsigned colNo  = 0;  // in printing, colNo needs to ++
static char buf[1024];
static string errMsg;
static int errInt;
static CirGate *errGate;

static bool
parseError(CirParseError err)
{
   switch (err) {
      case EXTRA_SPACE:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Extra space character is detected!!" << endl;
         break;
      case MISSING_SPACE:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Missing space character!!" << endl;
         break;
      case ILLEGAL_WSPACE: // for non-space white space character
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Illegal white space char(" << errInt
              << ") is detected!!" << endl;
         break;
      case ILLEGAL_NUM:
         cerr << "[ERROR] Line " << lineNo+1 << ": Illegal "
              << errMsg << "!!" << endl;
         break;
      case ILLEGAL_IDENTIFIER:
         cerr << "[ERROR] Line " << lineNo+1 << ": Illegal identifier \""
              << errMsg << "\"!!" << endl;
         break;
      case ILLEGAL_SYMBOL_TYPE:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Illegal symbol type (" << errMsg << ")!!" << endl;
         break;
      case ILLEGAL_SYMBOL_NAME:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Symbolic name contains un-printable char(" << errInt
              << ")!!" << endl;
         break;
      case MISSING_NUM:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Missing " << errMsg << "!!" << endl;
         break;
      case MISSING_IDENTIFIER:
         cerr << "[ERROR] Line " << lineNo+1 << ": Missing \""
              << errMsg << "\"!!" << endl;
         break;
      case MISSING_NEWLINE:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": A new line is expected here!!" << endl;
         break;
      case MISSING_DEF:
         cerr << "[ERROR] Line " << lineNo+1 << ": Missing " << errMsg
              << " definition!!" << endl;
         break;
      case CANNOT_INVERTED:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": " << errMsg << " " << errInt << "(" << errInt/2
              << ") cannot be inverted!!" << endl;
         break;
      case MAX_LIT_ID:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Literal \"" << errInt << "\" exceeds maximum valid ID!!"
              << endl;
         break;
      case REDEF_GATE:
         cerr << "[ERROR] Line " << lineNo+1 << ": Literal \"" << errInt
              << "\" is redefined, previously defined as "
              << errGate->getTypeStr() << " in line " << errGate->getLineNo()
              << "!!" << endl;
         break;
      case REDEF_SYMBOLIC_NAME:
         cerr << "[ERROR] Line " << lineNo+1 << ": Symbolic name for \""
              << errMsg << errInt << "\" is redefined!!" << endl;
         break;
      case REDEF_CONST:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Cannot redefine const (" << errInt << ")!!" << endl;
         break;
      case NUM_TOO_SMALL:
         cerr << "[ERROR] Line " << lineNo+1 << ": " << errMsg
              << " is too small (" << errInt << ")!!" << endl;
         break;
      case NUM_TOO_BIG:
         cerr << "[ERROR] Line " << lineNo+1 << ": " << errMsg
              << " is too big (" << errInt << ")!!" << endl;
         break;
      default: break;
   }
   return false;
}

/**************************************************************/
/*   class CirMgr member functions for circuit construction   */
/**************************************************************/
bool
CirMgr::readCircuit(const string& fileName)
{
   ifstream aigFile( fileName.c_str(), ios::in );

   string tempLine;
   string identifier;
   string temp;
   int rewind;   
   size_t next = 0;

   lineNo = 1;
   colNo = 1;
   unsigned lineNoRewind = 0;
   int var[5];
   int id;
   int faninId;
   bool isInv;
   bool isUndef;

   getline( aigFile, tempLine );
   ++lineNo;
   next = myStrGetTok( tempLine, identifier );

   for ( int i = 0; i < 5; ++i ) {
      next = myStrGetTok( tempLine, temp, next );
	 myStr2Int( temp, var[i] );
   }

   _maxId = var[0];
   _piNum = var[1];
   _latNum = var[2];
   _poNum = var[3];
   _aigNum = var[4];

   _Const0s.push_back( Const0Gate() );
   _PIs.reserve( _piNum );
   _POs.reserve( _poNum );
   _Aigs.reserve( _aigNum );
   _Undefs.reserve( _maxId - _aigNum - _piNum );
   _DFSList.reserve( _piNum + _poNum + _aigNum + _latNum );
   _AllList.reserve( _maxId + _poNum + 1 );
   _FloatingList.reserve( _maxId - _aigNum - _piNum );

   _AllList.push_back( &_Const0s[0] );
   for ( size_t i = 1; i < _AllList.capacity(); ++i ) {
      _AllList.push_back( 0 );
   }

   for ( unsigned i = 0; i < _piNum; ++i ) {
      getline( aigFile, tempLine );
	 myStrGetTok( tempLine, temp );
	 myStr2Int( temp, id );
	 if ( id > 0 ) {
	    id /= 2;
	    _PIs.push_back( PIGate( id, lineNo ) );
	    _AllList[id] = &( _PIs.back() );
	 }
	 ++lineNo;
   }

   rewind = aigFile.tellg();
   lineNoRewind = lineNo;

   for ( unsigned i = 0; i < _latNum; ++i ) {
      getline( aigFile, tempLine );
	 ++lineNo;
   }

   for ( unsigned i = 0; i < _poNum; ++i ) {
      getline( aigFile, tempLine );
	    id = _maxId + i + 1;
	    _POs.push_back( POGate( id, lineNo ) );
	    _AllList[id] = &( _POs.back() );
	 ++lineNo;
   }

   for ( unsigned i = 0; i < _aigNum; ++i ) {
      getline( aigFile, tempLine );
	 next = myStrGetTok( tempLine, temp );
	 myStr2Int( temp, id );

	 id /= 2;

	 _Aigs.push_back( AigGate( id, lineNo ) );
	 _AllList[id] = &( _Aigs.back() );

	 ++lineNo;
   }

   aigFile.seekg( rewind );
   lineNo = lineNoRewind;

   for ( unsigned i = 0; i < _poNum; ++i ) {
      getline( aigFile, tempLine );
	 myStrGetTok( tempLine, temp );
	 myStr2Int( temp, faninId );
	 if ( faninId >= 0 ) {
	    isInv = ( faninId % 2 != 0 );
	    faninId /= 2;
	    if ( !( _AllList[faninId] ) ) {
		  _Undefs.push_back( UndefGate( faninId ) );
		  _AllList[faninId] = &(_Undefs.back());
	    }
	    isUndef = !( _AllList[faninId] ) || 
	       ( _AllList[faninId]->getTypeStr() == 
		    UndefGate::typeName() ) ;
	    if ( isUndef ) {
	       _FloatingList.push_back( _POs[i].getId() );
	    }
	    _POs[i].addFanin( _AllList[faninId], isInv, isUndef );
	 }
	 ++lineNo;
   }

   bool floatRecorded;
   for ( unsigned i = 0; i < _aigNum; ++i ) {
      getline( aigFile, tempLine );
	 next = myStrGetTok( tempLine, temp );
	 floatRecorded = false;

	 for ( unsigned j = 0; j < 2; ++j ) {
	    next = myStrGetTok( tempLine, temp, next );
	    myStr2Int( temp, faninId );
	    isInv = ( faninId % 2 != 0 );
	    faninId /= 2;
	    if ( !_AllList[faninId] ) {
		  _Undefs.push_back( UndefGate( faninId ) );
		  _AllList[faninId] = &(_Undefs.back());
	    }
	    isUndef = !_AllList[faninId] || 
	       ( _AllList[faninId]->getTypeStr() == 
		    UndefGate::typeName() );
	    if ( isUndef && !floatRecorded ) {
	       _FloatingList.push_back( _Aigs[i].getId() );
		  floatRecorded = true;
	    }
	    _Aigs[i].addFanin( _AllList[faninId], isInv, isUndef );
	 }
	 ++lineNo;
   }

   string specifier;
   string symbol;
   while ( getline( aigFile, tempLine ) ) {
	 next = tempLine.find_first_not_of( ' ' );
	 specifier = tempLine.substr(next, 1);
	 if ( specifier == "c" ) {
	    break;
	 }

	 next = myStrGetTok( tempLine, temp, next + 1 );
	 next = tempLine.find_first_not_of( ' ', next );
	 symbol = tempLine.substr( next );
	 myStr2Int( temp, id );
	 if ( specifier == "i" ) {
	    if ( id < _PIs.size() ) {
	       _PIs[id].setName( symbol );
	    }
	 }
	 else if ( specifier == "o" ) {
	    if ( id < _POs.size() ) {
	       _POs[id].setName( symbol );
	    }
	 }
   }

   aigFile.close();

   //end parsing 

   for ( size_t i = 0; i < _AllList.size(); ++i ) {
      if ( _AllList[i] ) {
	    const vector< PtrV<CirGate> >& fanins = _AllList[i]->getFanins();
	    for ( size_t j = 0; j < fanins.size(); ++j ) {
	       ( fanins[j].ptr() )->addFanout( 
		     _AllList[i], fanins[j].isInv() );
	    }
	 }
   }

   sort( _FloatingList.begin(), _FloatingList.end() );

   for ( size_t i = 0; i < _PIs.size(); ++i ) {
      if ( (_PIs[i].getFanouts()).empty() ) {
	    _UnusedList.push_back( _PIs[i].getId() );
	 }
   }

   for ( size_t i = 0; i < _Aigs.size(); ++i ) {
      if ( ( _Aigs[i].getFanouts() ).empty() ) {
	    _UnusedList.push_back( _Aigs[i].getId() );
	 }
   }

   sort( _UnusedList.begin(), _UnusedList.end() );


   dfsTraversal();

   //initFECs();

   return true;
}

/**********************************************************/
/*   class CirMgr member functions for circuit printing   */
/**********************************************************/
/*********************
Circuit Statistics
==================
  PI          20
  PO          12
  AIG        130
------------------
  Total      162
*********************/
void
CirMgr::printSummary() const
{
   cout << endl
        << "Circuit Statistics" << endl
        << "==================" << endl
	   << "  PI" << right << setw(12) << _PIs.size() << endl
	   << "  PO" << setw(12) << _POs.size() << endl
	   << "  AIG" << setw(11) << _aigNum  << endl
	   << "------------------" << endl
	   << "  Total" << setw(9) 
	   << _poNum + _piNum + _aigNum << endl;
}

void
CirMgr::printNetlist() const
{
	cout << endl;
	for ( size_t i = 0; i < _DFSList.size(); ++i ){
		cout << "[" << i << "] " ;
		_DFSList[i]->printGate();
		cout << endl;
	}
}

void
CirMgr::printPIs() const
{
   cout << "PIs of the circuit:";
   for ( size_t i = 0; i < _PIs.size(); ++i ) {
      cout << " " << _PIs[i].getId();
   }
   cout << endl;
}

void
CirMgr::printPOs() const
{
   cout << "POs of the circuit:";
   for ( size_t i = 0; i < _POs.size(); ++i ) {
      cout << " " << _POs[i].getId();
   }
   cout << endl;
}

void
CirMgr::printFloatGates() const
{
   cleanDeadFloating();
   locateUnused();
   if ( !_FloatingList.empty() ) {
      cout << "Gates with floating fanin(s):" ;
	 for ( size_t i = 0; i < _FloatingList.size(); ++i ) {
	    cout << " " << _FloatingList[i];
	 }
	 cout << endl;
   }
   if ( !_UnusedList.empty() ) {
      cout << "Gates defined but not used  :" ;
	 for ( size_t i = 0; i < _UnusedList.size(); ++i ) {
	    cout << " " << _UnusedList[i];
	 }
	 cout << endl;
   }
}

void
CirMgr::printFECPairs() const
{
	if ( !_simmed ) {
		return;
	}
	unsigned id;
	unsigned leadSim;
	for ( size_t i = 0; i < _fecGrps.size(); ++i ) {
		cout << "[" << i << "]" ;
		leadSim = _AllList[ ( *_fecGrps[i] )[0] ]->getSimResult();
		for ( size_t j = 0; j < _fecGrps[i]->size(); ++j ){
			cout << " " ;
			id = ( *_fecGrps[i] )[j];
			if ( _AllList[ id ]->getSimResult() == ~leadSim ){
				cout << "!" ;
			}
			cout << id;
		}
		cout << endl;
	}
}

void
CirMgr::writeAag(ostream& outfile) const
{
	outfile << "aag" << " " << _maxId << " " << _piNum 
	     << " " << _latNum << " " << _poNum 
		<< " " << _aigInDfsNum << endl;
	for ( size_t i = 0; i < _PIs.size(); ++i ) {
		outfile << 2 * _PIs[i].getId() << endl;
	}

	for ( size_t i = 0; i < _POs.size(); ++i ) {
		outfile << ptrV2Lit( (_POs[i].getFanins())[0] ) << endl;
	}

	for ( size_t i = 0; i < _DFSList.size(); ++i ) {
		if ( _DFSList[i]->getTypeStr() == AigGate::typeName() ) {
			outfile << 2 * _DFSList[i]->getId() ;
			for ( size_t j = 0; j < 2; ++j ) {
				outfile << " " 
					<< ptrV2Lit( (_DFSList[i]->getFanins())[j] );
			}
			outfile << endl;
		}
	}
	
	for ( size_t i = 0; i < _PIs.size(); ++i ) {
		if ( !(_PIs[i].getName()).empty() ) {
			outfile << "i" << i << " " 
				<< _PIs[i].getName() << endl;
		}
	}
	for ( size_t i = 0; i < _POs.size(); ++i ) {
		if ( !(_POs[i].getName()).empty() ){
			outfile << "o" << i << " "
				<< _POs[i].getName() << endl;
		}
	}

	outfile << "c" << endl;
	outfile << "AAG output by Chung-Yang (Ric) Huang" << endl;
}

void
CirMgr::dfsTraversal()
{
	_DFSList.clear();
	CirGate::setGlobalRef();
	for ( size_t i = 0; i < _POs.size(); ++i ){
		_POs[i].dfsPost( _DFSList );
	}

	_aigInDfsNum = 0;
	for ( size_t i = 0; i < _DFSList.size(); ++i ) {
		if ( _DFSList[i]->getTypeStr() == AigGate::typeName() ) {
		   ++_aigInDfsNum;
		}
	}
}

void
CirMgr::gateFuneral( unsigned dead )
{
	/*IdList& grp = *_fecGrps[	_AllList[dead]->getFecGrpId() ];
	IdList::iterator it;
	for ( it = grp.begin();  it != grp.end(); ++it ) {
		if ( *it == dead ) {
			grp.erase(it);
			break;
		}
	}*/
	_AllList[dead]= 0;
	--_aigNum;
}

void
CirMgr::cleanDeadFECs()
{
	for ( int i = 0; i < _fecGrps.size(); ++i ) {
		for ( int j = 0; j < _fecGrps[i]->size(); ++j ) {
			if ( _AllList[ (*_fecGrps[i])[j] ] == 0) {
				eraseNoOrder( *_fecGrps[i], j );
				--j;
			}
			else {
				_AllList[ (*_fecGrps[i])[j] ]->setFecGrpId(i);
			}
		}
		if ( _fecGrps[i]->size() <= 1 ) {
			for ( size_t j = 0; j < _fecGrps[i]->size(); ++j ){
				_AllList[ (*_fecGrps[i])[j] ]->clearFec();
			}
			delete _fecGrps[i];
			eraseNoOrder( _fecGrps, i );
			--i;
		}
	}
}

void
CirMgr::cleanDeadFloating() const
{
	for ( int i = 0; i < _FloatingList.size(); ++i ) {
		if ( !_AllList[i] ) {
			eraseNoOrder( _FloatingList, i );
			--i;
		}
	}
	sort( _FloatingList.begin(), _FloatingList.end() );
}

void
CirMgr::locateUnused() const
{
	_UnusedList.clear();
	for ( unsigned i = 1; i < _AllList.size(); ++i ) {
		if ( !_AllList[i] ) {
			continue;
		}
		if ( _AllList[i]->getTypeStr() 
			  == POGate::typeName() ) {
			continue;
		}
		if ( ( _AllList[i]->getFanouts() ).empty() ) {
			_UnusedList.push_back( i );
		}
	}
	sort( _UnusedList.begin(), _UnusedList.end() );
}

void
CirMgr::cleanLists()
{
	cleanDeadFECs();
	;
}


unsigned
CirMgr::ptrV2Lit( PtrV<CirGate> p ) const
{
	unsigned id = 2 * ( p.ptr() )->getId();
	if ( p.isInv() ) {
		id += 1;
	}
	return id;
}

PtrV<CirGate>
CirMgr::lit2ptrV( unsigned litId ) const
{
	if ( litId / 2 >= _AllList.size() ) {
		return PtrV<CirGate>(0);
	}
	else {
		bool isInv = ( litId % 2 == 0 );
		unsigned id = litId / 2;
		CirGate* cptr = _AllList[id];
		bool isUndef = !cptr;
		return PtrV<CirGate>( cptr, isInv, isUndef );
	}
}
/*
bool
CirMgr::ptrVIdComp( PtrV<CirGate> a, PtrV<CirGate> b )
{
	return ( a.ptr()->getId() < b.ptr()->getId() );
}*/
