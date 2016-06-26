// -*- mode: C++; explicit-buffer-name: "Dijkstra.cpp<anabatic>" -*-
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
// |  C++ Module  :  "./anabatic/Dijkstra.cpp"                       |
// +-----------------------------------------------------------------+


#include <limits>
#include "hurricane/Error.h"
#include "hurricane/Net.h"
#include "hurricane/RoutingPad.h"
#include "hurricane/Horizontal.h"
#include "hurricane/Vertical.h"
#include "hurricane/UpdateSession.h"
#include "hurricane/DebugSession.h"
#include "crlcore/Utilities.h"
#include "anabatic/AnabaticEngine.h"
#include "anabatic/Dijkstra.h"


namespace Anabatic {

  using std::cerr;
  using std::endl;
  using std::numeric_limits;
  using Hurricane::ForEachIterator;
  using Hurricane::Error;
  using Hurricane::Component;
  using Hurricane::Segment;
  using Hurricane::Horizontal;
  using Hurricane::Vertical;
  using Hurricane::RoutingPad;
  using Hurricane::UpdateSession;
  using Hurricane::DebugSession;


// -------------------------------------------------------------------
// Class  :  "Anabatic::Vertex".


  DbU::Unit  Vertex::unreached = std::numeric_limits<long>::max();


  bool  Vertex::hasValidStamp () const
  { return _stamp == getAnabatic()->getStamp(); }


  string  Vertex::_getString () const
  {
    if (not _gcell) {
      string s = "<Vertex [key] " + getString(_id) + ">";
      return s;
    }

    string s = "<Vertex " + getString(_id)
             + " @(" + DbU::getValueString(_gcell->getXMin())
             +  "," + DbU::getValueString(_gcell->getYMin()) + ")"
             + " connexId:" + ((_connexId >= 0) ? getString(_connexId) : "None")
             + " d:" + ((_distance == unreached) ? "unreached" : DbU::getValueString(_distance) )
             +   "+" + getString(_branchId)
             + " stamp:" + (hasValidStamp() ? "valid" : "outdated")
             + " from:" + ((_from) ? "set" : "NULL")
             + ">";
    return s;
  }


  void  Vertex::notify ( Vertex* vertex, unsigned int flags )
  {
  //Vertex* vertex = getOwner();
    cdebug_log(111,0) << "Vertex::notify() " << vertex << endl;
  // Take into account the GCell modification here.
  }


// -------------------------------------------------------------------
// Class  :  "Anabatic::Dijkstra".


  Dijkstra::Mode::~Mode ()
  { }


  string  Dijkstra::Mode::_getTypeName () const
  { return "Anabatic::Dijkstra::Mode"; }


  string Dijkstra::Mode::_getString () const
  {
    string s = "";
    s += (_flags & Standart ) ? 'S' : '-';
    s += (_flags & Monotonic) ? 'M' : '-';

    return s;
  }


  DbU::Unit  Dijkstra::_distance ( const Vertex* a, const Vertex* b, const Edge* e )
  {
    DbU::Unit distance = a->getDistance() + e->getDistance();

    // Edge* aFrom = a->getFrom();
    // if (aFrom) {
    //   distance += (aFrom->isHorizontal() xor e->isHorizontal()) ? 3.0 : 0.0;
    // }
    return distance;
  }


  Dijkstra::Dijkstra ( AnabaticEngine* anabatic )
    : _anabatic    (anabatic)
    , _vertexes    ()
    , _distanceCb  (_distance)
    , _mode        (Mode::Standart)
    , _net         (NULL)
    , _stamp       (-1)
    , _sources     ()
    , _targets     ()
    , _searchArea  ()
    , _connectedsId(-1)
    , _queue       ()
  {
    const vector<GCell*>& gcells = _anabatic->getGCells();
    for ( GCell* gcell : gcells ) {
      _vertexes.push_back( new Vertex (gcell) );
    }
  }


  Dijkstra::~Dijkstra ()
  {
    for ( Vertex* vertex : _vertexes ) delete vertex;
  }


