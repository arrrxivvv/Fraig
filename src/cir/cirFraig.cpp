/****************************************************************************
  FileName     [ cirFraig.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir FRAIG functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2010-2013 LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <cassert>
#include <algorithm>
#include <queue>
#include <fstream>
#include "cirMgr.h"
#include "cirGate.h"
#include "sat.h"
#include "myHash.h"
#include "util.h"

using namespace std;

/*******************************/
/*   Global variable and enum  */
/*******************************/

bool
ptrVIdComp( PtrV<CirGate> a, PtrV<CirGate> b )
{
	return ( a.ptr()->getId() < b.ptr()->getId() );
}

class AigashKey
{
public:
   AigashKey( const vector< PtrV<CirGate> >& f ): _fanins(f) {
      sort( _fanins.begin(), _fanins.end(), ptrVIdComp );
   }

   size_t operator () () const {
      size_t value = 0;
	 for ( size_t i = 0; i < _fanins.size(); ++i ){
	    value += size_t( _fanins[i].ptr() ) << i;
	 }
	 return value;
   }

   bool operator == ( const AigashKey& k ) const {
      return ( _fanins == k._fanins );
   }
private:
   vector< PtrV<CirGate> > _fanins;
};

/**************************************/
/*   Static varaibles and functions   */
/**************************************/

/*******************************************/
/*   Public member functions about fraig   */
/*******************************************/
void
CirMgr::strash()
{
   Hash<AigashKey, CirGate*> hash( _aigNum / 4 );
   CirGate* persistG;
   for ( size_t i = 0; i < _DFSList.size(); ++i ) {
      if ( _DFSList[i]->getTypeStr() == AigGate::typeName() ) {
	    AigashKey k( _DFSList[i]->getFanins() );
	    if ( hash.check( k, persistG ) ) {
	       cout << "Strashing: " << persistG->getId() 
		       << " merging " << _DFSList[i]->getId()
			  << "..." << endl;
		  mergeStrashGates( persistG, _DFSList[i] );
	    }
	    else {
	       hash.forceInsert( k, _DFSList[i] );
	    }
	 }
   }
   cleanLists();
   dfsTraversal();
}

void
CirMgr::printFEC() const
{
	cout << "Total #FEC Group = " << _fecGrps.size() ;
	cout.flush();
}

void
CirMgr::fraig()
{
	fraigByDFS();
	_simmed = false;
}

void
CirMgr::fraigByDFS()
{
	SatSolver solver;
	genProofModel( solver );

	vector<IdList*> eqvGrps;

	unsigned fecGrpId;
	unsigned curId;
	unsigned peerId;
	unsigned leadId;
	bool isInv;
	IdList* eqvGrp;
	//IdList* nonGrp;

	while ( !_fecGrps.empty() ) {
		for ( size_t d = 0; d < _DFSList.size(); ++d ) {
			curId = _DFSList[d]->getId();

			if ( !_AllList[curId]->checkFec( fecGrpId ) ) {
				continue;
			}

			eqvGrp = new IdList;
			//nonGrp = new IdList; //beta
			eqvGrps.push_back( eqvGrp );
			eqvGrp->push_back( curId );
			//_fecGrps.push_back( nonGrp ); //beta
			for ( int f = _fecGrps[fecGrpId]->size() - 1; f >= 1; --f ) {
				peerId = ( *_fecGrps[fecGrpId] )[f];
				if ( peerId == curId ) {
					swap( (*_fecGrps[fecGrpId])[f], 
						  _fecGrps[fecGrpId]->front());
					++f;
					continue;
				}
				isInv = ( _AllList[curId]->getSimResult() !=
				     _AllList[peerId]->getSimResult() );
				//kickGateFromFec( fecGrpId, f ); //beta
				if ( checkEqv( solver, curId, peerId, isInv ) ) {
					kickGateFromFec( fecGrpId, f ); //beta
					eqvGrp->push_back(peerId);
					if ( peerId == 0 ) {
						swap( eqvGrp->back(), eqvGrp->front() );
					}
					continue;
				}

				//addGateToLastFec( peerId ); //beta
				if ( packInputs( solver ) ) {
					if ( justSim() ) {
						cout << "Updating by SAT... " ;
						printFEC();
						cout << endl;
					}
					if ( _AllList[curId]->checkFec( fecGrpId ) ) {
						f = _fecGrps[fecGrpId]->size();
						continue;
					}
					else {
						break;
					}
				}
			}
			if ( _AllList[curId]->checkFec( fecGrpId ) ) {
				//killFecGrp( fecGrpId ); //beta
				kickGateFromFec( fecGrpId, 0 );
				judgeFecDeath( fecGrpId ); //beta
			}

			if ( eqvGrps.back()->size() <= 1 ) {
				delete eqvGrps.back();
				eqvGrps.pop_back();
			}
			//judgeFecDeath( _fecGrps.size() - 1 ); //beta

			if ( eqvGrps.size() > 32 ) {
				break;
			}

		}
		for ( size_t i = 0; i < eqvGrps.size(); ++i ) {
			leadId = eqvGrps[i]->front();
			for ( size_t j = 1; j < eqvGrps[i]->size(); ++j ) {
				mergeEqvGates( leadId, (*eqvGrps[i])[j] );
			}
			delete eqvGrps[i];
		}
		eqvGrps.clear();
		cleanDeadFECs();
		dfsTraversal();
		sweep();
		justSim();
	}
}

