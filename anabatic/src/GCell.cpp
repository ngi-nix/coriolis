// -*- mode: C++; explicit-buffer-name: "GCell.cpp<anabatic>" -*-
//
// This file is part of the Coriolis Software.
// Copyright (c) UPMC 2016-2016, All Rights Reserved
//
// +-----------------------------------------------------------------+
// |                   C O R I O L I S                               |
// |     A n a b a t i c  -  Global Routing Toolbox                  |
// |                                                                 |
// |  Author      :                    Jean-Paul CHAPUT              |
// |  E-mail      :            Jean-Paul.Chaput@lip6.fr              |
// | =============================================================== |
// |  C++ Module  :  "./GCell.cpp"                                   |
// +-----------------------------------------------------------------+


#include <iostream>
#include "hurricane/Bug.h"
#include "hurricane/Warning.h"
#include "hurricane/Breakpoint.h"
#include "hurricane/Contact.h"
#include "hurricane/RoutingPad.h"
#include "hurricane/UpdateSession.h"
#include "anabatic/GCell.h"
#include "anabatic/AnabaticEngine.h"


namespace {

  using namespace std;
  using namespace Hurricane;
  using namespace Anabatic;


// -------------------------------------------------------------------
// Class  :  "::UsedFragments".

  class UsedFragments {
    private:
      class Axiss;

      class Axis {
        public:
                                Axis             ( UsedFragments*, DbU::Unit axis );
          inline DbU::Unit      getAxis          () const;
          inline UsedFragments* getUsedFragments () const;
                 void           merge            ( const Interval& mergeChunk );
                 Interval       getMaxFree       () const;
        private:
          UsedFragments* _ufragments;
          DbU::Unit      _axis;
          list<Interval> _chunks;
      };

    private:
      class AxisCompare {
        public:
          bool operator() ( const Axis* lhs, const Axis* rhs );
      };

      class AxisMatch : public unary_function<Axis*,bool> {
        public:
          inline      AxisMatch  ( DbU::Unit axis );
          inline bool operator() ( const Axis* );
        private:
          DbU::Unit  _axis;
      };

    public:
                        UsedFragments ();
                       ~UsedFragments ();
      inline DbU::Unit  getPitch      () const;
      inline DbU::Unit  getMin        () const;
      inline DbU::Unit  getMax        () const;
             Interval   getMaxFree    () const;
      inline void       setSpan       ( DbU::Unit min, DbU::Unit max );
      inline void       setCapacity   ( size_t );
      inline void       incGlobals    ( size_t count=1 );
      inline void       setPitch      ( DbU::Unit );
             void       merge         ( DbU::Unit axis, const Interval& );
    private:
      DbU::Unit      _pitch;
      vector<Axis*>  _axiss;
      Interval       _span;
      size_t         _capacity;
      size_t         _globals;
  };


  UsedFragments::Axis::Axis ( UsedFragments* ufragments, DbU::Unit axis )
    : _ufragments(ufragments)
    , _axis      (axis)
    , _chunks    ()
  {
    merge( Interval( ufragments->getMin()-ufragments->getPitch(), ufragments->getMin() ) );
    merge( Interval( ufragments->getMax(), ufragments->getMax()+ufragments->getPitch() ) );
  }

  inline DbU::Unit      UsedFragments::Axis::getAxis          () const { return _axis; }
  inline UsedFragments* UsedFragments::Axis::getUsedFragments () const { return _ufragments; }


  void  UsedFragments::Axis::merge ( const Interval& chunkMerge )
  {
    // cerr << "    Merge @" << DbU::getValueString(_axis)
    //      << " " << chunkMerge << endl;

    list<Interval>::iterator imerge = _chunks.end();
    list<Interval>::iterator ichunk = _chunks.begin();

    while ( ichunk != _chunks.end() ) {
      if (chunkMerge.getVMax() < (*ichunk).getVMin()) break;

      if (chunkMerge.intersect(*ichunk)) {
        if (imerge == _chunks.end()) {
          imerge = ichunk;
          (*imerge).merge( chunkMerge );
        } else {
          (*imerge).merge( *ichunk );
          ichunk = _chunks.erase( ichunk );
          continue;
        }
      }
      ichunk++;
    }

    if (imerge == _chunks.end()) {
      _chunks.insert( ichunk, chunkMerge );
    }
  }


  Interval  UsedFragments::Axis::getMaxFree () const
  {
    list<Interval>::const_iterator ichunk =   _chunks.begin();
    list<Interval>::const_iterator iend   = --_chunks.end();

    Interval maxFree;
    for ( ; ichunk != iend ; ++ichunk ) {
      list<Interval>::const_iterator inext = ichunk;
      ++inext;

      if (inext == iend) break;

      Interval currentFree ( (*ichunk).getVMax(), (*inext).getVMin() );
      if (currentFree.getSize() > maxFree.getSize())
        maxFree = currentFree;

      // cerr << "    @" << DbU::getValueString(_axis)
      //      << " before:" << *ichunk << " after:" << *inext
      //      << " size:" << DbU::getValueString(currentFree.getSize()) << endl;
    }

    return maxFree;
  }


  inline bool  UsedFragments::AxisCompare::operator() ( const Axis* lhs, const Axis* rhs )
  {
    if (lhs->getAxis () < rhs->getAxis ()) return true;
    return false;
  }


  inline  UsedFragments::AxisMatch::AxisMatch ( DbU::Unit axis )
    : _axis(axis)
  { }


  inline bool  UsedFragments::AxisMatch::operator() ( const Axis* axis )
  { return (axis->getAxis() == _axis); }


  UsedFragments::UsedFragments ()
    : _pitch   (0)
    , _axiss   ()
    , _span    (false)
    , _capacity(0)
    , _globals (0)
  { }


  UsedFragments::~UsedFragments ()
  {
    while ( not _axiss.empty() ) {
      delete (*_axiss.begin());
      _axiss.erase( _axiss.begin() );
    }
  }


  inline DbU::Unit  UsedFragments::getPitch    () const { return _pitch; }
  inline DbU::Unit  UsedFragments::getMin      () const { return _span.getVMin(); }
  inline DbU::Unit  UsedFragments::getMax      () const { return _span.getVMax(); }
  inline void       UsedFragments::setPitch    ( DbU::Unit pitch ) { _pitch=pitch; }
  inline void       UsedFragments::setSpan     ( DbU::Unit min, DbU::Unit max ) { _span=Interval(min,max); }
  inline void       UsedFragments::setCapacity ( size_t capacity ) { _capacity=capacity; }
  inline void       UsedFragments::incGlobals  ( size_t count ) { _globals+=count; }


  void  UsedFragments::merge ( DbU::Unit axis, const Interval& chunkMerge )
  {
    Interval restrict = chunkMerge.getIntersection(_span);
    if (restrict.isEmpty()) return;

    vector<Axis*>::iterator iaxis = find_if( _axiss.begin(), _axiss.end(), AxisMatch(axis) );

    Axis* paxis = NULL;
    if (iaxis == _axiss.end()) {
      paxis = new Axis(this,axis);
      _axiss.push_back ( paxis );
      stable_sort( _axiss.begin(), _axiss.end(), AxisCompare() );
    } else {
      paxis = *iaxis;
    }

    paxis->merge( restrict );
  }


  Interval  UsedFragments::getMaxFree () const
  {
  //cerr << "capacity:" << _capacity << " _globals:" << _globals << " _axiss:" << _axiss.size() << endl;
    if (_capacity > _globals + _axiss.size() + 1) return _span;

    Interval maxFree;
    vector<Axis*>::const_iterator iaxis = _axiss.begin();
    for ( ; iaxis != _axiss.end() ; ++iaxis ) {
      Interval axisMaxFree = (*iaxis)->getMaxFree();
      if (axisMaxFree.getSize() > maxFree.getSize())
        maxFree = axisMaxFree;
    }

    return maxFree;
  }


} // End of anonymous namespace.