  void  Dijkstra::load ( Net* net )
  {
    _cleanup();

    const Layer* gcontactLayer = _anabatic->getConfiguration()->getGContactLayer();

    _net   = net;
    _stamp = _anabatic->incStamp();

    DebugSession::open( _net, 112, 120 );
    cdebug_log(112,1) << "Dijkstra::load() " << _net << endl;

    vector< std::pair<Component*,bool> > components;
    for ( Component* component : _net->getComponents() ) {
      RoutingPad* rp = dynamic_cast<RoutingPad*>( component );
      if (rp) { components.push_back( make_pair(rp,true) ); continue; }

      Contact* gcontact = dynamic_cast<Contact*>( component );
      if (gcontact and (gcontact->getLayer() == gcontactLayer))
        components.push_back( make_pair(gcontact,false) );
    }

    for ( auto element : components ) {
      RoutingPad* rp       = NULL;
      Contact*    gcontact = NULL;
      Point       center;

      if (element.second) {
        rp     = static_cast<RoutingPad*>( element.first );
        center = rp->getBoundingBox().getCenter();
      } else {
        gcontact = static_cast<Contact*>( element.first );
        center   = gcontact->getCenter();
      }

      GCell* gcell = _anabatic->getGCellUnder( center );
        
      if (not gcell) {
        cerr << Error( "Dijkstra::load(): %s of %s is not under any GCell.\n"
                       "        It will be ignored so the routing may be incomplete."
                     , getString(element.first).c_str()
                     , getString(_net).c_str()
                     ) << endl;
        continue;
      }

      _searchArea.merge( gcell->getBoundingBox() );

      Vertex* vertex = gcell->getObserver<Vertex>(GCell::Observable::Vertex);
      if (vertex->getConnexId() < 0) {
        vertex->setDistance( Vertex::unreached );
        vertex->setStamp   ( _stamp );
        vertex->setConnexId( _connectedsId );
        vertex->setBranchId( 0 );
        vertex->setFrom    ( NULL );
        _targets.insert( vertex );
        cdebug_log(112,0) << "Add Vertex: " << vertex << endl;
      }

      if (gcontact) {
        for ( Component* slave : gcontact->getSlaveComponents() ) {
          Flags  sideHint      = Flags::NoFlags;
          GCell* oppositeGCell = NULL;

          Segment* segment = dynamic_cast<Horizontal*>(slave);
          if (segment) {
            cdebug_log(112,0) << "| " << segment << endl;
            if (segment->getSource() == gcontact) {
              oppositeGCell = _anabatic->getGCellUnder( segment->getTarget()->getCenter() );
              sideHint = Flags::EastSide;
            } else
              if (segment->getTarget() == gcontact) {
                cdebug_log(112,0) << "  Connected by target, skipped." << endl;
                continue;
              }
          } else {
            segment = dynamic_cast<Vertical*>(slave);
            if (segment) {
              cdebug_log(112,0) << "| " << segment << endl;
              if (segment->getSource() == gcontact) {
                oppositeGCell = _anabatic->getGCellUnder( segment->getTarget()->getCenter() );
                sideHint = Flags::NorthSide;
              } else
                if (segment->getTarget() == gcontact) {
                  cdebug_log(112,0) << "  Connected by target, skipped." << endl;
                  continue;
                }
            }
          }

          Edge* edge = gcell->getEdgeTo( oppositeGCell, sideHint );
          if (edge) {
            cdebug_log(112,0) << "+ Associated to edge." << endl;
            edge->setSegment( segment );
          } else
            cerr << Error( "Dijkstra::load(): Cannot bind segment to any edge:\n"
                           "  %s\n"
                           "  source:%s\n"
                           "  target:%s"
                         , getString(segment).c_str()
                         , getString(gcell).c_str()
                         , getString(oppositeGCell).c_str()
                         ) << endl;
        }
      }

      if (rp) {
        Contact* vcontact = vertex->getGContact( _net );
        rp->getBodyHook()->detach();
        rp->getBodyHook()->attach( vcontact->getBodyHook() );
      }
    }

    for ( Vertex* vertex : _targets ) {
      if (vertex->getConnexId() != 0) continue;
      _tagConnecteds( vertex, ++_connectedsId );
    }

    cdebug_log(112,0) << "Search area: " << _searchArea << endl;
    cdebug_tabw(112,-1);
    DebugSession::close();
  }


