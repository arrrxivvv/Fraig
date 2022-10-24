/****************************************************************************
  File_Name     [ cirGate.h ]
  Package_Name  [ cir ]
  Synopsis     [ Define basic gate data structures ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-2013 LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef CIR_GATE_H
#define CIR_GATE_H

#include <string>
#include <vector>
#include <iostream>
#include "cirDef.h"

using namespace std;

class CirGate;
template <class Gate>
class PtrV;

//------------------------------------------------------------------------
//   Define classes
//------------------------------------------------------------------------
class CirGate
{
public:
   CirGate( unsigned id, unsigned l, unsigned c = 0, string n = "" ) : 
      _lineNo(l), _colNo(c), _id(id), _name(n), _ref(0), _simResult(0), _fecId(0), _fecInv(0), _hasFec(0) {}
   virtual ~CirGate() {}

   // Basic access methods
   virtual string getTypeStr() const = 0;
   string getName() const { return _name; }
   void setName( const string& s ) { _name = s; }
   unsigned getLineNo() const { return _lineNo; }
   unsigned getId() const { return _id; }

   virtual void dfsPost( vector<CirGate*>& ) ;

   // Printing functions
   virtual void printGate() const;
   void reportGate() const;
   void reportFanin(int level) const;
   void reportFanout(int level) const;

   //global ref
   bool isGlobalRef() const { return (_ref == _globalRef); }
   void setToGlobalRef() const { _ref = _globalRef; }
   static void setGlobalRef() { ++_globalRef; }

   //Fanins, fanouts
   void addFanin( CirGate*, bool isInv, bool isUndef = false );
   void addFanout( CirGate*, bool isInv, bool isUndef = false );
   void eraseFanin( CirGate* );
   void eraseFanin(  PtrV<CirGate>);
   void eraseFanout( CirGate* );
   void eraseFanout( PtrV<CirGate> );

   const vector< PtrV<CirGate> >& getFanins() const { return _fanins; }
   const vector< PtrV<CirGate> >& getFanouts() const { return _fanouts; }

   //optimization
   void selfIsolate();
   virtual bool selfOptimize( CirGate* the0 ) { return false; }
   virtual void merge( CirGate* g );
   void replaceWithGate( CirGate*, bool = false );

   //simulation
   virtual void simulate() = 0;
   unsigned getSimResult() const { return _simResult; }
   unsigned getSimEqv() const {
      if ( _fecInv ) {
	     return ~_simResult;
	  }
	  else {
	     return _simResult;
	  }
   }
   void setFecInv( bool fI ) { _fecInv = fI; }
   void setFecGrpId( unsigned f ) {
      _fecId = f;
	  _hasFec = 1;
   }
   void clearFec() {
      _hasFec = 0;
   }
   bool hasFec() { return bool( _hasFec ); }
   bool checkFec( unsigned& fi ) {
      fi = _fecId;
	  return _hasFec;
   }

   //fraig
   void setVar( int si ) { _satVar = si; }
   int getVar() const { return _satVar; }

private:

protected:
   //void printFanin( int level, int indent = 0, bool isInv = false ) const;
   //void printFanout( int level, int indent = 0, bool isInv = false ) const;
   void virtual printFanInOut( bool isFanin, int level, int indent = 0, bool isInv = false ) const;

   void printSpaces( int indent ) const;
   void replaceWithFaninNo( unsigned );

   unsigned _lineNo;
   unsigned _colNo;
   unsigned _id;
   mutable unsigned _ref;
   string _name;
   vector< PtrV<CirGate> > _fanins;
   vector< PtrV<CirGate> > _fanouts;

   //IdList* _fecGrp;
   unsigned _fecId : 30;
   unsigned _fecInv : 1;
   unsigned _hasFec : 1;

   unsigned _simResult;

   int _satVar;

   static unsigned _globalRef;

};

template <class Gate>
class PtrV
{
	#define NEG 0x01
	#define UNDEF 0x02
	#define ALL 0x03
public: 
	PtrV( Gate* g, bool inv = false, bool und = false ) : 
		_ptrV( size_t(g) ) { 
		if ( inv ) { _ptrV |= NEG; }
		if ( und ) { _ptrV |= UNDEF; }
	}
	Gate* ptr() const { 
		return (Gate*) (  _ptrV & ~size_t(ALL) ); 
	}
	bool isInv() const {
		return ( _ptrV & NEG );
	}
	bool isUndef() const {
		return ( _ptrV & UNDEF );
	}
	PtrV<Gate> inv() { 
		PtrV<Gate> temp = *this; 
		temp._ptrV ^= NEG;
		return temp;
	}
	bool operator == ( PtrV<Gate> p ) const {
		return _ptrV == p._ptrV;
	}
private: 
	size_t _ptrV;
};

class Const0Gate : public CirGate
{
public:
	Const0Gate() : CirGate( 0, 0 ) {}
	virtual ~Const0Gate() {}

	virtual string getTypeStr() const{ return Const0Gate::_typeStr; }

	virtual void printGate() const;
	virtual void simulate() {};

	static string typeName() { return _typeStr; }
private:
	static const string _typeStr;
};

class PIGate : public CirGate
{
public: 
	PIGate( unsigned id, unsigned l, unsigned c = 0, string n = "" ) : CirGate( id, l, c, n ){}
	virtual ~PIGate() {}

	virtual string getTypeStr() const { return PIGate::_typeStr; }

	virtual void simulate() {};

	static string typeName() { return _typeStr; }

	void initSim( unsigned s ) { _simResult = s; }
	void setSimBit( unsigned b, unsigned i );

	//virtual void dfsPost( vector<CirGate*>& );
	
	//virtual void printGate();
protected:
	static const string _typeStr;
	//string _Name;
};

class POGate : public CirGate
{
public: 
	POGate( unsigned id, unsigned l, unsigned c = 0, string n = "" ) : CirGate( id, l, c, n ){}
	virtual ~POGate() {}

	virtual string getTypeStr() const { return POGate::_typeStr; }

	virtual void merge( CirGate* g ) {}

	virtual void simulate();

	static string typeName() { return _typeStr; }

	//virtual void dfsPost( vector<CirGate*>& );

	//virtual void printGate();
protected: 
	static const string _typeStr;
	//string _Name;
};

class AigGate : public CirGate
{
public:
	AigGate( unsigned id, unsigned l, unsigned c = 0 ) : 
		CirGate( id, l, c ) {}
	virtual ~AigGate() {}

	virtual string getTypeStr() const { return _typeStr; }

	virtual bool selfOptimize( CirGate* the0 );

	virtual void simulate();

	static string typeName() { return _typeStr; }

	//virtual void dfsPost( vector<CirGate*>& );

	//virtual void printGate();
protected:
	static const string _typeStr;
};

class UndefGate : public CirGate
{
public:
	UndefGate( unsigned id ) :
		CirGate( id, 0 ) { }
	virtual ~UndefGate() {}

	virtual string getTypeStr() const { return UndefGate::_typeStr; }

	virtual void simulate() {};

	static string typeName() { return _typeStr; }

	virtual void dfsPost( vector<CirGate*>& ) ;

	//virtual void printGate();
protected:
	void virtual printFanInOut( bool isFanin, int level, int indent = 0, bool isInv = false );
	static const string _typeStr;
};
#endif // CIR_GATE_H