namespace Anabatic {

  using std::cerr;
  using std::endl;
  using Hurricane::Bug;
  using Hurricane::Error;
  using Hurricane::Warning;
  using Hurricane::UpdateSession;
  using Hurricane::Horizontal;
  using Hurricane::Vertical;


// -------------------------------------------------------------------
// Class  :  "Katabatic::GCell::CompareByDensity".


  GCell::CompareByDensity::CompareByDensity ( size_t depth )
    : _depth(depth)
  { }


  bool  GCell::CompareByDensity::operator() ( GCell* lhs, GCell* rhs )
  {
    float difference = lhs->getDensity(_depth) - rhs->getDensity(_depth);
    if (difference != 0.0) return (difference > 0.0);
    
    return lhs->getId() < rhs->getId();
  }


// -------------------------------------------------------------------
// Class  :  "Anabatic::GCell".

  Name          GCell::_extensionName = "Anabatic::GCell";
  unsigned int  GCell::_displayMode   = GCell::Boundary;


  unsigned int  GCell::getDisplayMode      () { return _displayMode; }
  void          GCell::setDisplayMode      ( unsigned int mode ) { _displayMode = mode; }


  GCell::GCell ( AnabaticEngine* anabatic, DbU::Unit xmin, DbU::Unit ymin )
    : Super(anabatic->getCell())
    , _observable    ()
    , _anabatic      (anabatic)
    , _flags         (Flags::HChannelGCell|Flags::Invalidated)
    , _westEdges     ()
    , _eastEdges     ()
    , _southEdges    ()
    , _northEdges    ()
    , _xmin          (xmin)
    , _ymin          (ymin)
    , _gcontacts     ()
    , _vsegments     ()
    , _hsegments     ()
    , _contacts      ()
    , _depth         (Session::getRoutingGauge()->getDepth())
    , _pinDepth      (0)
    , _blockages     (new DbU::Unit [_depth])
    , _cDensity      (0.0)
    , _densities     (new float [_depth])
    , _feedthroughs  (new float [_depth])
    , _fragmentations(new float [_depth])
    , _globalsCount  (new float [_depth])
    , _key           (this,1)
  {
    for ( size_t i=0 ; i<_depth ; i++ ) {
      _blockages     [i] = 0;
      _densities     [i] = 0.0;
      _feedthroughs  [i] = 0.0;
      _fragmentations[i] = 0.0;
      _globalsCount  [i] = 0.0;

      if (Session::getRoutingGauge()->getLayerGauge(i)->getType() == Constant::PinOnly)
        ++_pinDepth;
    }

    updateKey( 1 );
  }


  void  GCell::_postCreate ()
  {
    Super::_postCreate();
    _anabatic->_add( this );
  }


  GCell* GCell::create ( AnabaticEngine* anabatic )
  {
    if (not anabatic)            throw Error( "GCell::create(): NULL anabatic argument." );
    if (not anabatic->getCell()) throw Error( "GCell::create(): AnabaticEngine has no Cell loaded." );

    anabatic->openSession();
    GCell* gcell = new GCell ( anabatic
                             , anabatic->getCell()->getAbutmentBox().getXMin()
                             , anabatic->getCell()->getAbutmentBox().getYMin() );
    gcell->_postCreate();
    gcell->_revalidate();
    Session::close();

    return gcell;
  }


  GCell* GCell::_create ( DbU::Unit xmin, DbU::Unit ymin )
  {
    GCell* gcell = new GCell ( getAnabatic(), xmin, ymin );
    gcell->_postCreate();
    return gcell;
  }


  GCell::~GCell ()
  {
  //cdebug_log(145,0) << "GCell::~GCell()" << endl;

    delete [] _blockages;
    delete [] _densities;
    delete [] _feedthroughs;
    delete [] _fragmentations;
    delete [] _globalsCount;
  }


  void  GCell::_destroyEdges ()
  {
    while (not  _westEdges.empty()) (* _westEdges.rbegin())->destroy();
    while (not  _eastEdges.empty()) (* _eastEdges.rbegin())->destroy();
    while (not _southEdges.empty()) (*_southEdges.rbegin())->destroy();
    while (not _northEdges.empty()) (*_northEdges.rbegin())->destroy();
  }


  void  GCell::_preDestroy ()
  {
    _destroyEdges();
    _anabatic->_remove( this );
    Super::_preDestroy();
  }


  void  GCell::_remove ( Edge* edge, Flags side )
  {
    if (side.contains(Flags::WestSide )) erase_element(  _westEdges, edge );
    if (side.contains(Flags::EastSide )) erase_element(  _eastEdges, edge );
    if (side.contains(Flags::SouthSide)) erase_element( _southEdges, edge );
    if (side.contains(Flags::NorthSide)) erase_element( _northEdges, edge );
  }


  void  GCell::_add ( Edge* edge, Flags side )
  {
    cdebug_log(110,1) << "GCell::_add(side): side:" << side << " " << edge << endl;
    if (side.contains(Flags::WestSide)) {
      cdebug_log(110,0) << "Adding to West side of " << this << endl;
      for ( auto iedge=_westEdges.begin() ; iedge != _westEdges.end() ; ++iedge )
        if ((*iedge)->getAxisMin() > edge->getAxisMin()) {
          _westEdges.insert( iedge, edge );
          cdebug_tabw(110,-1);
          return;
        }
      _westEdges.push_back( edge );
    }

    if (side.contains(Flags::EastSide)) {
      cdebug_log(110,0) << "Adding to East side of " << this << endl;
      for ( auto iedge=_eastEdges.begin() ; iedge != _eastEdges.end() ; ++iedge )
        if ((*iedge)->getAxisMin() > edge->getAxisMin()) {
          _eastEdges.insert( iedge, edge );
          cdebug_tabw(110,-1);
          return;
        }
      _eastEdges.push_back( edge );
    }

    if (side.contains(Flags::SouthSide)) {
      cdebug_log(110,0) << "Adding to South side of " << this << endl;
      for ( auto iedge=_southEdges.begin() ; iedge != _southEdges.end() ; ++iedge )
        cdebug_log(110,0) << "| @" << DbU::getValueString((*iedge)->getAxisMin()) << " " << *iedge << endl;

      for ( auto iedge=_southEdges.begin() ; iedge != _southEdges.end() ; ++iedge )
        if ((*iedge)->getAxisMin() > edge->getAxisMin()) {
          cdebug_log(110,0) << "Insert *before* " << *iedge << endl;

          _southEdges.insert( iedge, edge );
          for ( auto iedge2=_southEdges.begin() ; iedge2 != _southEdges.end() ; ++iedge2 )
            cdebug_log(110,0) << "| @" <<  DbU::getValueString((*iedge2)->getAxisMin()) << " " << *iedge2 << endl;
          cdebug_tabw(110,-1);
          return;
        }
      _southEdges.push_back( edge );
    }

    if (side.contains(Flags::NorthSide)) {
      cdebug_log(110,0) << "Adding to North side of " << this << endl;
      for ( auto iedge=_northEdges.begin() ; iedge != _northEdges.end() ; ++iedge )
        if ((*iedge)->getAxisMin() > edge->getAxisMin()) {
          _northEdges.insert( iedge, edge );
          cdebug_tabw(110,-1);
          return;
        }
      _northEdges.push_back( edge );
    }

    cdebug_tabw(110,-1);
  }


  Contact* GCell::hasGContact ( const Net* net ) const
  {
    for ( Contact* contact : _gcontacts ) {
      if (contact->getNet() == net) return contact;
    }
    return NULL;
  }


  Contact* GCell::hasGContact ( const Contact* owned ) const
  {
    for ( Contact* contact : _gcontacts ) {
      if (contact == owned) return contact;
    }
    return NULL;
  }