  void  Dijkstra::_selectFirstSource ()
  {
    if (_targets.empty()) {
#if 0
      cparanoid << Error( "Dijkstra::_selectFirstSource(): %s has no vertexes to route, ignored."
                        , getString(_net).c_str()
                        ) << endl;
#endif
      return;
    }

    Vertex* firstSource = NULL;

    if (_mode & Mode::Monotonic) {
      if (_targets.size() == 2) {
        auto ivertex = _targets.begin();
        Vertex* v1 = *ivertex;
        Vertex* v2 = *(++ivertex);

        firstSource = (v1->getCenter().getX() <= v2->getCenter().getY()) ? v1 : v2;
      } else {
        cerr << Error( "Dijkstra::_selectFirstSource(): %s cannot be routed in monotonic mode.\n"
                       "        Must have exactly two terminals (%u), revert to Standart."
                     , getString(_net).c_str()
                     , _targets.size()
                     ) << endl;
        _mode = Mode::Standart;
      }
    }

    if (not firstSource) {
    // Standart routing.
      Point      areaCenter  = _searchArea.getCenter();
      auto       ivertex     = _targets.begin();

      firstSource = *ivertex++;
      DbU::Unit  minDistance = areaCenter.manhattanDistance( firstSource->getCenter() );

      for ( ; ivertex != _targets.end() ; ++ivertex ) {
        DbU::Unit distance = areaCenter.manhattanDistance( (*ivertex)->getCenter() );
        if (distance < minDistance) {
          minDistance = distance;
          firstSource = *ivertex;
        }
      }
    }

    for ( auto ivertex = _targets.begin() ; ivertex != _targets.end() ; ) {
      auto inext = ivertex; inext++;

      if ((*ivertex)->getConnexId() == firstSource->getConnexId()) {
        _sources.insert( *ivertex );
        _targets.erase (  ivertex );
      }
 
      ivertex = inext;
    }

    cdebug_log(112,0) << "Dijkstra::_selectFirstSource() " << *_sources.begin() << endl;
  }


  void  Dijkstra::_cleanup ()
  {
    for ( Vertex* vertex : _sources ) if (vertex->getFrom()) vertex->getFrom()->setSegment( NULL );
    for ( Vertex* vertex : _targets ) if (vertex->getFrom()) vertex->getFrom()->setSegment( NULL );

  //_checkEdges();

    _sources.clear();
    _targets.clear();
    _searchArea.makeEmpty();
    _connectedsId = 0;
  }


  bool  Dijkstra::_propagate ( Flags enabledSides )
  {
    cdebug_log(112,1) << "Dijkstra::_propagate() " << _net << endl;

    while ( not _queue.empty() ) {
      _queue.dump();

      Vertex* current = _queue.top();
      _queue.pop();

      if ((current->getConnexId() == _connectedsId) or (current->getConnexId() < 0)) {
        for ( Edge* edge : current->getGCell()->getEdges() ) {

          if (edge == current->getFrom()) continue;
          
          GCell*  gneighbor = edge->getOpposite(current->getGCell());
          Vertex* vneighbor = gneighbor->getObserver<Vertex>(GCell::Observable::Vertex);

          if (not _searchArea.contains(vneighbor->getCenter())) continue;

          cdebug_log(111,0) << "| Edge " << edge << endl;
          cdebug_log(111,0) << "+ Neighbor: " << vneighbor << endl;

          DbU::Unit distance = _distanceCb( current, vneighbor, edge );

          if (vneighbor->getConnexId() == _connectedsId) continue;

          if (      (distance  < vneighbor->getDistance())
             or (   (distance == vneighbor->getDistance())
                and (current->getBranchId() > vneighbor->getBranchId())) ) {
            if (vneighbor->getDistance() != Vertex::unreached) {
              _queue.erase( vneighbor );
            } else {
              if (not vneighbor->hasValidStamp()) {
                vneighbor->setConnexId( -1 );
                vneighbor->setStamp   ( _stamp );
              }
            }

            vneighbor->setBranchId( current->getBranchId() );
            vneighbor->setDistance( distance );
            vneighbor->setFrom    ( edge );
            _queue.push( vneighbor );

            cdebug_log(111,0) << "Push: (size:" << _queue.size() << ") " << vneighbor << endl;
          }
        }

        continue;
      }

    // We did reach another target (different <connexId>).
    // Tag back the path, with a higher <branchId>.
      _traceback( current );
      cdebug_tabw(112,-1);
      return true;
    }

    cerr << Error( "Dijkstra::propagate(): %s has unreachable targets."
                 , getString(_net).c_str()
                 ) << endl;

    cdebug_tabw(112,-1);
    return false;
  }


