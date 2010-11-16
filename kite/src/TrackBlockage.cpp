
// -*- C++ -*-
//
// This file is part of the Coriolis Software.
// Copyright (c) UPMC/LIP6 2008-2010, All Rights Reserved
//
// ===================================================================
//
// $Id$
//
// x-----------------------------------------------------------------x
// |                                                                 |
// |                   C O R I O L I S                               |
// |      K i t e  -  D e t a i l e d   R o u t e r                  |
// |                                                                 |
// |  Author      :                    Jean-Paul CHAPUT              |
// |  E-mail      :       Jean-Paul.Chaput@asim.lip6.fr              |
// | =============================================================== |
// |  C++ Module  :       "./TrackBlockage.cpp"                      |
// | *************************************************************** |
// |  U p d a t e s                                                  |
// |                                                                 |
// x-----------------------------------------------------------------x




#include  <sstream>

#include  "hurricane/Bug.h"
#include  "hurricane/Warning.h"
#include  "hurricane/Net.h"
#include  "hurricane/Name.h"
#include  "hurricane/RegularLayer.h"
#include  "hurricane/Technology.h"
#include  "hurricane/DataBase.h"
#include  "hurricane/Horizontal.h"
#include  "hurricane/Vertical.h"
#include  "katabatic/AutoContact.h"
#include  "crlcore/RoutingGauge.h"
#include  "kite/GCell.h"
#include  "kite/DataNegociate.h"
#include  "kite/TrackBlockage.h"
#include  "kite/TrackCost.h"
#include  "kite/Track.h"
#include  "kite/Session.h"
#include  "kite/RoutingEvent.h"
#include  "kite/NegociateWindow.h"
#include  "kite/GCellGrid.h"
#include  "kite/KiteEngine.h"


namespace Kite {


  using namespace std;
  using Hurricane::inltrace;
  using Hurricane::ltracein;
  using Hurricane::ltraceout;
  using Hurricane::tab;
  using Hurricane::ForEachIterator;
  using Hurricane::Bug;
  using Hurricane::Error;
  using Hurricane::Net;
  using Hurricane::Name;
  using Hurricane::RegularLayer;
  using Hurricane::Technology;
  using Hurricane::DataBase;
  using Hurricane::Horizontal;
  using Hurricane::Vertical;


// -------------------------------------------------------------------
// Class  :  "TrackBlockage".


  TrackBlockage::TrackBlockage ( Track* track, Box& boundingBox )
    : TrackElement  (NULL)
    , _segment      (NULL)
  {
    if ( track ) {
      Technology*   technology = DataBase::getDB()->getTechnology();
      unsigned int  depth      = track->getDepth();
      const Layer*  layer1     = track->getLayer()->getBlockageLayer();
      RegularLayer* layer2     = dynamic_cast<RegularLayer*>(technology->getLayer(layer1->getMask()));
      ltrace(190) << "Blockage layer: " << layer2 << endl;
      if ( layer2 ) {
        DbU::Unit extention = layer2->getExtentionCap();
        if ( track->getDirection() == Constant::Horizontal ) {
          Interval uside = track->getKiteEngine()->getGCellGrid()->getUSide ( Constant::Horizontal );

          _sourceU = max ( boundingBox.getXMin()+extention, uside.getVMin());
          _targetU = min ( boundingBox.getXMax()-extention, uside.getVMax());

          _segment = Horizontal::create ( Session::getBlockageNet()
                                        , layer2
                                        , track->getAxis()
                                        , layer2->getMinimalSize()
                                        , _sourceU
                                        , _targetU
                                        );

          GCell*   gcell   = track->getKiteEngine()->getGCellGrid()->getGCell ( Point(_sourceU,track->getAxis()) );
          GCell*   end     = track->getKiteEngine()->getGCellGrid()->getGCell ( Point(_targetU,track->getAxis()) );
          GCell*   right   = NULL;
          Interval guside  = gcell->getUSide ( Constant::Horizontal, true );
          Interval segside ( boundingBox.getXMin(), boundingBox.getXMax() );

          
          ltrace(190) << "Depth: " << depth << " " << track->getLayer() << endl;
          ltrace(190) << "Begin: " << gcell << endl;
          ltrace(190) << "End:   " << end << endl;

          if ( gcell ) {
            while ( gcell and (gcell != end) ) {
              right = gcell->getRight();
              if ( right == NULL ) break;

              guside = gcell->getUSide ( Constant::Horizontal, true );
              Interval usedLength = guside.getIntersection ( segside );

              gcell->addBlockage ( depth, (float)usedLength.getSize()/(float)guside.getSize() );
              gcell = right;
            }
            if ( end ) {
              guside = gcell->getUSide ( Constant::Horizontal, true );
              Interval usedLength = guside.getIntersection ( segside );

              end->addBlockage ( depth, (float)usedLength.getSize()/(float)guside.getSize() );
            }
          }
        } else {
          Interval uside = track->getKiteEngine()->getGCellGrid()->getUSide ( Constant::Vertical );

          _sourceU = max ( boundingBox.getYMin()+extention, uside.getVMin());
          _targetU = min ( boundingBox.getYMax()-extention, uside.getVMax());

          _segment = Vertical::create ( Session::getBlockageNet()
                                      , layer2
                                      , track->getAxis()
                                      , layer2->getMinimalSize()
                                      , _sourceU
                                      , _targetU
                                      );

          GCell*   gcell      = track->getKiteEngine()->getGCellGrid()->getGCell ( Point(track->getAxis(),_sourceU) );
          GCell*   end        = track->getKiteEngine()->getGCellGrid()->getGCell ( Point(track->getAxis(),_targetU) );
          GCell*   up         = NULL;
          Interval guside     = gcell->getUSide ( Constant::Vertical, true );
          Interval segside    ( boundingBox.getYMin(), boundingBox.getYMax() );

          ltrace(190) << "Depth: " << depth << " " << track->getLayer() << endl;
          ltrace(190) << "Begin: " << gcell << endl;
          ltrace(190) << "End:   " << end << endl;

          if ( gcell ) {
            while ( gcell and (gcell != end) ) {
              up = gcell->getUp();
              if ( up == NULL ) break;

              guside = gcell->getUSide ( Constant::Vertical, true );
              Interval usedLength = guside.getIntersection ( segside );

              gcell->addBlockage ( depth, (float)usedLength.getSize()/(float)guside.getSize() );
              gcell = up;
            }
            if ( end ) {
              guside = gcell->getUSide ( Constant::Vertical, true );
              Interval usedLength = guside.getIntersection ( segside );

              end->addBlockage ( depth, (float)usedLength.getSize()/(float)guside.getSize() );
            }
          }
        }
      }
    }
  }