  Contact* GCell::breakGoThrough ( Net* net )
  {
    for ( Edge* edge : _eastEdges ) {
      for ( Segment* segment : edge->getSegments() ) {
        if (segment->getNet() == net) 
          return getAnabatic()->breakAt( segment, this );
      }
    }

    for ( Edge* edge : _northEdges ) {
      for ( Segment* segment : edge->getSegments() ) {
        if (segment->getNet() == net) 
          return getAnabatic()->breakAt( segment, this );
      }
    }

    return NULL;
  }


  Edge* GCell::getEdgeTo ( GCell* neighbor, Flags sideHint ) const
  {
    for ( Edge* edge : getEdges(sideHint) ) {
      if (edge->getOpposite(this) == neighbor) return edge;
    }
    return NULL;
  }


  Edge* GCell::getEdgeAt ( Flags sideHint, DbU::Unit u ) const
  {
    for ( Edge* edge : getEdges(sideHint) ) {
      GCell* side = edge->getOpposite(this);
      if ( (sideHint & (Flags::WestSide |Flags::EastSide )) and (u < side->getYMax()) ) return edge;
      if ( (sideHint & (Flags::SouthSide|Flags::NorthSide)) and (u < side->getXMax()) ) return edge;
    }
    return NULL;
  }


  GCell* GCell::getWest  ( DbU::Unit y ) const
  {
    for ( Edge* edge : _westEdges ) {
      GCell* side = edge->getOpposite(this);
      if (y < side->getYMax()) return side;
    }
    return NULL;
  }


  GCell* GCell::getEast  ( DbU::Unit y ) const
  {
    for ( Edge* edge : _eastEdges ) {
      GCell* side = edge->getOpposite(this);
      if (y < side->getYMax()) return side;
    }
    return NULL;
  }


  GCell* GCell::getSouth ( DbU::Unit x ) const
  {
    for ( Edge* edge : _southEdges ) {
      GCell* side = edge->getOpposite(this);
      if (x < side->getXMax()) return side;
    }
    return NULL;
  }


  GCell* GCell::getNorth ( DbU::Unit x ) const
  {
    for ( Edge* edge : _northEdges ) {
      GCell* side = edge->getOpposite(this);
      if (x < side->getXMax()) return side;
    }
    return NULL;
  }


  GCell* GCell::getNeighborAt ( Flags side, DbU::Unit axis ) const
  {
    if (side & Flags::EastSide ) return getEast (axis);
    if (side & Flags::WestSide ) return getWest (axis);
    if (side & Flags::NorthSide) return getNorth(axis);
    if (side & Flags::SouthSide) return getSouth(axis);
    return NULL;
  }


  GCell* GCell::getUnder ( DbU::Unit x, DbU::Unit y ) const
  {
    const GCell* current = this;

    if (not this) cerr << Error("*this* is NULL!") << endl;

    while ( current ) {
      if (not current->isFlat() and current->getBoundingBox().contains(x,y)) break;

      if (x >= current->getXMax()) { current = current->getEast (); continue; }
      if (y >= current->getYMax()) { current = current->getNorth(); continue; }

      cerr << Error( "GCell::getUnder(): No GCell under (%s,%s), this must *never* happen."
                   , DbU::getValueString(x).c_str()
                   , DbU::getValueString(y).c_str()
                   ) << endl;
      current = NULL; break;
    }
    
    return const_cast<GCell*>( current );
  }


  Box  GCell::getBorder ( const GCell* s, const GCell* t )
  {
    Flags flags = Flags::NoFlags;
    flags |= (s->getXMax() == t->getXMin()) ? Flags::EastSide  : 0;
    flags |= (t->getXMax() == s->getXMin()) ? Flags::WestSide  : 0;
    flags |= (s->getYMax() == t->getYMin()) ? Flags::NorthSide : 0;
    flags |= (t->getYMax() == s->getYMin()) ? Flags::SouthSide : 0;

    if (flags & Flags::Vertical) {
      if (flags & Flags::Horizontal) return Box();
      if (flags & Flags::WestSide)
        return Box( s->getXMin(), std::max( s->getYMin(), t->getYMin() )
                  , s->getXMin(), std::min( s->getYMax(), t->getYMax() ) );
      else
        return Box( t->getXMin(), std::max( s->getYMin(), t->getYMin() )
                  , t->getXMin(), std::min( s->getYMax(), t->getYMax() ) );
    }

    if (flags & Flags::Horizontal) {
      if (flags & Flags::Vertical) return Box();
      if (flags & Flags::NorthSide)
        return Box( std::max( s->getXMin(), t->getXMin() ), t->getYMin()
                  , std::min( s->getXMax(), t->getXMax() ), t->getYMin() );
      else
        return Box( std::max( s->getXMin(), t->getXMin() ), s->getYMin()
                  , std::min( s->getXMax(), t->getXMax() ), s->getYMin() );
    }

    return Box();
  }


  GCell*  GCell::vcut ( DbU::Unit x )
  {
    cdebug_log(110,1) << "GCell::vcut() @x:" << DbU::getValueString(x) << " " << this << endl;

    if ( (x < getXMin()) or (x > getXMax()) )
      throw Error( "GCell::vcut(): Vertical cut axis at %s is outside GCell box,\n"
                   "        in %s."
                 , DbU::getValueString(x).c_str()
                 , getString(this).c_str()
                 );

    GCell* chunk = _create( x, getYMin() );
    cdebug_log(110,0) << "New chunk:" << chunk << endl;

    _moveEdges( chunk, 0, Flags::EastSide );
    Edge::create( this, chunk, Flags::Horizontal );

    if (not _southEdges.empty()) {
      cdebug_log(110,0) << "Split/create south edges." << endl;

      size_t iedge = 0;
      for ( ; (iedge < _southEdges.size()) ; ++iedge ) {
        cdebug_log(110,0) << "[" << iedge << "] xmax of:"
                        << _southEdges[iedge]->getOpposite(this)
                        << " " << _southEdges[iedge] << endl;
        if (x <= _southEdges[iedge]->getOpposite(this)->getXMax()) break;
      }

      if (      (x <  _southEdges[iedge]->getOpposite(this)->getXMax())
         or (   (x == _southEdges[iedge]->getOpposite(this)->getXMax())
            and (chunk->getXMax() == getXMax())) )
        Edge::create( _southEdges[iedge]->getOpposite(this), chunk, Flags::Vertical );

      _moveEdges( chunk, iedge+1, Flags::SouthSide );
    }

    if (not _northEdges.empty()) {
      cdebug_log(110,0) << "Split/create north edges." << endl;

      size_t iedge = 0;
      for ( ; (iedge < _northEdges.size()) ; ++iedge )
        if (x <= _northEdges[iedge]->getOpposite(this)->getXMax()) break;

      if (      (x <  _northEdges[iedge]->getOpposite(this)->getXMax())
         or (   (x == _northEdges[iedge]->getOpposite(this)->getXMax())
            and (chunk->getXMax() == getXMax())) )
        Edge::create( chunk, _northEdges[iedge]->getOpposite(this), Flags::Vertical );

      _moveEdges( chunk, iedge+1, Flags::NorthSide );
    }

    _revalidate();
    chunk->_revalidate();

    cdebug_tabw(110,-1);

    return chunk;
  }


