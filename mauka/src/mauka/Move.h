
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
// The Coriolis Project  is free software;  you can  redistribute it and/or
// modify  it  under the  terms  of  the  GNU  General  Public License  as
// published by  the Free  Software Foundation; either  version 2  of  the
// License, or (at your option) any later version.
// 
// The Coriolis Project  is distributed in the hope that it will be useful,
// but  WITHOUT  ANY  WARRANTY;  without  even  the  implied  warranty  of
// MERCHANTABILITY  or  FITNESS  FOR A  PARTICULAR PURPOSE.   See  the GNU
// General Public License for more details.
// 
// You should have received a copy  of  the  GNU  General  Public  License
// along with  the Coriolis Project;  if  not,  write to the  Free Software
// Foundation, inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
//
//
// License-Tag
//
// Date   : 19/07/2006
// Author : Christophe Alexandre  <Christophe.Alexandre@lip6.fr>
//
// Authors-Tag 
#ifndef __MOVE_H
#define __MOVE_H
#include <map>

#include "hurricane/Instance.h"
#include "hurricane/Net.h"

// ****************************************************************************************************
// Move declaration
// ****************************************************************************************************

namespace Mauka {

using Hurricane::DbU;

class Bin;
class Row;
class SubRow;

class Move {
// *********

// Types
// *****
    public: typedef std::map<unsigned, unsigned> AffectedNets;
            
// Attributes
// **********
    private: SimAnnealingPlacer*        _simAnnealingPlacer; 
    private: MaukaEngine*               _mauka;
    private: Surface*                   _surface;
    private: bool                       _exchange;
    private: unsigned                   _srcIns;
    private: Bin*                       _srcBin;
    private: double                     _srcBinInitCost;
    private: SubRow*                    _srcSubRow;
    private: Row*                       _srcRow;
    private: double                     _srcRowInitCost;
    private: DbU::Unit                  _srcWidth;
    private: Bin*                       _dstBin;
    private: double                     _dstBinInitCost;
    private: SubRow*                    _dstSubRow;
    private: Row*                       _dstRow;
    private: double                     _dstRowInitCost;
    private: unsigned                   _dstIns;
    private: DbU::Unit                  _dstWidth;
    private: AffectedNets               _affectedNets;

// Constructors
// ************
    public: Move(SimAnnealingPlacer* simannealingplacer);
            
// Accessors
// *********
    public: double getDeltaRowCost() const;
    public: double getDeltaBinCost() const;
    public: double getDeltaNetCost();
            
// Others
// ******
    public: bool Next(const double dist);
    public: void accept();
    public: void Reject();
    public: void TryMove();
};

}

#endif /* __MOVE_H */
