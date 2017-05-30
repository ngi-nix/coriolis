// -*- C++ -*-
//
// This file is part of the Coriolis Software.
// Copyright (c) UPMC 2008-2016, All Rights Reserved
//
// +-----------------------------------------------------------------+
// |                   C O R I O L I S                               |
// |      K i t e  -  D e t a i l e d   R o u t e r                  |
// |                                                                 |
// |  Author      :                    Jean-Paul CHAPUT              |
// |  E-mail      :       Jean-Paul.Chaput@asim.lip6.fr              |
// | =============================================================== |
// |  C++ Header  :  "./katana/TrackFixedSegment.h"                  |
// +-----------------------------------------------------------------+


#ifndef  KATANA_TRACK_FIXED_SEGMENT_H
#define  KATANA_TRACK_FIXED_SEGMENT_H

#include "katana/TrackElement.h"


namespace Katana {

  using std::string;
  using std::map;
  using Hurricane::Record;
  using Hurricane::Interval;
  using Hurricane::DbU;
  using Hurricane::Net;
  using Hurricane::Layer;

  class Track;


// -------------------------------------------------------------------
// Class  :  "TrackFixedSegment".

  class TrackFixedSegment : public TrackElement {
    public:
      static  TrackElement*  create                 ( Katana::Track* track , Segment* segment );
    public:                                         
      virtual AutoSegment*   base                   () const;
      virtual Segment*       getSegment             () const;
      virtual bool           isHorizontal           () const;
      virtual bool           isVertical             () const;
      virtual bool           isFixed                () const;
      virtual bool           isPriorityLocked       () const;
      virtual unsigned long  getId                  () const;
      virtual Flags          getDirection           () const;
      virtual Net*           getNet                 () const;
      virtual const Layer*   getLayer               () const;
      virtual TrackElement*  getNext                () const;
      virtual TrackElement*  getPrevious            () const;
      virtual DbU::Unit      getAxis                () const;
      virtual Interval       getFreeInterval        () const;
      virtual float          getPriority            () const;
      virtual void           setPriorityLock        ( bool );
      virtual void           forcePriority          ( float );
      virtual void           computePriority        ();
      virtual void           computeAlignedPriority ();
      virtual Record*        _getRecord             () const;
      virtual string         _getString             () const;
      virtual string         _getTypeName           () const;
    protected:
    // Attributes.
      static Net*     _blockageNet;
             Segment* _segment;
    protected:
    // Constructors & Destructors.
                                 TrackFixedSegment ( Track*, Segment* ) ;
      virtual                   ~TrackFixedSegment ();
      virtual void               _postCreate       ();
      virtual void               _preDestroy       ();
    private:
                                 TrackFixedSegment ( const TrackFixedSegment& );
              TrackFixedSegment& operator=         ( const TrackFixedSegment& );
  };


}  // Katana namespace.


INSPECTOR_P_SUPPORT(Katana::TrackFixedSegment);

#endif  // KATANA_TRACK_FIXED_SEGMENT_H
