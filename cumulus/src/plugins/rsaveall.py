
# This file is part of the Coriolis Software.
# Copyright (c) Sorbonne Universit√© 2015-2021, All Rights Reserved
#
# +-----------------------------------------------------------------+
# |                   C O R I O L I S                               |
# |      C u m u l u s  -  P y t h o n   T o o l s                  |
# |                                                                 |
# |  Author      :                    Jean-Paul CHAPUT              |
# |  E-mail      :            Jean-Paul.Chaput@lip6.fr              |
# | =============================================================== |
# |  Python      :       "./plugins/rsaveall.py"                    |
# +-----------------------------------------------------------------+


try:
    import sys
    import traceback
    import os.path
    import Cfg
    import CRL
    import helpers
    from   helpers.io import ErrorMessage
    from   helpers.io import WarningMessage
    import plugins
except Exception as e:
    helpers.io.catch( e )
    sys.exit(2)


# Write back netlist & layout to disk. Layout is written
# *only* if there is an abutment box otherwise the cell is
# considered as unplaced.
# Must write all the sub-blocks of the core but *not* the
# standard cell (mainly the feed-through).

def rsave ( cell, depth=0 ):
    if cell.isTerminal(): return
    framework = CRL.AllianceFramework.get()
    if depth == 0: print( '  o  Recursive Save-Cell.' )
    print( '     {}+ {} (netlist+layout).'.format( ' '*(depth*2), cell.getName() ))
    flags = CRL.Catalog.State.Logical
    if not cell.getAbutmentBox().isEmpty():
        flags |= CRL.Catalog.State.Physical
    framework.saveCell( cell, flags )
    for instance in cell.getInstances():
        masterCell = instance.getMasterCell()
        if not masterCell.isTerminal():
            rsave( masterCell, depth+1 )
    return


# --------------------------------------------------------------------
# Plugin hook functions, unicornHook:menus, ScritMain:call

def unicornHook ( **kw ):
    plugins.kwUnicornHook( 'tools.rsaveAll'
                         , 'Rercursive Save (all)'
                         , 'Recursively save layout <b>and</b> netlist of top cell and it\'s Instances'
                         , sys.modules[__name__].__file__
                         , **kw
                         )
    return


def scriptMain ( **kw ):
    try:
       #helpers.setTraceLevel( 550 )
        cell, editor = plugins.kwParseMain( **kw )
        if not cell:
            print( WarningMessage( 'No Cell loaded in the editor (yet), nothing done.' ))
            return 0
        rsave( cell )
        CRL.destroyAllVHDL()
    except Exception as e:
        helpers.io.catch( e )
    sys.stdout.flush()
    sys.stderr.flush()
    return 0