  GCell* GCell::hcut ( DbU::Unit y )
  {
    cdebug_log(110,1) << "GCell::hcut() @y:" << DbU::getValueString(y) << " " << this << endl;

    if ( (y < getYMin()) or (y > getYMax()) )
      throw Error( "GCell::hcut(): Horizontal cut axis at %s is outside GCell box,\n"
                   "        in %s."
                 , DbU::getValueString(y).c_str()
                 , getString(this).c_str()
                 );

    GCell* chunk = _create( getXMin(), y );
    cdebug_log(110,0) << "New chunk:" << chunk << endl;

    _moveEdges( chunk, 0, Flags::NorthSide );
    Edge::create( this, chunk, Flags::Vertical );

    if (not _westEdges.empty()) {
      size_t iedge = 0;
      for ( ; (iedge < _westEdges.size()) ; ++iedge )
        if (y <= _westEdges[iedge]->getOpposite(this)->getYMax()) break;

      if (      (y <  _westEdges[iedge]->getOpposite(this)->getYMax())
         or (   (y == _westEdges[iedge]->getOpposite(this)->getYMax())
            and (chunk->getYMax() == getYMax())) )
        Edge::create( _westEdges[iedge]->getOpposite(this), chunk, Flags::Horizontal );

      _moveEdges( chunk, iedge+1, Flags::WestSide );
    }

    if (not _eastEdges.empty()) {
      size_t iedge = 0;
      for ( ; (iedge < _eastEdges.size()) ; ++iedge )
        if (y <= _eastEdges[iedge]->getOpposite(this)->getYMax()) break;

      if (      (y <  _eastEdges[iedge]->getOpposite(this)->getYMax())
         or (   (y == _eastEdges[iedge]->getOpposite(this)->getYMax())
            and (chunk->getYMax() == getYMax())) )
        Edge::create( chunk, _eastEdges[iedge]->getOpposite(this), Flags::Horizontal );

      _moveEdges( chunk, iedge+1, Flags::EastSide );
    }

    _revalidate();
    chunk->_revalidate();

    cdebug_tabw(110,-1);

    return chunk;
  }


  bool  GCell::doGrid ()
  {
    getAnabatic()->openSession();

    DbU::Unit side  = Session::getSliceHeight();
    Interval  hspan = getSide( Flags::Horizontal );
    Interval  vspan = getSide( Flags::Vertical );

    if (hspan.getSize() < 3*side) {
      cerr << Error( "GCell::doGrid(): GCell is too narrow (dx:%s) to build a grid.\n"
                     "        (%s)"
                   , DbU::getValueString(hspan.getSize()).c_str()
                   , getString(this).c_str()
                   ) << endl;
      Session::close();
      return false;
    }

    if (vspan.getSize() < 3*side) {
      cerr << Error( "GCell::doGrid(): GCell is too narrow (dy:%s) to build a grid.\n"
                     "        (%s)"
                   , DbU::getValueString(vspan.getSize()).c_str()
                   , getString(this).c_str()
                   ) << endl;
      return false;
    }

    GCell*    row    = this;
    GCell*    column = NULL;
    DbU::Unit ycut   = vspan.getVMin()+side;
    for ( ; ycut < vspan.getVMax() ; ycut += side ) {
      column = row;
      row    = row->hcut( ycut );
      row->setType( Flags::MatrixGCell );

      for ( DbU::Unit xcut = hspan.getVMin()+side ; xcut < hspan.getVMax() ; xcut += side ) {
        column = column->vcut( xcut );
        column->setType( Flags::MatrixGCell );
      }
    }

    column = row;
    for ( DbU::Unit xcut = hspan.getVMin()+side ; xcut < hspan.getVMax() ; xcut += side ) {
      column = column->vcut( xcut );
      column->setType( Flags::MatrixGCell );
    }

    setType( Flags::MatrixGCell );

  //size_t hLocal = - getAnabatic()->getConfiguration()->getHEdgeLocal();
  //size_t vLocal = - getAnabatic()->getConfiguration()->getVEdgeLocal();
  //for ( ; ibegin < gcells.size() ; ++ibegin ) {
  //  gcells[ibegin]->setType( Flags::MatrixGCell );

  //  for ( Edge* edge : gcells[ibegin]->getEdges(Flags::NorthSide|Flags::EastSide) ) {
  //    if (edge->isHorizontal()) edge->incCapacity( hLocal );
  //    else                      edge->incCapacity( vLocal );
  //  }
  //}

    Session::close();
    return true;
  }


  void  GCell::_revalidate ()
  {
    cdebug_log(110,1) << "GCell::revalidate() " << this << endl;
    cdebug_log(110,1) << "West side."  << endl; for ( Edge* edge : _westEdges  ) edge->revalidate(); cdebug_tabw(110,-1);
    cdebug_log(110,1) << "East side."  << endl; for ( Edge* edge : _eastEdges  ) edge->revalidate(); cdebug_tabw(110,-1);
    cdebug_log(110,1) << "South side." << endl; for ( Edge* edge : _southEdges ) edge->revalidate(); cdebug_tabw(110,-1);
    cdebug_log(110,1) << "North side." << endl; for ( Edge* edge : _northEdges ) edge->revalidate(); cdebug_tabw(110,-1);

    if (_xmin > getXMax()+1)
      cerr << Error( "GCell::_revalidate(): %s, X Min is greater than Max.", getString(this).c_str() );
    if (_ymin > getYMax()+1)
      cerr << Error( "GCell::_revalidate(): %s, Y Min is greater than Max.", getString(this).c_str() );

    _anabatic->_updateLookup( this );
  //_anabatic->getMatrix()->show();
    cdebug_tabw(110,-1);
  }


  void  GCell::_moveEdges ( GCell* dest, size_t ibegin, Flags flags )
  {
    cdebug_log(110,1) << "GCell::_moveEdges() " << this << endl;
    cdebug_log(110,0)   << "           toward " << dest << endl;
    cdebug_log(110,0)   << "           ibegin: " << ibegin << " flags:" << flags << endl;

    size_t iclear = ibegin;

    if (flags.contains(Flags::SouthSide) and not _southEdges.empty()) {
      cdebug_log(110,0) << "South side." << endl;

      if (iclear < _southEdges.size()) {
        for ( size_t iedge=ibegin ; (iedge < _southEdges.size()) ; ++iedge ) {
          _southEdges[iedge]->_setTarget( dest );
          dest->_southEdges.push_back( _southEdges[iedge] );
        }
        _southEdges.resize( iclear );
      } else {
        if (iclear > _southEdges.size())
          cerr << Error("GCell::_moveEdges(): On south side, iclear=%u is greater than size()-1=%u\n"
                        "        (%s)"
                       , iclear
                       , _southEdges.size()
                       , getString(this).c_str()
                       ) << endl;
      }
    }

    if (flags.contains(Flags::NorthSide) and not _northEdges.empty()) {
      cdebug_log(110,0) << "North side." << endl;

      if (iclear < _northEdges.size()) {
        for ( size_t iedge=ibegin ; (iedge < _northEdges.size()) ; ++iedge ) {
          _northEdges[iedge]->_setSource( dest );
          dest->_northEdges.push_back( _northEdges[iedge] );
        }
        _northEdges.resize( iclear );
      } else {
        if (iclear > _northEdges.size())
          cerr << Error("GCell::_moveEdges(): On north side, iclear=%u is greater than size()-1=%u\n"
                        "        (%s)"
                       , iclear
                       , _northEdges.size()
                       , getString(this).c_str()
                       ) << endl;
      }
    }

    if (flags.contains(Flags::WestSide) and not _westEdges.empty()) {
      cdebug_log(110,0) << "West side." << endl;

      if (iclear < _westEdges.size()) {
        for ( size_t iedge=ibegin ; (iedge < _westEdges.size()) ; ++iedge ) {
          _westEdges[iedge]->_setTarget( dest );
          dest->_westEdges.push_back( _westEdges[iedge] );
        }
        _westEdges.resize( iclear );
      } else {
        if (iclear > _westEdges.size())
          cerr << Error("GCell::_moveEdges(): On west side, iclear=%u is greater than size()-1=%u\n"
                        "        (%s)"
                       , iclear
                       , _westEdges.size()
                       , getString(this).c_str()
                       ) << endl;
      }
    }

    if (flags.contains(Flags::EastSide) and not _eastEdges.empty()) {
      cdebug_log(110,0) << "East side." << endl;

      if (iclear < _eastEdges.size()) {
        for ( size_t iedge=ibegin ; (iedge < _eastEdges.size()) ; ++iedge ) {
          _eastEdges[iedge]->_setSource( dest );
          dest->_eastEdges.push_back( _eastEdges[iedge] );
        }
        _eastEdges.resize( iclear );
      } else {
        if (iclear > _eastEdges.size())
           cerr << Error("GCell::_moveEdges(): On east side, iclear=%u is greater than size()-1=%u\n"
                        "        (%s)"
                        , iclear
                        , _eastEdges.size()
                        , getString(this).c_str()
                        ) << endl;
      }
    }

    cdebug_tabw(110,-1);
  }


