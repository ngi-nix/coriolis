
// -*- C++ -*-
//
// This file is part of the Coriolis Software.
// Copyright (c) UPMC/LIP6 2010-2010, All Rights Reserved
//
// ===================================================================
//
// $Id$
//
// x-----------------------------------------------------------------x 
// |                                                                 |
// |                   C O R I O L I S                               |
// |          Alliance / Hurricane  Interface                        |
// |                                                                 |
// |  Author      :                    Jean-Paul CHAPUT              |
// |  E-mail      :       Jean-Paul.Chaput@asim.lip6.fr              |
// | =============================================================== |
// |  C++ Module  :       "./PyToolBox.cpp"                          |
// | *************************************************************** |
// |  U p d a t e s                                                  |
// |                                                                 |
// x-----------------------------------------------------------------x


#include "hurricane/isobar/PyCell.h"
#include "crlcore/ToolBox.h"
#include "crlcore/PyToolBox.h"


namespace  CRL {

  using std::cerr;
  using std::endl;
  using std::hex;
  using std::string;
  using std::ostringstream;
  using Hurricane::tab;
  using Hurricane::Exception;
  using Hurricane::Bug;
  using Hurricane::Error;
  using Hurricane::Warning;
  using Isobar::ProxyProperty;
  using Isobar::ProxyError;
  using Isobar::ConstructorError;
  using Isobar::HurricaneError;
  using Isobar::HurricaneWarning;
  using Isobar::getPyHash;
  using Isobar::__cs;
  using Isobar::Converter;
  using Isobar::ParseOneArg;
  using Isobar::ParseTwoArg;
  using Isobar::PyCell;
  using Isobar::PyCell_Link;


extern "C" {


// x=================================================================x
// |              "PyToolBox" Python Module Code Part                |
// x=================================================================x

#if defined(__PYTHON_MODULE__)


#else  // End of Python Module Code Part.


// x=================================================================x
// |               "PyToolBox" Shared Library Code Part              |
// x=================================================================x


  extern PyObject* PyToolBox_createPartRing ( PyObject* module, PyObject* args )
  { 
    cdebug_log(30,0) << "PyToolBox_createPartRing ()" << endl;
    HTRY
      PyObject* arg0;
      PyObject* arg1;
      if ( not ParseTwoArg( "CRL.createPartRing", args, CELL_STRING_ARG, &arg0, &arg1 ) )
        return NULL;
      createPartRing( PYCELL_O(arg0), PyString_AsString(arg1) );
      HCATCH
    Py_RETURN_NONE;
  } 


  extern PyObject* PyToolBox_restoreNetsDirection ( PyObject* module, PyObject* args )
  {
    cdebug_log(30,0) << "PyToolbox_restoreNetsDirection()" << endl;
    HTRY
      PyObject* arg0 = NULL;
      PyObject* arg1 = NULL;
      __cs.init( "CRL.restoreNetsDirection" );
      if (not PyArg_ParseTuple(args,"O&O&:CRL.restoreNetsDirection"
                              ,Converter,&arg0
                              ,Converter,&arg1
                              )) {
          PyErr_SetString( ConstructorError, "CRL.restoreNetsDirection(): Takes exactly two parameters." );
          return NULL;
      }
  
      if (__cs.getObjectIds() == ":ent:int") {
        restoreNetsDirection( PYCELL_O(arg0), PyInt_AsLong(arg1) );
      } else {
        string message = "CRL.restoreNetsDirection(): Bad type of parameter(s), \"" + __cs.getObjectIds() + "\".";
        PyErr_SetString( ConstructorError, message.c_str() );
        return NULL;
      }
    HCATCH
    Py_RETURN_NONE;
  }


#endif  // End of Shared Library Code Part.


}  // End of extern "C".


}  // End of CRL namespace.
