// ****************************************************************************************************
// File: Reference.cpp
// Authors: J.-P. Chaput
// Copyright (c) LIP6/UPMC 2006-2006, All Rights Reserved
// ****************************************************************************************************

#include "hurricane/Reference.h"
#include "hurricane/Cell.h"
#include "hurricane/Slice.h"
#include "hurricane/Error.h"

namespace Hurricane {



// ****************************************************************************************************
// Reference implementation
// ****************************************************************************************************

// Static Variables
// ****************


  DbU::Unit Reference::_extend = 0;


Reference::Reference(Cell* cell, const Name& name, DbU::Unit x, DbU::Unit y)
// ***************************************************************
:  Inherit(cell),
  _name(name),
  _point(x,y)
{
  if ( !_extend ) _extend = DbU::grid(0.5);

  if (_name.isEmpty())
    throw Error("Can't create " + _TName("Reference") + " : empty name");
}

Reference* Reference::create(Cell* cell, const Name& name, DbU::Unit x, DbU::Unit y)
// ***********************************************************************
{
  Reference* reference = new Reference(cell, name, x, y);

  reference->_postCreate();

  return reference;
}

Reference* Reference::create(Cell* cell, const Name& name, const Point& point)
// ***************************************************************************
{
  return create(cell,name,point.getX(),point.getY());
}

Box  Reference::getBoundingBox() const
// ***********************************
{
  return Box(_point).inflate(_extend);
}

void Reference::translate(const DbU::Unit& dx, const DbU::Unit& dy)
// *****************************************************
{
  if ((dx != 0) || (dy != 0)) {
    invalidate(false);
    _point.translate(dx, dy);
  }
}

string Reference::_getString() const
// ********************************
{
  string s = Inherit::_getString();
  s.insert(s.length() - 1, " " + getString(_name));
  s.insert(s.length() - 1, " " + getString(_point));
  return s;
}

Record* Reference::_getRecord() const
// ***************************
{
  Record* record = Inherit::_getRecord();
  if (record) {
    record->add(getSlot("Name", &_name));
    record->add(getSlot("point", &_point));
  }
  return record;
}

} // End of Hurricane namespace.

// ****************************************************************************************************
// Copyright (c) LIP6/UPMC 2006-2006, All Rights Reserved
// ****************************************************************************************************