  void GCell::setXY ( DbU::Unit x, DbU::Unit y )
  {
    UpdateSession::open();
    _xmin = x;
    _ymin = y;
    UpdateSession::close(); 
  }

  
  void GCell::updateContactsPosition ()
  {
    UpdateSession::open();
    DbU::Unit xc = (getXMax() + getXMin())/2;
    DbU::Unit yc = (getYMax() + getYMin())/2;
    for (vector<Contact*>::iterator it = _gcontacts.begin(); it != _gcontacts.end(); it++){
      for ( Component* c : (*it)->getSlaveComponents() ){
        Horizontal* h = dynamic_cast<Horizontal*>(c);
        Vertical*   v = dynamic_cast<Vertical*>  (c);
        if (h){
        //if (h->getY() == (*it)->getY()) h->setY(yc);
          h->setY(yc);
        } else if (v) {
        //if (v->getX() == (*it)->getX()) v->setX(xc);
          v->setX(xc);
        }
      }
      (*it)->setX(xc);
      (*it)->setY(yc);
    }
    UpdateSession::close(); 
  }


  Contact* GCell::getGContact ( Net* net )
  {
    for ( Contact* contact : _gcontacts ) {
      if (contact->getNet() == net) {
        cdebug_log(111,0) << "GCell::getGContact(): " << contact << endl;
        return contact;
      }
    }

    Point    center  = getBoundingBox().getCenter();
    Contact* contact = Contact::create( net
                                      , _anabatic->getConfiguration()->getGContactLayer()
                                      , center.getX()
                                      , center.getY()
                                      , DbU::fromLambda(2.0)
                                      , DbU::fromLambda(2.0)
                                      );
    _gcontacts.push_back( contact );
    cdebug_log(111,0) << "GCell::getGContact(): " << contact << endl;
    return contact;
  }


  bool  GCell::unrefContact ( Contact* unref )
  {
    if (_gcontacts.empty()) return false;

    cdebug_log(112,0) << "GCell::unrefContact(): " << unref << endl;

    for ( size_t i=0 ; i< _gcontacts.size() ; ++i ) {
      if (_gcontacts[i] == unref) {
        if (_gcontacts[i]->getSlaveComponents().getLocator()->isValid()) return false;
        
        cdebug_log(112,0) << "  Effective destroy." << endl;
        std::swap( _gcontacts[i], _gcontacts[_gcontacts.size()-1] );
        _gcontacts[ _gcontacts.size()-1 ]->destroy();
        _gcontacts.pop_back();

        return true;
      }
    }

    return false;
  }


  void  GCell::cleanupGlobal ()
  {
    for ( size_t i=0 ; i<_gcontacts.size() ; ) {
      if (not _gcontacts[i]->getSlaveComponents().getLocator()->isValid()) {
        std::swap( _gcontacts[i], _gcontacts[_gcontacts.size()-1] );
        _gcontacts[ _gcontacts.size()-1 ]->destroy();
        _gcontacts.pop_back();
      } else
        ++i;
    }
  }


  const Name& GCell::getName () const
  { return _extensionName; }


  Box  GCell::getBoundingBox () const
  {
    return Box( getXMin(), getYMin(), getXMax(1), getYMax(1) );
  }


  void  GCell::translate ( const DbU::Unit&, const DbU::Unit& )
  {
    cerr << Error( "GCell::translate(): On %s,\n"
                   "        Must never be called on a GCell object (ignored)."
                 , getString(this).c_str()
                 ) << endl;
  }


  bool  GCell::isNorth ( GCell* c ) const
  {
    bool found = false;
    for (vector<Edge*>::const_iterator it = _northEdges.begin(); it != _northEdges.end(); it++){
      if ( (*it)->getOpposite(this)->getId() == c->getId() ) {
        found = true;
        break;
      }
    }
    return found;
  }


  bool  GCell::isSouth ( GCell* c ) const
  {
    bool found = false;
    for (vector<Edge*>::const_iterator it = _southEdges.begin(); it != _southEdges.end(); it++){
      if ( (*it)->getOpposite(this)->getId() == c->getId() ) {
        found = true;
        break;
      }
    }
    return found;
  }


  bool  GCell::isEast ( GCell* c ) const
  {
    bool found = false;
    for (vector<Edge*>::const_iterator it = _eastEdges.begin(); it != _eastEdges.end(); it++){
      if ( (*it)->getOpposite(this)->getId() == c->getId() ) {
        found = true;
        break;
      }
    }
    return found;
  }

  
  bool  GCell::isWest ( GCell* c ) const
  {
    bool found = false;
    for (vector<Edge*>::const_iterator it = _westEdges.begin(); it != _westEdges.end(); it++){
      if ( (*it)->getOpposite(this)->getId() == c->getId() ) {
        found = true;
        break;
      }
    }
    return found;
  }


  bool  GCell::isSaturated ( size_t depth ) const
  { return getDensity(depth) > Session::getSaturateRatio(); }


  Interval  GCell::getSide ( unsigned int direction ) const
  {
    if (direction & Flags::Vertical) return Interval( getYMin(), getYMax() );
    return Interval( getXMin(), getXMax() );
  }


  AutoSegments  GCell::getHStartSegments ()
  { return new AutoSegments_AnchorOnGCell (this,Flags::EastSide); }


  AutoSegments  GCell::getVStartSegments ()
  { return new AutoSegments_AnchorOnGCell (this,Flags::NorthSide); }


  AutoSegments  GCell::getHStopSegments ()
  { return new AutoSegments_AnchorOnGCell (this,Flags::WestSide); }


  AutoSegments  GCell::getVStopSegments ()
  { return new AutoSegments_AnchorOnGCell (this,Flags::SouthSide); }


  size_t  GCell::getRoutingPads ( set<RoutingPad*>& rps )
  {
    for ( size_t i=0 ; i<_contacts.size() ; ++i ) {
      RoutingPad* rp = dynamic_cast<RoutingPad*>(_contacts[i]->getAnchor());
      if ( rp ) {
        rps.insert ( rp );
      }
    }
    return rps.size();
  }


  float  GCell::getHCapacity () const
  {
    int capacity = 0;
    if (not _eastEdges.empty()) {
      for ( Edge* edge : _eastEdges ) capacity += edge->getCapacity(); 
    } else {
      for ( Edge* edge : _westEdges ) capacity += edge->getCapacity(); 
    }
    return (float)capacity;
  }


  float  GCell::getVCapacity () const
  {
    int capacity = 0;
    if (not _northEdges.empty()) {
      for ( Edge* edge : _northEdges ) capacity += edge->getCapacity(); 
    } else {
      for ( Edge* edge : _southEdges ) capacity += edge->getCapacity(); 
    }
    return (float)capacity;
  }


  float  GCell::getAverageHVDensity () const
  {
  // Average density of all layers mixeds together.
    float density = 0.0;
    for ( size_t i=0 ; i<_depth ; i++ )
      density += _densities[i];
    return density / ((float)(_depth-_pinDepth));
  }


