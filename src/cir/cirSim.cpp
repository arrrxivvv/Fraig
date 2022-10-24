/****************************************************************************
  FileName     [ cirSim.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir simulation functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-2013 LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <fstream>
#include <iostream>
#include <iomanip>
#include <cassert>
#include <climits>
#include <sstream>
#include <stdexcept>
#include "cirMgr.h"
#include "cirGate.h"
#include "myHash.h"
#include "util.h"

using namespace std;

/*******************************/
/*   Global variable and enum  */
/*******************************/

class FirstSimKey
{
public:
	FirstSimKey( unsigned s ): _simR(s) { 
		if ( _simR & _mask ) {
			_simR = ~_simR;
		}
	}

	size_t operator () () const {
		return ( _simR + ( _simR << 1 ) + ( _simR << 2 ) );
	}

	bool operator == ( const FirstSimKey& k ) const {
		return ( _simR == k._simR ); 
	}
private: 
	unsigned _simR;
	static const unsigned _mask = 1;
};

class SimKey
{
public:
	SimKey( unsigned s ): _simR(s) { }

	size_t operator () () const {
		return ( _simR + ( _simR << 1 ) + ( _simR << 2 ) );
	}

	bool operator == ( const SimKey& k ) const {
		return ( _simR == k._simR ); 
	}
private: 
	unsigned _simR;
};

class LengthException : public runtime_error
{
public:
	LengthException( const string& s, unsigned l ) 
		: runtime_error(""), _pattern(s), _piNum(l) { }
	~LengthException() throw() {}
	
	const string& getPattern() const { return _pattern; }
	unsigned getNumIn() const { return _piNum; }
private:
	const string _pattern;
	const unsigned _piNum;
};

class CharException : public runtime_error
{
public:
	CharException( const string& s, const char c ) 
		: runtime_error(""), 
		  _pattern( s ), _errChar(c) {}
	~CharException() throw() {}
	const string& getPattern() const { return _pattern; }
	char getChar( ) const { return _errChar; }
private:
	const string _pattern;
	const char _errChar;
};

/**************************************/
/*   Static varaibles and functions   */
/**************************************/

/************************************************/
/*   Public member functions about Simulation   */
/************************************************/
void
CirMgr::randomSim()
{
	unsigned max = maxFail();
	cout << "MAX_FAILS = " << max << endl;
	unsigned fail = 0;
	unsigned trial = 0;
	if ( !_simmed ) {
		for ( size_t i = 0; i < _piNum; ++i ) {
			_PIs[i].initSim( rnGen(INT_MAX) );
		}
		firstSim();
	}
	
	while ( fail < max && !(_fecGrps.empty()) ) {
		for ( size_t i = 0; i < _piNum; ++i ) {
			_PIs[i].initSim( rnGen(INT_MAX) );
		}
		if ( !justSim() ) {
			++fail;
		}
		trial += 8 * sizeof(unsigned);
		
		printFEC();
		cout << '\r' ;

		printSimLog();
	}

	cout << trial << " patterns simulated." << endl;
	_simmed = true;
}

void
CirMgr::fileSim(ifstream& patternFile)
{
	size_t totalCount = 0;
	vector<string> patterns( 8 * sizeof(unsigned) );
	size_t count;
	size_t input;

	try {
	while ( patternFile ) {
		for ( count = 0; count < patterns.size(); ++count ){
			patternFile >> patterns[count];
			if ( !patternFile ) {
				break;
			}
			if ( patterns[count].length() != _piNum ) {
				throw LengthException( 
					patterns[count], _piNum );
			}
			++totalCount;
		}
		if ( count == 0 ) { break; }

		for ( size_t i = 0; i < _piNum; ++i ) {
			input = 0;
			for ( size_t j = 0; j < count; ++j ) {
				if ( patterns[j][i] == '1' ) {
					input += 1 << j;
				}
				else if ( patterns[j][i] != '0' ) {
					throw CharException( 
						patterns[j], patterns[j][i] );
				}
			}
			_PIs[i].initSim( input );
		}
		if ( !_simmed ) {
			firstSim();
		}
		else {
			justSim();
		}

		printFEC();
		cout << '\r' ;

		printSimLog( count );
		}
	}
	catch( LengthException &lExcpt ) {
		cerr << "Error: Pattern" 
			 << "(" << lExcpt.getPattern() << ")" 
			 << " length" 
			 << "(" << lExcpt.getPattern().length() << ")"
			 << " does not match the number of inputs"
			 << "(" << lExcpt.getNumIn() << ")" 
			 << " in a circuit!!" << endl;
		totalCount = 0;
		_simmed = false;
		_fecGrps.clear();
	}
	catch( CharException &chExcept ) {
		cerr << "Error: Pattern"
			 << "(" << chExcept.getPattern() << ")"
			 << " contains a non-0/1 character"
			 << "(\'" << chExcept.getChar() << "\')."
			 << endl;
		totalCount = 0;
		_simmed = false;
		_fecGrps.clear();
	}
	patternFile.close();
	if (_simLog) {	
		_simLog->close();
	}

	cout  << totalCount << " patterns simulated." << endl;
	//_simmed = true;
}