void
CirMgr::fraigBFS()
{
	SatSolver solver;
	genProofModel( solver );

	unsigned inputNum;
	vector<IdList*> eqvGrps;

	const vector< PtrV<CirGate> >* fano;
	unsigned fecGrpId;
	unsigned curId;
	unsigned peerId;
	unsigned leadId;
	bool isInv;
	IdList* eqvGrp;
	IdList* nonGrp;

	deque< unsigned > bfsQ;
	while ( !_fecGrps.empty() ) {
		
		//init bfsQ
		CirGate::setGlobalRef();
		//bfsQ.push_back( _Const0s[0].getId() );
		//_Const0s[0].setToGlobalRef();
		for ( size_t i = 0; i < _piNum; ++i ) {
			bfsQ.push_back( _PIs[i].getId() );
			_PIs[i].setToGlobalRef();
		}

		while ( !bfsQ.empty() ) {
			curId = bfsQ.front();
			bfsQ.pop_front();
			fano = &( _AllList[curId]->getFanouts() );

			for ( size_t i = 0; i < fano->size(); ++i ) {
				if ( !( (*fano)[i].ptr()->isGlobalRef() ) ){
					(*fano)[i].ptr()->setToGlobalRef();
					bfsQ.push_back( (*fano)[i].ptr()->getId() );
				}
			}

			if ( !_AllList[curId]->checkFec( fecGrpId ) ) {
				continue;
			}

			eqvGrp = new IdList;
			nonGrp = new IdList;
			eqvGrps.push_back( eqvGrp );
			eqvGrp->push_back( curId );
			_fecGrps.push_back( nonGrp );
			for ( int i = _fecGrps[fecGrpId]->size() - 1; i >= 1; --i ) {
				peerId = ( *_fecGrps[fecGrpId] )[i];
				if ( peerId == curId ) {
					swap( (*_fecGrps[fecGrpId])[i], 
						  _fecGrps[fecGrpId]->front());
					++i;
					continue;
				}
				isInv = ( _AllList[curId]->getSimResult() !=
				     _AllList[peerId]->getSimResult() );
				//_fecGrps[fecGrpId]->pop_back();
				kickGateFromFec( fecGrpId, i );
				if ( checkEqv( solver, curId, peerId, isInv ) ) {
					//_AllList[peerId]->clearFec();
					eqvGrp->push_back(peerId);
					if ( peerId == 0 ) {
						swap( eqvGrp->back(), eqvGrp->front() );
					}
					continue;
				}

				/*_fecGrps.back()->push_back( peerId );
				_AllList[peerId]->setFecGrpId( 
					_fecGrps.size() - 1 )  ;*/
				addGateToLastFec( peerId );
				if ( packInputs( solver ) ) {
					/*if ( _fecGrps.back()->size() <= 1 ) {
						killFecGrp( _fecGrps.size() - 1 );
					}*/
					judgeFecDeath( _fecGrps.size() - 1 );
					if ( justSim() ) {
						cout << "Updating by SAT... " ;
						printFEC();
						cout << endl;
					}
					if ( _AllList[curId]->checkFec( fecGrpId ) ) {
						i = _fecGrps[fecGrpId]->size();
						continue;
					}
					else {
						break;
					}
				}
			}
			if ( _AllList[curId]->checkFec( fecGrpId ) ) {
				/*eraseNoOrder( *_fecGrps[fecGrpId], 0 );
				_AllList[curId]->clearFec();

				if ( _fecGrps[fecGrpId]->size() <= 1 ) {
					killFecGrp( fecGrpId );
				}*/
				killFecGrp( fecGrpId );
			}

			if ( eqvGrps.back()->size() <= 1 ) {
				delete eqvGrps.back();
				eqvGrps.pop_back();
			}
			judgeFecDeath( _fecGrps.size() - 1 );
			/*if ( _fecGrps.back()->size() <= 1 ) {
				killFecGrp( _fecGrps.size() - 1 );
			}*/

			if ( eqvGrps.size() > 32 ) {
				break;
			}

		}
		for ( size_t i = 0; i < eqvGrps.size(); ++i ) {
			leadId = eqvGrps[i]->front();
			for ( size_t j = 1; j < eqvGrps[i]->size(); ++j ) {
				mergeEqvGates( leadId, (*eqvGrps[i])[j] );
			}
			delete eqvGrps[i];
		}
		eqvGrps.clear();
		bfsQ.clear();
		cleanDeadFECs();
		//initFECs();
		dfsTraversal();
		sweep();
		justSim();
	}
		//cout << countD << endl; //debug
}