  float  GCell::getMaxHVDensity () const
  {
  // Maximum density between all horizontal vs. all vertical layers.
    size_t hplanes  = 0;
    size_t vplanes  = 0;
    float  hdensity = 0.0;
    float  vdensity = 0.0;

    for ( size_t i=_pinDepth ; i<_depth ; i++ ) {
      if ( i%2 ) { hdensity += _densities[i]; ++hplanes; }
      else       { vdensity += _densities[i]; ++vplanes; }
    }

    if (hplanes) hdensity /= hplanes;
    if (vplanes) vdensity /= vplanes;

    return std::max(hdensity, vdensity);
  }


  float  GCell::getDensity ( unsigned int flags ) const
  {
    if (isInvalidated() and not(flags & Flags::NoUpdate))  const_cast<GCell*>(this)->updateDensity();

    float density = 0.0;

    if (getAnabatic()->getDensityMode() == AverageHVDensity) {
      density = getAverageHVDensity();
    } else if (getAnabatic()->getDensityMode() == MaxHVDensity) {
      density = getMaxHVDensity();
    } else if (getAnabatic()->getDensityMode() == AverageHDensity) {
      size_t hplanes  = 0;
      float  hdensity = 0.0;

      for ( size_t i=_pinDepth ; i<_depth ; i++ ) {
        if (i%2) { hdensity += _densities[i]; ++hplanes; }
      }
      if (hplanes) hdensity /= hplanes;

      density = hdensity;
    } else if (getAnabatic()->getDensityMode() == AverageVDensity) {
      size_t vplanes  = 0;
      float  vdensity = 0.0;

      for ( size_t i=_pinDepth ; i<_depth ; i++ ) {
        if (i%2 == 0) { vdensity += _densities[i]; ++vplanes; }
      }

      if (vplanes) vdensity /= vplanes;

      density = vdensity;
    } else if (getAnabatic()->getDensityMode() == MaxDensity) {
      for ( size_t i=_pinDepth ; i<_depth ; i++ ) {
        if (_densities[i] > density) density = _densities[i];
      }
    } else if (getAnabatic()->getDensityMode() == MaxHDensity) {
      for ( size_t i=_pinDepth ; i<_depth ; i++ ) {
        if ((i%2) and (_densities[i] > density)) density = _densities[i];
      }
    } else if (getAnabatic()->getDensityMode() == MaxVDensity) {
      for ( size_t i=_pinDepth ; i<_depth ; i++ ) {
        if ((i%2 == 0) and (_densities[i] > density)) density = _densities[i];
      }
    }

    return density;
  }


  void  GCell::addBlockage ( size_t depth, DbU::Unit length )
  {
    if (depth >= _depth) return;

    _blockages[depth] += length;
    _flags |= Flags::Invalidated;

    cdebug_log(149,0) << "GCell:addBlockage() " << this << " "
                << depth << ":" << DbU::getValueString(_blockages[depth]) << endl;
  }


  void  GCell::removeContact ( AutoContact* ac )
  {
    size_t begin = 0;
    size_t end   = _contacts.size();
    bool   found = false;

    for ( ; not found and (begin < end) ; begin++ ) {
      if ( _contacts[begin] == ac ) {
        _contacts[begin] = _contacts[end-1];
        found = true;
      }
    }

    if (found) {
      cdebug_log(149,0) << "remove " << ac << " from " << this << endl;
      _contacts.pop_back();
    } else {
      cerr << Bug("%p:%s do not belong to %s."
                 ,ac->base(),getString(ac).c_str(),_getString().c_str()) << endl;
    }
  }


  void  GCell::removeHSegment ( AutoSegment* segment )
  {
    size_t end   = _hsegments.size();
    size_t begin = 0;

    for ( ; begin < end ; begin++ ) {
      if (_hsegments[begin] == segment) std::swap( _hsegments[begin], _hsegments[--end] );
    }

    if (_hsegments.size() == end) {
      cerr << Bug( "%s do not go through %s."
                 , getString(segment).c_str(), _getString().c_str() ) << endl;
      return;
    }

    if (_hsegments.size() - end > 1)
      cerr << Bug( "%s has multiple occurrences of %s."
                 , _getString().c_str(), getString(segment).c_str() ) << endl;

    _hsegments.erase( _hsegments.begin() + end, _hsegments.end() );
  }


  void  GCell::removeVSegment ( AutoSegment* segment )
  {
    size_t end   = _vsegments.size();
    size_t begin = 0;

    for ( ; begin < end ; begin++ ) {
      if (_vsegments[begin] == segment) std::swap( _vsegments[begin], _vsegments[--end] );
    }

    if (_vsegments.size() == end) {
      cerr << Bug( "%s do not go through %s."
                 , getString(segment).c_str()
                 , _getString().c_str() ) << endl;
      return;
    }

    if (_vsegments.size() - end > 1)
      cerr << Bug( "%s has multiple occurrences of %s."
                 , _getString().c_str()
                 , getString(segment).c_str() ) << endl;

    _vsegments.erase( _vsegments.begin() + end, _vsegments.end() );
  }


  void  GCell::updateContacts ()
  { for ( AutoContact* contact : _contacts ) contact->updateGeometry(); }


