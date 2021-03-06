

// -*- C++ -*-
//
// This file is part of the Coriolis Project.
// Copyright (C) Laboratoire LIP6 - Departement ASIM
// Universite Pierre et Marie Curie
//
// Main contributors :
//        Christophe Alexandre   <Christophe.Alexandre@lip6.fr>
//        Sophie Belloeil             <Sophie.Belloeil@lip6.fr>
//        Hugo Cl?ment                   <Hugo.Clement@lip6.fr>
//        Jean-Paul Chaput           <Jean-Paul.Chaput@lip6.fr>
//        Damien Dupuis                 <Damien.Dupuis@lip6.fr>
//        Christian Masson           <Christian.Masson@lip6.fr>
//        Marek Sroka                     <Marek.Sroka@lip6.fr>
// 
// The  Coriolis Project  is  free software;  you  can redistribute it
// and/or modify it under the  terms of the GNU General Public License
// as published by  the Free Software Foundation; either  version 2 of
// the License, or (at your option) any later version.
// 
// The  Coriolis Project is  distributed in  the hope that it  will be
// useful, but WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY  or FITNESS FOR  A PARTICULAR PURPOSE.   See the
// GNU General Public License for more details.
// 
// You should have  received a copy of the  GNU General Public License
// along with the Coriolis Project; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA
//
// License-Tag
// Authors-Tag
// ===================================================================
//
// $Id: CLEFDEF.h,v 1.4 2006/02/19 00:52:50 jpc Exp $
//
// x-----------------------------------------------------------------x 
// |                                                                 |
// |                   C O R I O L I S                               |
// |          Alliance / Hurricane  Interface                        |
// |                                                                 |
// |  Author      :                Christophe Alexandre              |
// |  E-mail      :        Christophe.Alexandre@lip6.fr              |
// | =============================================================== |
// |  C++ Header  :       "./LefDef.h"                               |
// | *************************************************************** |
// |  U p d a t e s                                                  |
// |                                                                 |
// x-----------------------------------------------------------------x


# include  <iostream>

namespace Hurricane {
  class Library;
  class Cell;
}

# ifndef  __CRL_LEFDEF_H__
#   define  __CRL_LEFDEF_H__


namespace CRL {


  using std::string;
  using Hurricane::Library;
  using Hurricane::Cell;

  class Catalog;


// -------------------------------------------------------------------
// functions.

  extern void  CParsLEFTechno ( string fileTechno );
  extern void  lefParser ( const string libPath, Library* lib, Catalog& catal );
  extern void  defParser ( const string cellPath, Cell* cell );
  extern void  defDriver ( const string cellPath, Cell* cell, unsigned int& saveState );


} // End of CRL namespace.


# endif // __CRL_LEFDEF_H__