/*************************************************/
/*   Private member functions about Simulation   */
/*************************************************/
void
CirMgr::initPIs( vector<unsigned>& inputs )
{
	assert( inputs.size() == _piNum );
	for ( size_t i = 0; i < _piNum; ++i ) {
		_PIs[i].initSim( inputs[i] );
	}
}
/*
void
CirMgr::initFECs()
{
	_fecGrps.clear();
	
	IdList* fecGrp = new IdList;
	fecGrp->reserve( _aigNum + 1 );
	fecGrp->push_back( 0 );

	CirGate::setGlobalRef();
	for ( size_t i = 0; i < _DFSList.size(); ++i ) {
		_DFSList[i]->setToGlobalRef();
	}

	for ( unsigned i = 1; i < _AllList.size(); ++i ) {
		if ( _AllList[i] != 0 &&
		     _AllList[i]->isGlobalRef() && 
		     _AllList[i]->getTypeStr() == AigGate::typeName() ) {
			fecGrp->push_back( i );
		}
	}

	_fecGrps.push_back( fecGrp );
}*/

bool
CirMgr::initFECs()
{
	//assert( _fecGrps.empty() ); //debug
	_fecGrps.clear();

	IdList* grp;
	Hash< FirstSimKey, IdList* > grpHash;
	vector< IdList* > newGrps;
	unsigned leadId;
	unsigned id;
	unsigned simR;
	size_t newSize;
	bool fecInv;
	bool distinguished = false;

	CirGate::setGlobalRef();
	for ( size_t i = 0; i < _DFSList.size(); ++i ) {
		_DFSList[i]->setToGlobalRef();
	}

	grpHash.init( _DFSList.size() / 5 );
	for ( size_t i = 0; i < _AllList.size(); ++i ) {
		if ( i != 0 && 
			 (!_AllList[i] || 
			  ! ( _AllList[i]->isGlobalRef() )  || 
			  _AllList[i]->getTypeStr() != AigGate::typeName() ) ) {
			continue;
		}
		_AllList[i]->clearFec();
		simR = _AllList[i]->getSimResult();
		if ( !grpHash.check( FirstSimKey(simR), grp ) ) {
			grp = new IdList;
			grpHash.forceInsert( simR, grp );
		}
		grp->push_back( i );
	}

	Hash< FirstSimKey, IdList* >::iterator it;

	newSize = 0;
	for ( it = grpHash.begin(); it != grpHash.end(); ++it ) {
		grp = (*it).second;
		if ( grp->size() > 1 ) {
			newGrps.push_back( grp );
			++newSize;
		}
		else {
			delete grp;
		}
	}
	if ( newSize != 1 ) {
		distinguished = true;
	}

	swap( _fecGrps, newGrps );
	for ( size_t i = 0; i < _fecGrps.size(); ++i ){
		leadId = _fecGrps[i]->front();
		for ( size_t j = 0; j < _fecGrps[i]->size(); ++j ){
			id =  (*_fecGrps[i])[j];
			fecInv = ( _AllList[id]->getSimResult() != 
					  _AllList[leadId]->getSimResult() );
			_AllList[id]->setFecGrpId( i );
			_AllList[id]->setFecInv( fecInv );
		}
	}
	return distinguished;
}

void
CirMgr::firstSim()
{
	assert(!_simmed);
	for ( size_t i = 0; i < _DFSList.size(); ++i ) {
		_DFSList[i]->simulate();
	}
	initFECs();
	_simmed = true;
}

bool
CirMgr::justSim()
{
	for ( size_t i = 0; i < _DFSList.size(); ++i ) {
		_DFSList[i]->simulate();
	}
	return updateFECs();
}