  size_t  GCell::updateDensity ()
  {
    if (not isInvalidated()) return (isSaturated()) ? 1 : 0;

    _flags.reset( Flags::Saturated );

    for ( size_t i=0 ; i<_vsegments.size() ; i++ ) {
      if ( _vsegments[i] == NULL )
        cerr << "NULL Autosegment at index " << i << endl;
    }

    sort( _hsegments.begin(), _hsegments.end(), AutoSegment::CompareByDepthLength() );
    sort( _vsegments.begin(), _vsegments.end(), AutoSegment::CompareByDepthLength() );

    float                    hcapacity    = getHCapacity ();
    float                    vcapacity    = getVCapacity ();
    float                    ccapacity    = hcapacity * vcapacity * 4; 
    DbU::Unit                width        = getXMax() - getXMin();
    DbU::Unit                height       = getYMax() - getYMin();
    DbU::Unit                hpenalty     = 0 /*_box.getWidth () / 3*/;
    DbU::Unit                vpenalty     = 0 /*_box.getHeight() / 3*/;
    DbU::Unit                uLengths1    [ _depth ];
    DbU::Unit                uLengths2    [ _depth ];
    float                    localCounts  [ _depth ];
    vector<UsedFragments>    ufragments   ( _depth );

    for ( size_t i=0 ; i<_depth ; i++ ) {
      ufragments[i].setPitch ( Session::getPitch(i) );
      _feedthroughs[i] = 0.0;
      uLengths2    [i] = 0;
      localCounts  [i] = 0.0;
      _globalsCount[i] = 0.0;

      if (Session::getDirection(i) & Flags::Horizontal) {
        ufragments[i].setSpan    ( getXMin(), getXMax() );
        ufragments[i].setCapacity( (size_t)hcapacity );
      } else {
        ufragments[i].setSpan    ( getYMin(), getYMax() );
        ufragments[i].setCapacity( (size_t)vcapacity );
      }
    }

  // Compute wirelength associated to contacts (in DbU::Unit converted to float).
    AutoSegment::DepthLengthSet  processeds;
    for ( AutoContact* contact : _contacts ) {
      for ( size_t i=0 ; i<_depth ; i++ ) uLengths1[i] = 0;
      contact->getLengths ( uLengths1, processeds );
      for ( size_t i=0 ; i<_depth ; i++ ) {
        if (Session::getDirection(i) & Flags::Horizontal)
          uLengths2[i] += uLengths1[i]+hpenalty;
        else
          uLengths2[i] += uLengths1[i]+vpenalty; break;
      }
    }

  // Add the "pass through" horizontal segments.
    if ( _hsegments.size() ) {
      const Layer* layer = _hsegments[0]->getLayer();
      size_t       depth = Session::getRoutingGauge()->getLayerDepth(layer);
      size_t       count = 0;
      for ( size_t i=0 ; i<_hsegments.size() ; i++ ) {
        _globalsCount[depth] += 1.0;
        ufragments[depth].incGlobals();

        if ( layer != _hsegments[i]->getLayer() ) {
          uLengths2[depth] += count * width;

          count = 0;
          layer = _hsegments[i]->getLayer();
          depth = Session::getRoutingGauge()->getLayerDepth(layer);
        }
        count++;
        _feedthroughs[depth] += 1.0;
      }
      if ( count ) {
        uLengths2[depth] += count * width;
      }
    }

  // Add the "pass through" vertical segments.
    if ( _vsegments.size() ) {
      const Layer* layer = _vsegments[0]->getLayer();
      size_t       depth = Session::getRoutingGauge()->getLayerDepth(layer);
      size_t       count = 0;
      for ( size_t i=0 ; i<_vsegments.size() ; i++ ) {
        _globalsCount[depth] += 1.0;
        ufragments[depth].incGlobals();

        if ( layer != _vsegments[i]->getLayer() ) {
          uLengths2[depth] += count * height;

          count = 0;
          layer = _vsegments[i]->getLayer();
          depth = Session::getRoutingGauge()->getLayerDepth(layer);
        }
        count++;
        _feedthroughs[depth] += 1.0;
      }
      if ( count ) {
        uLengths2[depth] += count * height;
      }
    }

  // Add the blockages.
    for ( size_t i=0 ; i<_depth ; i++ ) {
      uLengths2[i] += _blockages[i];
    }

  // Compute the number of non pass-through tracks.
    if (processeds.size()) {
      AutoSegment::DepthLengthSet::iterator isegment = processeds.begin();
      const Layer* layer = (*isegment)->getLayer();
      DbU::Unit    axis  = (*isegment)->getAxis();
      size_t       depth = Session::getRoutingGauge()->getLayerDepth(layer);
      size_t       count = 0;
      for ( ; isegment != processeds.end(); ++isegment ) {
        _feedthroughs[depth] += ((*isegment)->isGlobal()) ? 0.50 : 0.33;
        localCounts  [depth] += 1.0;
        if ( (*isegment)->isGlobal() ) _globalsCount[depth] += 1.0;

        ufragments[depth].merge( (*isegment)->getAxis(), (*isegment)->getSpanU() );
        if ( (axis != (*isegment)->getAxis()) or (layer != (*isegment)->getLayer()) ) {
          count = 0;
          axis  = (*isegment)->getAxis();
          layer = (*isegment)->getLayer();
          depth = Session::getRoutingGauge()->getLayerDepth(layer);
        }
        ++count;
      }
    }

  // Normalize: 0 < d < 1.0 (divide by H/V capacity).
    for ( size_t i=0 ; i<_depth ; i++ ) {
      if (Session::getDirection(i) & Flags::Horizontal) {
        if (width) {
          _densities     [i]  = ((float)uLengths2[i]) / ( hcapacity * (float)width );
          _feedthroughs  [i] += (float)(_blockages[i] / width);
          _fragmentations[i]  = (float)ufragments[i].getMaxFree().getSize() / (float)width;
        } else {
          _densities     [i]  = 0;
          _feedthroughs  [i]  = 0;
          _fragmentations[i]  = 0;
        }
      } else {
        if (height) {
          _densities     [i]  = ((float)uLengths2[i]) / ( vcapacity * (float)height );
          _feedthroughs  [i] += (float)(_blockages[i] / height);
          _fragmentations[i]  = (float)ufragments[i].getMaxFree().getSize() / (float)height;
        } else {
          _densities     [i]  = 0;
          _feedthroughs  [i]  = 0;
          _fragmentations[i]  = 0;
        }
      }

      if (_densities[i] >= 1.0) _flags |= Flags::Saturated;
    }

    if (ccapacity)
      _cDensity = ( (float)_contacts.size() ) / ccapacity;
    else
      _cDensity = 0;
    _flags.reset( Flags::Invalidated );

    checkDensity();

    return isSaturated() ? 1 : 0 ;
  }


  void  GCell::truncDensities ()
  {
    int  hcapacity = (int)getHCapacity();
    int  vcapacity = (int)getVCapacity();
    Box  bBox      = getBoundingBox();

    for ( size_t i=0 ; i<_depth ; i++ ) {
      if (Session::getDirection(i) & Flags::Horizontal) {
        if (_blockages[i] > hcapacity * bBox.getWidth())
          _blockages[i]  = hcapacity * bBox.getWidth();
      } else {
        if (_blockages[i] > vcapacity * bBox.getHeight())
          _blockages[i]  = vcapacity * bBox.getHeight();
      }
    }
    _flags &= ~Flags::Saturated;
  }


  size_t  GCell::checkDensity () const
  {
    if (isInvalidated()) const_cast<GCell*>(this)->updateDensity();

    if ( not Session::isInDemoMode() and Session::doWarnGCellOverload() ) {
      for ( size_t i=0 ; i<_depth ; i++ ) {
        if (_densities[i] > 1.0) {
          cparanoid << Warning( "%s overloaded in %s (M2:%.2f M3:%.2f M4:%.2f M5:%.2f)"
                              , _getString().c_str()
                              , getString(Session::getRoutingGauge()->getRoutingLayer(i)->getName()).c_str()
                              , _densities[1]  // M2
                              , _densities[2]  // M3
                            //, _blockages[2]  // M4
                              , _densities[3]  // M5
                              , _densities[4]  // M6
                              )
             << endl;
        }
      }
    }

    return isSaturated() ? 1 : 0 ;
  }


  bool  GCell::hasFreeTrack ( size_t depth, float reserve ) const
  {
    if (isInvalidated()) const_cast<GCell*>(this)->updateDensity();

    float capacity = 0.0;
    if (Session::getDirection(depth) & Flags::Horizontal) capacity = getHCapacity();
    else                                                  capacity = getVCapacity();

    cdebug_log(149,0) << "  | hasFreeTrack [" << getId() << "] depth:" << depth << " "
                << Session::getRoutingGauge()->getRoutingLayer(depth)->getName()
              //<< " " << (_densities[depth]*capacity) << " vs. " << capacity
                << " " << _feedthroughs[depth] << " vs. " << capacity
                << " " << this << endl;
    
    return (_feedthroughs[depth] + 0.99 + reserve <= capacity);
  }


  void  GCell::rpDesaturate ( set<Net*>& globalNets )
  {
    set<RoutingPad*> rps;
    getRoutingPads( rps );

    set<Net*> rpNets;
    for ( RoutingPad* rp : rps ) {
      if (rp->getLayer() != Session::getRoutingLayer(0)) continue;
      rpNets.insert( rp->getNet() );
    }

    if (rpNets.size() < Session::getSaturateRp()) return;

    cerr << Warning("%s has %zd terminals (h:%zd, v:%zd)"
                   ,getString(this).c_str()
                   ,rps.size()
                   ,_hsegments.size()
                   ,_vsegments.size()
                   ) << endl;

    AutoSegment* segment;
    while ( (_densities[1] > 0.5) and stepDesaturate(1,globalNets,segment,Flags::ForceMove) ) {
      cdebug_log(149,0) << "Moved up: " << segment << endl;
    }
  }


  bool  GCell::stepDesaturate ( size_t        depth
                              , set<Net*>&    globalNets
                              , AutoSegment*& moved
                              , unsigned int  flags
                              )
  {
    cdebug_log(9000,0) << "Deter| GCell::stepDesaturate() [" << getId() << "] depth:" << depth << endl;

    updateDensity();
    moved = NULL;

    if (not (flags & Flags::ForceMove) and not isSaturated(depth)) return false;

    vector<AutoSegment*>::iterator isegment;
    vector<AutoSegment*>::iterator iend;

    if (Session::getDirection(depth) & Flags::Horizontal) {
      iend     = _hsegments.end  ();
      isegment = _hsegments.begin();
    } else {
      iend     = _vsegments.end  ();
      isegment = _vsegments.begin();
    }

    for ( ; (isegment != iend) ; isegment++ ) {
      unsigned int segmentDepth = Session::getRoutingGauge()->getLayerDepth((*isegment)->getLayer());

      if (segmentDepth < depth) continue;
      if (segmentDepth > depth) break;

      globalNets.insert( (*isegment)->getNet() );
      cdebug_log(9000,0) << "Deter| Move up " << (*isegment) << endl;

      moved = (*isegment);

      if (moved) return true;
    }

    return false;
  }


