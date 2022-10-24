/****************************************************************************
  FileName     [ cirGate.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define class CirAigGate member functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-2013 LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <iostream>
#include <iomanip>
#include <sstream>
#include <stdarg.h>
#include <cassert>
#include "cirGate.h"
#include "cirMgr.h"
#include "util.h"

using namespace std;

extern CirMgr *cirMgr;

unsigned CirGate::_globalRef = 0;

const string Const0Gate::_typeStr = "CONST";
const string PIGate::_typeStr = "PI";
const string POGate::_typeStr = "PO";
const string AigGate::_typeStr = "AIG";
const string UndefGate::_typeStr = "UNDEF";

template<class G>
bool locatePtr( const vector< PtrV<G> >& arr, G* const g, unsigned & pos, unsigned start = 0 )
{
	for ( unsigned i = start; i < arr.size(); ++i ) {
		if ( arr[i].ptr() == g ) {
			pos = i;
			return true;
		}
	}
	return false;
}

/**************************************/
/*   class CirGate member functions   */
/**************************************/
void 
CirGate::dfsPost( vector<CirGate*>& dfsList )
{
	for ( size_t i = 0; i < _fanins.size(); ++i ){
		if ( !( _fanins[i].ptr() )->isGlobalRef() ){
			( _fanins[i].ptr() )->setToGlobalRef();
			( _fanins[i].ptr() )->dfsPost( dfsList );
		}
	}
	dfsList.push_back( this );
}

void 
CirGate::printGate() const
{
	cout << left << setw(3) << getTypeStr() << " "
		<< _id ;
	for ( size_t i = 0; i < _fanins.size(); ++i ){
		cout << " ";
		if ( _fanins[i].isUndef() ) { cout << "*"; }
		if ( _fanins[i].isInv() ) { cout << "!"; }
		cout << ( _fanins[i].ptr() )->_id;
	}

	if ( !_name.empty() ){
		cout << " (" << _name << ")" ;
	}
}

void
CirGate::reportGate() const
{
	unsigned mask;
	unsigned tempS = _simResult;;
	//const IdList* grp;
	ostringstream ss;
	string temp;
	for ( int i = 0; i < 50; ++i ) {
		cout << "=" ;
	}
	cout << endl;

	ss << "= " << getTypeStr() << "(" << _id 
	   << ")" ;
	if ( !_name.empty() ) {
		ss << "\"" << _name << "\"";
	}
	ss << ", " << "line " << _lineNo ;
	temp = ss.str();
	cout << temp; 
	cout << right << setw( 50 - temp.size() ) << "=" << endl;

	ss.str("");
	ss << "= FECs:" ;
	if ( _hasFec ) {
		const IdList* grp = cirMgr->getFecGrp( _fecId );
		for ( size_t i = 0; i < grp->size(); ++i ) {
			if ( (*grp)[i] == _id ) {
				continue;
			}
			ss << " " ;
			if ( cirMgr->getGate( (*grp)[i] )->getSimResult()
			  	 == ~_simResult ) {
				ss << "!" ;
			}
			ss << (*grp)[i];
		}
	}
	temp = ss.str();
	cout << temp;
	for ( size_t i = temp.size() + 1; i < 49; ++i ) {
		cout << ' ';
	}
	cout << " =" << endl;

	ss.str("");
	mask = 1 << ( 8 * sizeof(unsigned) - 1 );
	ss << "= Value:" << ' ';
	ss << ( (tempS & mask)? 1 : 0 );
	for ( size_t i = 1; i < 8 * sizeof(unsigned); ++i ){
		tempS = tempS << 1;
		if ( i % 4 == 0 ) {
			ss << '_';
		}
		ss << ( (tempS & mask)? 1 : 0 );
	}
	temp = ss.str();
	cout << temp;
	cout << right << setw( 50 - temp.size() ) << "=" << endl;


	for ( int i = 0; i < 50; ++i ){
		cout << "=" ;
	}
	cout << endl;
}

void
CirGate::reportFanin(int level) const
{
   assert (level >= 0);
   setGlobalRef();
   printFanInOut( true, level );
}