bool
CirMgr::updateFECs()
{
	IdList* grp;
	Hash< SimKey, IdList* > grpHash;
	vector< IdList* > newGrps;
	newGrps.reserve( _fecGrps.size() );
	unsigned id;
	unsigned simEqv;
	size_t newSize;
	bool distinguished = false;
	for ( size_t i = 0; i < _fecGrps.size(); ++i ) {
		grpHash.init( _fecGrps[i]->size() / 5 );
		for ( size_t j = 0; j < _fecGrps[i]->size(); ++j ) {
			id = (*(_fecGrps[i]))[j];
			_AllList[id]->clearFec();
			simEqv = _AllList[id]->getSimEqv();
			if ( !grpHash.check( SimKey(simEqv), grp ) ) {
				grp = new IdList;
				grpHash.forceInsert( simEqv, grp );
			}
			grp->push_back( id );
		}
		delete _fecGrps[i];
		_fecGrps[i] = 0;

		Hash< SimKey, IdList* >::iterator it;

		newSize = 0;
		for ( it = grpHash.begin(); it != grpHash.end(); ++it ) {
			grp = (*it).second;
			if ( grp->size() > 1 ) {
				newGrps.push_back( grp );
				++newSize;
			}
			else {
				delete grp;
			}
		}
		if ( newSize != 1 ) {
			distinguished = true;
		}
	}
	swap( _fecGrps, newGrps );
	for ( size_t i = 0; i < _fecGrps.size(); ++i ){
		for ( size_t j = 0; j < _fecGrps[i]->size(); ++j ){
			_AllList[ (*_fecGrps[i])[j] ]->
				setFecGrpId( i );
		}
	}
	return distinguished;
}

void
CirMgr::printSimLog( unsigned til )
{
	if ( !_simLog ) {
		return;
	}
	unsigned mask = 1;
	if ( til > 8 * sizeof(unsigned) ) {
		til = 8 * sizeof(unsigned);
	}
	for ( size_t i = 0; i < til; ++i ) {
		for ( size_t j = 0; j < _piNum; ++j ){
			*_simLog << ( ( mask & _PIs[j].getSimResult() ) ?
			1 : 0 ) ;
		}
			*_simLog << " " ;
		for ( size_t j = 0; j < _poNum; ++j ) {
			*_simLog << ( ( mask & _POs[j].getSimResult() ) ? 1 : 0 );
		}
		*_simLog << endl;
		mask = mask << 1;
	}
}

unsigned
CirMgr::maxFail()
{
	unsigned mask = UINT_MAX;
	size_t i;
	unsigned max = _piNum;
	for ( i = 1; i < 8*(sizeof(unsigned)); ++i ){
		mask <<= 1;
		if ( !( mask & _piNum ) ) {
			break;
		}
	}
	for ( size_t j = 0; j < ( i / 3 ); ++j ) {
		max /= 2;
	}

	if ( max < 3 ) {
		if ( _aigNum < 50 ) {
			max = 3;
		}
		else {
			max = 5;
		}
	}
	return max; 
}
/*
void
CirMgr::debugSim()
{
	static unsigned fileId = 0;
	++fileId;
	string logName = "simout";
	stringstream ss;
	ss.str("");
	ss << logName << fileId;
	logName = ss.str();
	
	size_t totalCount = 0;
	vector<string> patterns( 8 * sizeof(unsigned) );
	size_t count;
	size_t input;
	unsigned mask;
	ifstream pFile( "./tests.fraig/pattern.09" );
	ofstream lFile( logName.c_str() );

	while ( pFile ) {
		for ( count = 0; count < patterns.size(); ++count ){
			//getline( patternFile, patterns[count] );
			pFile >> patterns[count];
			if ( !pFile ) {
				break;
			}
			++totalCount;
		}
		if ( count == 0 ) { break; }

		for ( size_t i = 0; i < _piNum; ++i ) {
			input = 0;
			for ( size_t j = 0; j < count; ++j ) {
				if ( patterns[j][i] == '1' ) {
					input += 1 << j;
				}
			}
			_PIs[i].initSim( input );
		}

		for ( size_t i = 0; i < _DFSList.size(); ++i) {
			_DFSList[i]->simulate();
		}


		if ( lFile ) {
			mask = 1;
			for ( size_t i = 0; i < count; ++i ) {
				for ( size_t j = 0; j < _piNum; ++j ){
					lFile << ( ( mask & _PIs[j].getSimResult() ) ?
					1 : 0 ) ;
				}

				lFile << " " ;
				for ( size_t j = 0; j < _poNum; ++j ) {
					lFile << ( ( mask & _POs[j].getSimResult() ) ? 1 : 0 );
				}
				lFile << endl;
				mask = mask << 1;
			}
		}
	}
}*/
