
# This file is part of the Coriolis Software.
# Copyright (c) Sorbonne Universit√© 2019-2021, All Rights Reserved
#
# +-----------------------------------------------------------------+
# |                   C O R I O L I S                               |
# |          Alliance / Hurricane  Interface                        |
# |                                                                 |
# |  Author      :                    Jean-Paul CHAPUT              |
# |  E-mail      :            Jean-Paul.Chaput@lip6.fr              |
# | =============================================================== |
# |  Python      :       "./etc/node600/phenitec/alliance.py"       |
# +-----------------------------------------------------------------+


import helpers.io
helpers.io.vprint( 2, '     - "{}".'.format(helpers.truncPath(__file__)) )

import os
import os.path
from   CRL        import Environment
from   CRL        import AllianceFramework


allianceTop = None
if 'ALLIANCE_TOP' in os.environ:
    allianceTop = os.environ['ALLIANCE_TOP']
    if not os.path.isdir(allianceTop):
        allianceTop = None
if not allianceTop: allianceTop = '/soc/alliance'

cellsTop = None
if 'CELLS_TOP' in os.environ:
    cellsTop = os.environ['CELLS_TOP']
    if not os.path.isdir(cellsTop):
        cellsTop = None
if not cellsTop:
    cellsTop = allianceTop+'/cells'

af  = AllianceFramework.get()
env = af.getEnvironment()

env.setSCALE_X        ( 100 )
env.setCATALOG        ( 'CATAL' )
env.setIN_LO          ( 'vst'   )
env.setIN_PH          ( 'ap'    )
env.setOUT_LO         ( 'vst'   )
env.setOUT_PH         ( 'ap'    )
env.setPOWER          ( 'vdd'   )
env.setGROUND         ( 'vss'   )
env.setCLOCK          ( '.*ck.*|.*nck.*'   )
env.setBLOCKAGE       ( 'blockage[Nn]et.*' )
env.setPad            ( '.*_sp$'          )

env.setWORKING_LIBRARY( '.' )
env.addSYSTEM_LIBRARY ( library=cellsTop+'/nsxlib' , mode=Environment.Append )
env.addSYSTEM_LIBRARY ( library=cellsTop+'/phlib80', mode=Environment.Append )