  void  Dijkstra::_traceback ( Vertex* current )
  {
    cdebug_log(112,1) << "Dijkstra::_traceback() " << _net << " branchId:" << _sources.size() << endl;

    int branchId = _sources.size();
    _targets.erase( current );

    while ( current ) {
      cdebug_log(112,0) << "| " << current << endl;
      if (current->getConnexId() == _connectedsId) break;

      Edge* from = current->getFrom();
      if (not from) break;
      from->incRealOccupancy( 1 );

      _sources.insert( current );
      current->setDistance( 0.0 );
      current->setConnexId( _connectedsId );
      current->setBranchId( branchId );
      _queue.push( current );
      
      Vertex* source = current;
      Vertex* target = source->getPredecessor();
      current = target;

      if (  (source->getGCell()->getXMin() > target->getGCell()->getXMin())
         or (source->getGCell()->getYMin() > target->getGCell()->getYMin()) )
        std::swap( source, target );

      Contact* sourceContact = source->getGContact( _net );
      Contact* targetContact = target->getGContact( _net );

      Segment* segment = NULL;
      if (from->isHorizontal()) {
        segment = Horizontal::create( sourceContact
                                    , targetContact
                                    , _anabatic->getConfiguration()->getGHorizontalLayer()
                                    , from->getAxis()
                                    , DbU::fromLambda(2.0)
                                    );
      } else {
        segment = Vertical::create( sourceContact
                                  , targetContact
                                  , _anabatic->getConfiguration()->getGVerticalLayer()
                                  , from->getAxis()
                                  , DbU::fromLambda(2.0)
                                  );
      }
      from->setSegment( segment );
    }

    cdebug_tabw(112,-1);
  }


  void  Dijkstra::run ( Dijkstra::Mode mode )
  {
    DebugSession::open( _net, 112, 120 );

    cdebug_log(112,1) << "Dijkstra::run() on " << _net << " mode:" << mode << endl;
    _mode = mode;

    _selectFirstSource();
    if (_sources.empty()) {
      cdebug_log(112,0) << "No source to start, not routed." << endl;
      cdebug_tabw(112,-1);
      return;
    }

    Flags enabledEdges = Flags::AllSides;
    if (_mode & Mode::Monotonic) {
      if ((*_sources.begin())->getCenter().getY() <= (*_targets.begin())->getCenter().getY())
        enabledEdges = Flags::EastSide | Flags::NorthSide;
      else
        enabledEdges = Flags::EastSide | Flags::SouthSide;
    }

    _queue.clear();
    _connectedsId = (*_sources.begin())->getConnexId();
    for ( Vertex* source : _sources ) {
      _queue.push( source );
      source->setDistance( 0.0 );
      cdebug_log(112,0) << "Push source: (size:" << _queue.size() << ") "
                        << source
                        << " _connectedsId:" << _connectedsId << endl;
    }


    while ( not _targets.empty() and _propagate(enabledEdges) );

    _queue.clear();

    cdebug_tabw(112,-1);
    DebugSession::close();
  }