void
CirGate::reportFanout(int level) const
{
   assert (level >= 0);
   setGlobalRef();
   printFanInOut( false, level );
}

void
CirGate::addFanin( CirGate* gate, bool isInv, bool isUndef )
{
	_fanins.push_back( PtrV<CirGate>( gate, isInv, isUndef ) );
}

void
CirGate::addFanout( CirGate* gate, bool isInv, bool isUndef )
{
	_fanouts.push_back( PtrV<CirGate>( gate, isInv, isUndef ) );
}

void
CirGate::eraseFanin( CirGate* g )
{
	for ( size_t i = 0; i < _fanins.size(); ++i ) {
		if ( _fanins[i].ptr() == g ) {
			_fanins.erase( _fanins.begin() + i );
			break;
		}
	}
}

void
CirGate::eraseFanin( PtrV<CirGate> gV )
{
	for ( size_t i = 0; i < _fanins.size(); ++i ) {
		if ( _fanins[i] == gV ) {
			_fanins.erase( _fanins.begin() + i );
			break;
		}
	}
}

void
CirGate::eraseFanout( CirGate* g )
{
	for ( size_t i = 0; i < _fanouts.size(); ++i ){
		if ( _fanouts[i].ptr() == g ) {
			_fanouts.erase( _fanouts.begin() + i );
			break;
		}
	}
}

void
CirGate::eraseFanout( PtrV<CirGate> gV )
{
	for ( size_t i = 0; i < _fanouts.size(); ++i ){
		if ( _fanouts[i] == gV ) {
			_fanouts.erase( _fanouts.begin() + i );
			break;
		}
	}
}

/*void
CirGate::printFanin( int level, int indent, bool isInv ) const
{
}*/
void
CirGate::selfIsolate()
{
	for ( size_t i = 0; i < _fanins.size(); ++i ) {
		( _fanins[i].ptr() )->eraseFanout( this );
	}
	/*for ( size_t i = 0; i < _fanouts.size(); ++i ) {
		( _fanouts[i].ptr() )->eraseFanin( this );
	}*/
}

void
CirGate::merge( CirGate* g )
{
   for ( size_t i = 0; i < g->_fanins.size(); ++i ) {
      ( g->_fanins[i] ).ptr()->eraseFanout( 
	       PtrV<CirGate>( g, g->_fanins[i].isInv() ) );
   }
   _fanouts.reserve( _fanouts.size() + g->_fanouts.size() );
   unsigned pos;
   bool isInv;
   for ( size_t i = 0; i < g->_fanouts.size(); ++i ) {
      _fanouts.push_back( g->_fanouts[i] );
	 isInv = g->_fanouts[i].isInv();
	 locatePtr( ( g->_fanouts[i] ).ptr()->_fanins, g, pos );
	 ( g->_fanouts[i] ).ptr()->_fanins[pos] = 
	    PtrV<CirGate>( this, isInv );
   }
}

void
CirGate::printFanInOut( bool isFanin, int level, int indent, bool isInv ) const
{
	const vector< PtrV<CirGate> >& child = isFanin? _fanins: _fanouts;
	printSpaces( indent );
	if ( isInv ) { cout << "!" ; }
	cout << getTypeStr() << " " << _id;

	if ( child.empty() || level == 0 ) { 
		cout << endl;
	}
	else if ( isGlobalRef() ) { 
		cout << " (*)" 
			<< endl; 
	}
	else {
		cout << endl;
		for ( size_t i = 0; i < child.size(); ++i ) {
			( child[i].ptr() )->printFanInOut( isFanin, level - 1, indent + 2, child[i].isInv() );
		}
		setToGlobalRef();
	}
}

void
CirGate::printSpaces( int indent ) const
{
	for ( int i = 0; i < indent; ++i ){
		cout << " " ;
	}
}

