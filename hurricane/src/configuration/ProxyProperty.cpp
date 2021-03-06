
// -*- C++ -*-
//
// This file is part of the Coriolis Software.
// Copyright (c) UPMC 2008-2018, All Rights Reserved
//
// +-----------------------------------------------------------------+ 
// |                   C O R I O L I S                               |
// |    I s o b a r  -  Hurricane / Python Interface                 |
// |                                                                 |
// |  Authors     :                    Jean-Paul CHAPUT              |
// |  E-mail      :       Jean-Paul.Chaput@asim.lip6.fr              |
// | =============================================================== |
// |  C++ Module  :  "./ProxyProperty.cpp"                           |
// +-----------------------------------------------------------------+


#include <iostream>
#include <sstream>
#include "hurricane/configuration/ProxyProperty.h"
#include "hurricane/Error.h"
#include "hurricane/Warning.h"

using namespace Hurricane;

namespace Isobar3 {

namespace {

  using namespace std;


  // ---------------------------------------------------------------
  // Local Variables.


  const char* twiceSetOffset =
    "ProxyProperty::setOffset () :\n\n"
    "    Attempt to sets the _shadow member offset twice.\n";


}


// -------------------------------------------------------------------
// Class : "ProxyProperty".


  Name    ProxyProperty::_name     = "Isobar3::ProxyProperty";
  size_t  ProxyProperty::_offset = (size_t)-1;


  ProxyProperty::ProxyProperty ( void* shadow )
    : Property()
    , _owner  (NULL)
    , _shadow (shadow)
  {
    if (_offset == (size_t)-1)
      throw Error( "ProxyProperty::ProxyProperty(): _offset has not been computed yet!" );
  }


  ProxyProperty* ProxyProperty::create ( void* shadow )
  {
    if (not shadow)
      throw Error( "ProxyProperty::create(): Empty \"shadow\" argument." );

    ProxyProperty* property = new ProxyProperty ( shadow );
    if (not property)
      throw Error( "ProxyProperty::create()" );
    return property;
  }


  void  ProxyProperty::_preDestroy ()
  {
    if (_owner) _owner->_onDestroyed( this );

    cdebug_log(20,0) << "ProxyProperty::_owner:        " << (void*)_owner  << endl;
    cdebug_log(20,0) << "ProxyProperty::_shadow:       " << (void*)_shadow << endl;
    if (_offset >= 0) {
      void** shadowMember = ( (void**)( (size_t)_shadow + _offset ) );
      cdebug_log(20,0) << "ProxyProperty::_shadowMember: " << (void*)*shadowMember << endl;
      *shadowMember = NULL;
    }
  }


  void  ProxyProperty::onCapturedBy ( DBo* owner )
  {
    if ((_owner) and (_owner != owner) )
      throw Error( getString(this).c_str() );
    _owner = owner;
  }


  void  ProxyProperty::onReleasedBy ( DBo* owner )
  { if (_owner == owner) onNotOwned (); }


  void  ProxyProperty::onNotOwned ()
  { destroy (); }


  void  ProxyProperty::setOffset ( size_t offset )
  {
    if (_offset != (size_t)-1) throw Error( twiceSetOffset );
    _offset = offset;
  }


  string  ProxyProperty::_getString () const
  {
    ostringstream  os;
    os << "<" << _getTypeName() << " ";
    if ( _owner ) os << hex << (void*)_owner << " ";
    else          os << "[not owned] ";
    os << _shadow << ">";
    return os.str();
  }


  Record* ProxyProperty::_getRecord () const
  {
    Record* record = Property::_getRecord ();

    if (record) {
      record->add ( getSlot ( "_owner" ,  _owner  ) );
      record->add ( getSlot ( "_shadow",  _shadow ) );
    }
    return ( record );
  }


} // Isobar namespace.