  void  Dijkstra::ripup ( Edge* edge )
  {
    DebugSession::open( _net, 112, 120 );

    cdebug_log(112,1) << "Dijkstra::ripup(): " << edge << endl;

    GCell*  gsource = edge->getSource();
    GCell*  gtarget = edge->getTarget();
    Vertex* vsource = gsource->getObserver<Vertex>(GCell::Observable::Vertex);
    Vertex* vtarget = gtarget->getObserver<Vertex>(GCell::Observable::Vertex);

    if (  (not isSourceVertex(vsource) and not isTargetVertex(vsource)) 
       or (not isSourceVertex(vtarget) and not isTargetVertex(vtarget)) ) {
      cerr << Error( "Dijkstra::ripup(): %s do *not* belong to %s (ignored)."
                   , getString(edge).c_str()
                   , getString(_net).c_str()
                   ) << endl;
      cdebug_tabw(112,-1);
      DebugSession::close();
      return;
    } 

    edge->destroySegment();
    // for ( Contact* contact : gsource->getGContacts() ) {
    //   if (contact->getNet() != _net) continue;

    //   for ( Component* component : contact->getSlaveComponents() ) {
    //     Segment* segment = dynamic_cast<Segment*>( component );
    //     if (segment and (gtarget->hasGContact(dynamic_cast<Contact*>(segment->getTarget())))) {
    //       segment->destroy();
    //       break;
    //     }
    //   }

    //   break;
    // }

    edge->incRealOccupancy( -1 );
    _propagateRipup( vsource );
    vtarget = _propagateRipup( vtarget );
    _tagConnecteds( vtarget, ++_connectedsId );

    cdebug_tabw(112,-1);
    DebugSession::close();
  }


  Vertex* Dijkstra::_propagateRipup ( Vertex* end )
  {
    cdebug_log(112,1) << "Dijkstra::_propagateRipup() from:" << end << endl;

    while ( end ) {
      cdebug_log(112,0) << "| " << end << endl;

      Contact* gcontact = end->getGCell()->getGContact( _net );
      if (not gcontact) {
        cdebug_log(112,0) << "Exiting on missing GContact." << endl;
        cdebug_tabw(112,-1);
        return end;
      }

      Edge* eneighbor = NULL;
      for ( Edge* edge : end->getGCell()->getEdges() ) {
        if (edge->getSegment()) {
          if (not eneighbor) eneighbor = edge;
          else {
            eneighbor = NULL;
            break;
          }
        }
      }

      for ( Component* component : gcontact->getSlaveComponents() ) {
        if (dynamic_cast<RoutingPad*>(component)) {
          eneighbor = NULL;
          break;
        }
      }

      if (not eneighbor) {
        cdebug_log(112,0) << "Normal exit (fork or RoutingPad)." << endl;
        cdebug_tabw(112,-1);
        return end;
      }

      cdebug_log(112,0) << "+ " << eneighbor << endl;
      eneighbor->incRealOccupancy( -1 );
      eneighbor->destroySegment();
      eneighbor->setSegment( NULL );

      end->setConnexId( -1 );
      end->setDistance( Vertex::unreached );
      end = eneighbor->getOpposite(end->getGCell())->getObserver<Vertex>(GCell::Observable::Vertex);;
    }

    cdebug_log(112,0) << "Exiting on nothing left." << endl;
    cdebug_tabw(112,-1);
    return NULL;
  }


  void  Dijkstra::_tagConnecteds ( Vertex* source, int connexId )
  {
    cdebug_log(112,1) << "Dijkstra::_tagConnecteds()" << endl;

    source->setConnexId( connexId );

    VertexSet stack;
    stack.insert( source );

    while ( not stack.empty() ) {
      source = *stack.begin();
      stack.erase( source );

      cdebug_log(112,0) << "| source:" << source << " stack.size():" << stack.size() << endl;

      for ( Edge* edge : source->getGCell()->getEdges() ) {
        if (not edge->getSegment()) {
          cdebug_log(112,0) << "  Not connected:" << edge << endl; 
          continue;
        }

        GCell*  gneighbor = edge->getOpposite(source->getGCell());
        Vertex* vneighbor = gneighbor->getObserver<Vertex>(GCell::Observable::Vertex);

        if (not vneighbor->hasValidStamp()) continue; 
        if (vneighbor->getConnexId() == connexId) continue;

        vneighbor->setConnexId( connexId );
        stack.insert( vneighbor );
      }
    }

    cdebug_tabw(112,-1);
  }


  void  Dijkstra::_checkEdges () const
  {
    cdebug_log(112,1) << "Dijkstra::_checkEdges()" << endl;

    for ( Vertex* vertex : _vertexes ) {
      for ( Edge* edge : vertex->getGCell()->getEdges(Flags::EastSide|Flags::NorthSide) ) {
        if (edge->getSegment()) {
          cdebug_log(112,0) << "Not reset:" << edge << edge->getSegment() << endl;
        }
      }
    }

    cdebug_tabw(112,-1);
  }


}  // Anabatic namespace.