/********************************************/
/*   Private member functions about fraig   */
/********************************************/

void
CirMgr::genProofModel( SatSolver& s ) 
{
	s.initialize();

	int v = s.newVar();
	_Const0s[0].setVar( v );
	for ( size_t i = 0; i < _piNum; ++i ) {
		int v = s.newVar();
		_PIs[i].setVar( v );
	}
	for ( size_t i = 0; i < _DFSList.size(); ++i ) {
		if ( _DFSList[i]->getTypeStr() == AigGate::typeName() ) {
			int v = s.newVar();
			_DFSList[i]->setVar( v );
		}
	}

	for ( size_t i = 0; i < _DFSList.size(); ++i ) {
		if ( _DFSList[i]->getTypeStr() == 
			 AigGate::typeName() ) {
			s.addAigCNF( _DFSList[i]->getVar(), 
			  (_DFSList[i]->getFanins())[0].ptr()->getVar(), 
			  (_DFSList[i]->getFanins())[0].isInv(), 
			  (_DFSList[i]->getFanins())[1].ptr()->getVar(), 
			  (_DFSList[i]->getFanins())[1].isInv() );
		}
	}
}

bool
CirMgr::checkEqv( SatSolver& s, unsigned a, unsigned b, bool isInv ) const
{
	s.assumeRelease();
	s.assumeProperty( _Const0s[0].getVar(), false );
	if ( a == 0 ) {
		s.assumeProperty( _AllList[b]->getVar(), !isInv );
	}
	else if ( b == 0 ) {
		s.assumeProperty( _AllList[a]->getVar(), !isInv );
	}
	else {
		int target = s.newVar();
		s.addXorCNF( target, _AllList[a]->getVar(), false, 
							 _AllList[b]->getVar(), isInv );
		s.assumeProperty( target, true );
	}
	cout << "Proving (" << a << ", ";
	if ( isInv ) {
		cout << "!" ;
	}
	cout << b << ")..." << '\r';
	cout.flush();
	return !( s.assumpSolve() );
}

bool
CirMgr::packInputs( const SatSolver& s )
{
	static unsigned bitNum = 0;
	for ( size_t i = 0; i < _PIs.size(); ++i ) {
		_PIs[i].setSimBit( bitNum, 
			s.getValue( _PIs[i].getVar() ) );
	}
	++bitNum;
	if ( bitNum >= 32 ) {
		bitNum = 0;
		return true;
	}
	return false;
}

void
CirMgr::killFecGrp( unsigned fid )
{
	for ( size_t i = 0; i < _fecGrps[fid]->size(); ++i ) {
		_AllList[ (*_fecGrps[fid])[i] ]->clearFec();
	}
	delete _fecGrps[fid];
	eraseNoOrder( _fecGrps, fid );

	if ( _fecGrps.empty() || fid >= _fecGrps.size() ) { 
		return; 
	}
	for ( size_t i = 0; i < _fecGrps[fid]->size(); ++i ) {
		_AllList[ (*_fecGrps[fid])[i] ]->setFecGrpId(fid);
	}
}

void
CirMgr::mergeStrashGates( CirGate* persistG, CirGate* dyingG )
{
	persistG->merge( dyingG );
	_AllList[ dyingG->getId() ] = 0;
	--_aigNum;
}

void
CirMgr::mergeEqvGates( unsigned persist, unsigned dying )
{
	bool isInv = 
		( _AllList[persist]->getSimResult() != 
		  _AllList[dying]->getSimResult() );
	_AllList[dying]->replaceWithGate( _AllList[persist], isInv );
	_AllList[dying] = 0;
	--_aigNum;
}


void
CirMgr::addGateToFec( unsigned f, unsigned g )
{
	assert( f < _fecGrps.size() );
	_fecGrps[f]->push_back( g );
	_AllList[ g ]->setFecGrpId( f );
}

void
CirMgr::addGateToLastFec( unsigned gid )
{
	assert( !_fecGrps.empty() );
	addGateToFec( _fecGrps.size() - 1, gid );
}

void
CirMgr::kickGateFromFec( unsigned f, unsigned num )
{
	assert( f < _fecGrps.size() );
	assert( num < _fecGrps[f]->size() );
	_AllList[ (*_fecGrps[f])[num] ]->clearFec();
	eraseNoOrder( *_fecGrps[f], num );
}

bool
CirMgr::judgeFecDeath( unsigned f )
{
	if ( _fecGrps.empty() ) {
		return false;
	}
	if ( f >= _fecGrps.size() || f < 0 ) {
		return false;
	}
	if ( _fecGrps[f]->size() <= 1 ) {
		killFecGrp( f );
		return true;
	}
	return false;
}