  void  TrackBlockage::_postCreate ()
  { TrackElement::_postCreate (); }


  TrackBlockage::~TrackBlockage ()
  { }


  void  TrackBlockage::_preDestroy ()
  {
    ltrace(90) << "TrackBlockage::_preDestroy() - " << (void*)this << endl;
    TrackElement::_preDestroy ();
  }


  TrackElement* TrackBlockage::create ( Track* track, Box& boundingBox )
  {
    TrackBlockage* trackBlockage = NULL;
    if ( track ) { 
      trackBlockage = new TrackBlockage ( track, boundingBox );
      trackBlockage->_postCreate ();
      Session::addInsertEvent ( trackBlockage, track );

      ltrace(190) << "Adding: " << boundingBox << " on " << track << endl;
      
      ltrace(200) << "TrackBlockage::create(): " << trackBlockage << endl;
    }
    return trackBlockage;
  }


  AutoSegment*   TrackBlockage::base            () const { return NULL; }
  bool           TrackBlockage::isFixed         () const { return true; }
  bool           TrackBlockage::isBlockage      () const { return true; }
  DbU::Unit      TrackBlockage::getAxis         () const { return getTrack()->getAxis(); }
  bool           TrackBlockage::isHorizontal    () const { return getTrack()->isHorizontal(); }
  bool           TrackBlockage::isVertical      () const { return getTrack()->isVertical(); }
  unsigned int   TrackBlockage::getDirection    () const { return getTrack()->getDirection(); }
  Net*           TrackBlockage::getNet          () const { return _segment->getNet(); }
  const Layer*   TrackBlockage::getLayer        () const { return _segment->getLayer(); }
  Interval       TrackBlockage::getFreeInterval ( bool useOrder ) const { return Interval(); }


  unsigned long  TrackBlockage::getId () const
  {
    cerr << Error("::getId() called on %s.",_getString().c_str()) << endl;
    return 0;
  }


  TrackElement* TrackBlockage::getNext () const
  {
    size_t dummy = _index;
    return _track->getNext ( dummy, getNet() );
  }


  TrackElement* TrackBlockage::getPrevious () const
  {
    size_t dummy = _index;
    return _track->getPrevious ( dummy, getNet() );
  }


  string  TrackBlockage::_getTypeName () const
  { return "TrackBlockage"; }


  string  TrackBlockage::_getString () const
  {
    string s1 = _segment->_getString();
    string s2 = " ["   + DbU::getValueString(_sourceU)
              +  ":"   + DbU::getValueString(_targetU) + "]"
              +  " "   + DbU::getValueString(_targetU-_sourceU)
              + " ["   + ((_track) ? getString(_index) : "npos") + "]";
    s1.insert ( s1.size()-1, s2 );

    return s1;
  }


  Record* TrackBlockage::_getRecord () const
  {
    Record* record = TrackElement::_getRecord ();
    record->add ( getSlot ( "_segment", _segment ) );

    return record;
  }


} // End of Kite namespace.