  bool  GCell::stepBalance ( size_t depth, GCell::Set& invalidateds )
  {
    cdebug_log(149,0) << "stepBalance() - " << this << endl;

    updateDensity();

    vector<AutoSegment*>::iterator isegment;
    vector<AutoSegment*>::iterator iend;
    set<Net*>                      globalNets;

    if (Session::getDirection(depth) & Flags::Horizontal) {
      iend     = _hsegments.end  ();
      isegment = _hsegments.begin();
    } else {
      iend     = _vsegments.end  ();
      isegment = _vsegments.begin();
    }

    for ( ; (isegment != iend) ; isegment++ ) {
      unsigned int segmentDepth = Session::getRoutingGauge()->getLayerDepth((*isegment)->getLayer());

      if (segmentDepth < depth) continue;
      if (segmentDepth > depth) break;

#if THIS_IS_DISABLED
    // Hard-coded: reserve 3 tracks (1/20 * 3).
      if ((*isegment)->canMoveULeft(0.05)) {
        getAnabatic()->moveULeft(*isegment,globalNets,invalidateds);
        return true;
      }
      if ((*isegment)->canMoveURight(0.05)) {
        getAnabatic()->moveURight(*isegment,globalNets,invalidateds);
        return true;
      }
#endif
    }

    return false;
  }


  bool  GCell::stepNetDesaturate ( size_t depth, set<Net*>& globalNets, GCell::Set& invalidateds )
  {
    cdebug_log(9000,0) << "Deter| GCell::stepNetDesaturate() depth:" << depth << endl;
    cdebug_log(9000,0) << "Deter| " << this << endl;

    updateDensity();

    vector<AutoSegment*>::iterator isegment;
    vector<AutoSegment*>::iterator iend;

    if (Session::getDirection(depth) & Flags::Horizontal) {
      iend     = _hsegments.end   ();
      isegment = _hsegments.begin ();
    } else {
      iend     = _vsegments.end   ();
      isegment = _vsegments.begin ();
    }

    for ( ; (isegment != iend) ; isegment++ ) {
      unsigned int segmentDepth = Session::getRoutingGauge()->getLayerDepth((*isegment)->getLayer());

      if (segmentDepth < depth) continue;
      if (segmentDepth > depth) break;

      cdebug_log(9000,0) << "Deter| Move up " << (*isegment) << endl;

      if (getAnabatic()->moveUpNetTrunk(*isegment,globalNets,invalidateds))
        return true;
    }

    return false;
  }

  string  GCell::_getTypeName () const
  { return getString(_extensionName); }


  string  GCell::_getString () const
  {
    string s = Super::_getString();
    s.insert( s.size()-1, " "+getString(getBoundingBox()) );
    s.insert( s.size()-1, " "+getString(_flags) );
    return s;
  }


  Record* GCell::_getRecord () const
  {
    Record* record = Super::_getRecord();
    record->add( getSlot("_flags"      , &_flags     ) );
    record->add( getSlot("_westEdges"  , &_westEdges ) );
    record->add( getSlot("_eastEdges"  , &_eastEdges ) );
    record->add( getSlot("_southEdges" , &_southEdges) );
    record->add( getSlot("_northEdges" , &_northEdges) );
    record->add( DbU::getValueSlot("_xmin", &_xmin) );
    record->add( DbU::getValueSlot("_ymin", &_ymin) );
    record->add( getSlot ( "_vsegments", &_vsegments ) );
    record->add( getSlot ( "_hsegments", &_hsegments ) );
    record->add( getSlot ( "_contacts" , &_contacts  ) );
    record->add( getSlot ( "_depth"    , &_depth     ) );

    RoutingGauge* rg = getAnabatic()->getConfiguration()->getRoutingGauge();

    for ( size_t depth=0 ; depth<_depth ; ++depth ) {
      ostringstream s;
      const Layer* layer = rg->getRoutingLayer(depth)->getBlockageLayer();
      s << "_blockages[" << depth << ":" << ((layer) ? layer->getName() : "None") << "]";
      record->add( getSlot ( s.str(),  &_blockages[depth] ) );
    }

    for ( size_t depth=0 ; depth<_depth ; ++depth ) {
      ostringstream s;
      const Layer* layer = rg->getRoutingLayer(depth);
      s << "_densities[" << depth << ":" << ((layer) ? layer->getName() : "None") << "]";
      record->add( getSlot ( s.str(),  &_densities[depth] ) );
    }
    return record;
  }


// -------------------------------------------------------------------
// Class  :  "Anabatic::GCellDensitySet".


  GCellDensitySet::GCellDensitySet ( size_t depth )
    : _depth   (depth)
    , _set     ()
    , _requests()
  { }


  GCellDensitySet::GCellDensitySet ( size_t depth, const GCell::Vector& gcells )
    : _depth   (depth)
    , _set     ()
    , _requests()
  {
    for ( size_t i=0 ; i<gcells.size() ; i++ )
      _requests.insert( gcells[i] );
    requeue();
  }


  GCellDensitySet::~GCellDensitySet ()
  {
    if (not _requests.empty()) {
      cerr << Warning("~GCellDensitySet(): Still contains %d requests (and %d elements)."
                     ,_requests.size(),_set.size()) << endl;
    }
  }


  void  GCellDensitySet::requeue ()
  {
    cdebug_log(149,0) << "GCellDensitySet::requeue()" << endl;

    std::set<GCell*,GCell::CompareByKey>::iterator iinserted;
    GCell::Set::iterator                           igcell    = _requests.begin();

  // Remove invalidateds GCell from the queue.
    for ( ; igcell != _requests.end() ; ++igcell ) {
      iinserted = _set.find(*igcell);
      if (iinserted != _set.end()) {
        _set.erase( iinserted );
      }
    }

  // Re-insert invalidateds GCell in the queue *after* updating the key.
    for ( igcell = _requests.begin() ; igcell != _requests.end() ; ++igcell ) {
      (*igcell)->updateKey( _depth );
      _set.insert( *igcell );
    }

    _requests.clear();
  }


// -------------------------------------------------------------------
// Utilities.


  string  getVectorString ( float* v, size_t size )
  {
    ostringstream s;

    s << setprecision(3);
    for ( size_t i=0 ; i<size ; i++ ) {
      if ( !i ) s << "[";
      else      s << " ";
      s << v[i];
    }
    s << "]";

    return s.str();
  }


  bool  isLess ( const GCell* lhs, const GCell* rhs, Flags direction )
  {
    if (direction & Flags::Horizontal) {
      if (lhs->getXMin() != rhs->getXMin()) return lhs->getXMin() < rhs->getXMin();
    } else {
      if (direction & Flags::Vertical) {
        if (lhs->getYMin() != rhs->getYMin()) return lhs->getYMin() < rhs->getYMin();
      }
    }
    return lhs->getId() < rhs->getId();
  }


  bool  isGreater ( const GCell* lhs, const GCell* rhs, Flags direction )
  {
    if (direction & Flags::Horizontal) {
      if (lhs->getXMin() != rhs->getXMin()) return lhs->getXMin() > rhs->getXMin();
    } else {
      if (direction & Flags::Vertical) {
        if (lhs->getYMin() != rhs->getYMin()) return lhs->getYMin() > rhs->getYMin();
      }
    }
    return lhs->getId() > rhs->getId();
  }


}  // Anabatic namespace.
