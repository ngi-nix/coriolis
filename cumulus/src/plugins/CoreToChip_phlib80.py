#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# This file is part of the Coriolis Software.
# Copyright (c) UPMC 2019-2018, All Rights Reserved
#
# +-----------------------------------------------------------------+
# |                   C O R I O L I S                               |
# |      C u m u l u s  -  P y t h o n   T o o l s                  |
# |                                                                 |
# |  Author      :                    Jean-Paul CHAPUT              |
# |  E-mail      :            Jean-Paul.Chaput@lip6.fr              |
# | =============================================================== |
# |  Python      :   "./plugins/CoreToChip_phlib80.py"              |
# +-----------------------------------------------------------------+

import sys
import helpers
from   helpers.io import ErrorMessage
from   helpers.io import WarningMessage
import plugins
import core2chip.phlib80


# --------------------------------------------------------------------
# Plugin hook functions, unicornHook:menus, ScritMain:call

def unicornHook ( **kw ):
    kw['beforeAction'] = 'placeAndRoute.stepByStep'
   #kw['beforeAction'] = 'placeAndRoute.clockTree'

    plugins.kwAddMenu    ( 'placeAndRoute'          , 'P&&R', **kw )
    plugins.kwAddMenu    ( 'placeAndRoute.core2chip', 'Core To Chip', **kw )
    plugins.kwUnicornHook( 'placeAndRoute.core2chip.phlib80'
                         , 'Phenitec08 Symbolic CMOS I/O pads'
                         , 'Wrap a complete chip around a Core for Phenitec generic CMOS'
                         , sys.modules[__name__].__file__
                         , **kw
                         )
    return


def ScriptMain ( **kw ):
  rvalue = True
  try:
    helpers.staticInitialization( quiet=True )
   #helpers.setTraceLevel( 550 )

    cell, editor = plugins.kwParseMain( **kw )
    if not cell:
      raise ErrorMessage( 1, 'CoreToChip_phlib80.ScriptMain(): No cell (core) loaded in the editor yet.' )

    chip_phlib80 = core2chip.phlib80.phlib80( cell )
    chip_phlib80.buildChip()
    
    if editor: editor.setCell( chip_phlib80.chip )

  except Exception, e:
    helpers.io.catch( e )
    rvalue = False

  sys.stdout.flush()
  sys.stderr.flush()
      
  return rvalue
