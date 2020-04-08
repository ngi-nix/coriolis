#!/usr/bin/env python
#
# This file is part of the Coriolis Software.
# Copyright (c) UPMC 2014-2018, All Rights Reserved
#
# +-----------------------------------------------------------------+
# |                   C O R I O L I S                               |
# |      C u m u l u s  -  P y t h o n   T o o l s                  |
# |                                                                 |
# |  Author      :                    Jean-Paul CHAPUT              |
# |  E-mail      :            Jean-Paul.Chaput@lip6.fr              |
# | =============================================================== |
# |  Python      :       "./plugins/clocktreeplugin.py"             |
# +-----------------------------------------------------------------+

import sys
import traceback
import os.path
import math
import Cfg
import Hurricane
from   Hurricane  import Breakpoint
from   Hurricane  import UpdateSession
import Viewer
import CRL
from   CRL        import RoutingLayerGauge
import helpers
from   helpers    import trace
from   helpers.io import ErrorMessage
import Etesian
import Unicorn
import plugins
from   plugins.cts.clocktree       import HTree
from   plugins.cts.clocktree       import computeAbutmentBox
from   plugins.chip.configuration  import ChipConf


# --------------------------------------------------------------------
# Plugin hook functions, unicornHook:menus, ScritMain:call

def unicornHook ( **kw ):
    kw['beforeAction'] = 'placeAndRoute.placeChip'

    plugins.kwAddMenu    ( 'placeAndRoute', 'P&&R', **kw )
    plugins.kwUnicornHook( 'placeAndRoute.clockTree'
                         , 'Place Block && Clock Tree'
                         , 'Place a block with a buffered H-Tree for the clock'
                         , sys.modules[__name__].__file__
                         , **kw
                         )
    return


def scriptMain ( **kw ):
    try:
       #helpers.setTraceLevel( 550 )
        errorCode = 0
      
        print '  o  Cleaning up any previous run.'
        for fileName in os.listdir('.'):
            if fileName.endswith('.ap'):
                print '      - <%s>' % fileName
                os.unlink(fileName)
      
        cell = None
        if kw.has_key('cell') and kw['cell']:
            cell = kw['cell']
      
        editor = None
        if kw.has_key('editor') and kw['editor']:
            editor = kw['editor']
            print '  o  Editor detected, running in graphic mode.'
            if cell == None: cell = editor.getCell()
      
        if cell == None:
            raise ErrorMessage( 3, 'ClockTree: No cell loaded yet.' )
      
        conf = ChipConf( {}, cell, editor )
      
        if cell.getAbutmentBox().isEmpty():
            spaceMargin = Cfg.getParamPercentage('etesian.spaceMargin').asPercentage() / 100.0 + 0.01
            aspectRatio = Cfg.getParamPercentage('etesian.aspectRatio').asPercentage() / 100.0
            computeAbutmentBox( cell, spaceMargin, aspectRatio, conf.cellGauge )
            if editor: editor.fit()
      
        ht = HTree.create( conf, cell, None, cell.getAbutmentBox() )
        if editor: editor.refresh()
        etesian = Etesian.EtesianEngine.create( cell )
        etesian.place()
        etesian.destroy()
        ht.connectLeaf()
       #ht.prune()
        ht.route()
        ht.save( cell )
    except Exception, e:
        helpers.io.catch( e )
      
    return 0
