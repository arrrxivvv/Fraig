/****************************************************************************
  FileName     [ myHash.h ]
  PackageName  [ util ]
  Synopsis     [ Define Hash and Cache ADT ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2009-2013 LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef MY_HASH_H
#define MY_HASH_H

#include <vector>

using namespace std;

extern size_t getHashSize( size_t );

//--------------------
// Define Hash classes
//--------------------
// To use Hash ADT, you should define your own HashKey class.
// It should at least overload the "()" and "==" operators.
//
// class HashKey
// {
// public:
//    HashKey() {}
// 
//    size_t operator() () const { return 0; }
// 
//    bool operator == (const HashKey& k) { return true; }
// 
// private:
// };
//
template <class HashKey, class HashData>
class Hash
{
typedef pair<HashKey, HashData> HashNode;
   friend class iterator;

public:
   Hash() : _numBuckets(0), _buckets(0) {}
   Hash(size_t b) : _numBuckets(0), _buckets(0) { init(b); }
   ~Hash() { reset(); }

   // TODO: implement the Hash<HashKey, HashData>::iterator
   // o An iterator should be able to go through all the valid HashNodes
   //   in the Hash
   // o Functions to be implemented:
   //   - constructor(s), destructor
   //   - operator '*': return the HashNode
   //   - ++/--iterator, iterator++/--
   //   - operators '=', '==', !="
   //
   // (_bId, _bnId) range from (0, 0) to (_numBuckets, 0)
   //
   class iterator
   {
      friend class Hash<HashKey, HashData>;

   public:
      iterator() : _hash(0), _bucketNum(0), _num(0) {}
	  iterator( const Hash* h, size_t b, size_t n ) : 
	     _hash(h), _bucketNum(b), _num(n) {}

	 HashNode& operator * () { return _hash->_buckets[_bucketNum][_num]; }
	 const HashNode& operator * () const { return _hash->_buckets[_bucketNum][_num]; }
	 iterator& operator ++ () {
	    ++_num;
		if ( _num >= _hash->_buckets[_bucketNum].size() ) {
			toNextBuc();
	    }
	    //else { ++_num; }
	    return *this;
	 }
	 iterator operator ++ (int) {
	    iterator temp = *this;
	    ++(*this);
	    return temp;
	 }

	 iterator& operator -- () {
	    if ( _num == 0 ) {
			toPrevBuc();
	    }
	    else { --_num; }
	    return *this;
	 }
	 iterator operator -- (int) {
	    iterator temp = *this;
	    --( *this );
	    return temp;
	 }

	 iterator& operator = ( const iterator& it ) {
	    _hash = it._hash;
		_bucketNum = it._bucketNum;
	    _num = it._num;
	    return *this;
	 }

	 bool operator == ( const iterator& it ) const {
	    return ( _hash == it._hash &&
	             _bucketNum == it._bucketNum && 
			   _num == it._num );
	 }

	 bool operator != ( const iterator& it ) const {
	    return !( *this == it );
	 }

   private:
	 void toNextBuc() {
	 	size_t tempB = _bucketNum;
		while ( tempB + 1 < _hash->_numBuckets ) {
			++tempB;
			if ( !( _hash->_buckets[tempB] ).empty() ) {
				_bucketNum = tempB;
				_num = 0;
				return;
			}
		}

		_num = ( _hash->_buckets[_bucketNum] ).size();
	 }

	 void toPrevBuc() {
	 	size_t tempB = _bucketNum;
		while ( tempB > 0 ) {
			--tempB;
			if ( !( _hash->_buckets[tempB] ).empty() ) {
				_bucketNum = tempB;
				_num = ( _hash->_buckets[_bucketNum] ).size() - 1;
				return;
			}
		}
		_num = 0;
	 }

	 const Hash* _hash;
	 size_t _bucketNum;
	 size_t _num;
   };

   // TODO: implement these functions
   //
   // Point to the first valid data
   iterator begin() const { 
	  iterator it( this, 0, 0 );
	  if ( empty() ) {
	     return it;
	  }

	  if ( _buckets[0].empty() ) {
	     it.toNextBuc();
	  }
	  return it;
   }
   // Pass the end
   iterator end() const {
	  if ( empty() ) {
	     return begin();
	  }

	  iterator it( this, _numBuckets - 1, 0 );
	  if ( _buckets[ _numBuckets-1 ].empty() ) {
	     it.toPrevBuc();
		 ++it._num;
	  }
	  else {
	     it.toNextBuc();
	  }
	  return it;
   }
   // return true if no valid data
   bool empty() const { 
      for ( size_t i = 0; i < _numBuckets; ++i ) {
	    if ( !_buckets[i].empty() ) {
	       return false;
	    }
	 }
	 return true; 
   }
   // number of valid data
   size_t size() const { 
      size_t s = 0; 
	 for ( size_t i = 0; i < _numBuckets; ++i ) {
	    s += _buckets[i].size();
	 }
	 return s; 
   }
   size_t numBuckets() const { return _numBuckets; }

   vector<HashNode>& operator [] (size_t i) { return _buckets[i]; }
   const vector<HashNode>& operator [](size_t i) const { return _buckets[i]; }

   void init(size_t b) { 
	 reset();
	 _numBuckets = getHashSize( b );
	 _buckets = new vector<HashNode>[_numBuckets];
   }
   void reset() { 
      delete[] _buckets;
	  _numBuckets = 0;
   }

   // check if k is in the hash...
   // if yes, update n and return true;
   // else return false;
   bool check(const HashKey& k, HashData& n) { 
	 size_t buc = bucketNum( k );
	 for ( size_t i = 0; i < _buckets[buc].size(); ++i ) {
	    if ( _buckets[buc][i].first == k ) {
	       n = _buckets[buc][i].second;
		  return true;
	    }
	 }
	 return false; 
   }

   // return true if inserted successfully (i.e. k is not in the hash)
   // return false is k is already in the hash ==> will not insert
   bool insert(const HashKey& k, const HashData& d) { 
      size_t n = bucketNum( k );
	 for ( size_t i = 0; i < _buckets[n].size(); ++i ) {
	    if ( k == _buckets[n][i].first ) {
	       return false;
	    }
	 }
	 _buckets[n].push_back( make_pair( k, d ) );
	 return true; 
   }

   // return true if inserted successfully (i.e. k is not in the hash)
   // return false is k is already in the hash ==> still do the insertion
   bool replaceInsert(const HashKey& k, const HashData& d) { 
      size_t n = bucketNum( k );
	 for ( size_t i = 0; i < _buckets[n].size(); ++i ) {
	    if ( k == _buckets[n][i].first ) {
	       _buckets[n][i].second = d;
		  return false;
	    }
	 }
	 _buckets.push_back( make_pair( k, d ) );
	 return true; 
   }

   // Need to be sure that k is not in the hash
   void forceInsert(const HashKey& k, const HashData& d) { 
      size_t n = bucketNum( k );
	 _buckets[n].push_back( make_pair( k, d ) );
   }

private:
   // Do not add any extra data member
   size_t                   _numBuckets;
   vector<HashNode>*        _buckets;

   size_t bucketNum(const HashKey& k) const {
      return (k() % _numBuckets); }

};


//---------------------
// Define Cache classes
//---------------------
// To use Cache ADT, you should define your own HashKey class.
// It should at least overload the "()" and "==" operators.
//
// class CacheKey
// {
// public:
//    CacheKey() {}
//    
//    size_t operator() () const { return 0; }
//   
//    bool operator == (const CacheKey&) const { return true; }
//       
// private:
// }; 
// 
template <class CacheKey, class CacheData>
class Cache
{
typedef pair<CacheKey, CacheData> CacheNode;

public:
   Cache() : _size(0), _cache(0) {}
   Cache(size_t s) : _size(0), _cache(0) { init(s); }
   ~Cache() { reset(); }

   // NO NEED to implement Cache::iterator class

   // TODO: implement these functions
   //
   // Initialize _cache with size s
   void init(size_t s) { 
      reset();
	  _size = s;
	  _cache = new CacheNode[_size];
   }
   void reset() { 
      delete _cache; 
	  _cache = 0;
   }

   size_t size() const { return _size; }

   CacheNode& operator [] (size_t i) { return _cache[i]; }
   const CacheNode& operator [](size_t i) const { return _cache[i]; }

   // return false if cache miss
   bool read(const CacheKey& k, CacheData& d) const { 
	  size_t place = k() % _size();
	  if ( _cache[place].first == k ) {
	     d = _cache[place].second;
		 return true;
	  }
	  return false; 
   }
   // If k is already in the Cache, overwrite the CacheData
   void write(const CacheKey& k, const CacheData& d) {
      _cache[ k() % _size() ] = CacheNode( k, d );
   }

private:
   // Do not add any extra data member
   size_t         _size;
   CacheNode*     _cache;
};


#endif // MY_HASH_H