void
CirGate::replaceWithFaninNo( unsigned n )
{
	unsigned start;
	unsigned pos;
	bool isInv;
	CirGate* inPtr = _fanins[n].ptr();
	for ( size_t i = 0; i < _fanins.size(); ++i ) {
		( _fanins[i].ptr() )->eraseFanout( this );
	}

	( inPtr->_fanouts ).reserve( (inPtr->_fanouts).size() + _fanouts.size() );
	//( inPtr->_fanouts ).insert( (inPtr->_fanouts).end(), _fanouts.begin(), _fanouts.end() );
	for ( unsigned i = 0; i < _fanouts.size(); ++i ){
		start = 0;
		while ( locatePtr( ( _fanouts[i].ptr() )->_fanins, this, pos, start ) ) {
			isInv = ( ( (_fanouts[i].ptr())->_fanins[pos] ).isInv() != 
				     _fanins[n].isInv() );
			( ( _fanouts[i].ptr() )->_fanins )[pos] = 
				PtrV<CirGate>( _fanins[n].ptr(), isInv );
			inPtr->_fanouts.push_back( 
				PtrV<CirGate>( _fanouts[i].ptr(), isInv ) );
			start = pos + 1;
		}
	}
	cout << "Simplifying: " << inPtr->getId() << " merging " ;
	if ( _fanins[n].isInv() ) {
		cout << "!";
	}
	cout << _id << "..." << endl;
}

void
CirGate::replaceWithGate( CirGate* g, bool gInv )
{
	for ( size_t i = 0; i < _fanins.size(); ++i ) {
		( _fanins[i].ptr() )->eraseFanout( this );
	}
	g->_fanouts.reserve( g->_fanouts.size() + _fanouts.size() );
	unsigned start;
	unsigned pos;
	bool isInv;
	for ( size_t i = 0; i < _fanouts.size(); ++i ) {
		start = 0;
		while ( locatePtr( (_fanouts[i].ptr())->_fanins, this, pos, start ) ) {
			isInv = ( _fanouts[i].ptr() )->_fanins[pos].isInv() != gInv;
			( ( _fanouts[i].ptr() )->_fanins )[pos] = 
				PtrV<CirGate>( g, isInv );
			g->_fanouts.push_back( 
				PtrV<CirGate>( _fanouts[i].ptr(), isInv ) );
			start = pos + 1;
		}
	}
	cout << "Simplifying: " << g->getId() << " merging ";
	if ( gInv ) { cout << '!'; }
	cout << _id << "..." << endl;
}

void
Const0Gate::printGate() const
{
	cout << _typeStr << 0 ;
}

void
POGate::simulate()
{
	_simResult = ( _fanins[0].ptr() )->getSimResult();
	if ( _fanins[0].isInv() ) {
		_simResult = ~_simResult;
	}
}

void
PIGate::setSimBit( unsigned b, unsigned i )
{
	unsigned mask = 1 << b;
	if ( i == 0 ) {
		_simResult &= ~mask;
	}
	else if ( i == 1 ) {
		_simResult |= mask;
	}
}

bool
AigGate::selfOptimize( CirGate* the0 )
{
	if ( _fanins[0] == _fanins[1] ) {
		replaceWithFaninNo( 0 );
		return true;
	}
	if ( _fanins[0] == _fanins[1].inv() ) {
		replaceWithGate( the0 );
		return true;
	}
	unsigned pos;
	if ( locatePtr( _fanins, the0, pos ) ) {
		if ( (_fanins[pos]).isInv() ) {
			replaceWithFaninNo( 1-pos );
		}
		else {
			replaceWithGate( the0 );
		}
		return true;
	}
	return false;
}

void
AigGate::simulate()
{
	unsigned s[2];
	for ( size_t i = 0; i < 2; ++i ) {
		s[i] = ( _fanins[i].ptr() )->getSimResult();
		if ( _fanins[i].isInv() ) {
			s[i] = ~s[i];
		}
	}
	_simResult = s[0] & s[1];
}

void
UndefGate::dfsPost( vector<CirGate*>& dfsList ) {}

void
UndefGate::printFanInOut( bool isFanin, int level, int indent, bool isInv )
{
	printSpaces( indent );
	if ( isInv ) { cout << "!"; }
	cout << getTypeStr() << " " << _id;

	if ( isGlobalRef() ) { cout << "(*)"; }
	cout << endl;
}
