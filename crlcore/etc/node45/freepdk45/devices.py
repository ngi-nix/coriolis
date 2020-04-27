
# This file is part of the Coriolis Software.
# Copyright (c) UPMC 2019-2020, All Rights Reserved
#
# +-----------------------------------------------------------------+
# |                   C O R I O L I S                               |
# |          Alliance / Hurricane  Interface                        |
# |                                                                 |
# |  Author      :                    Jean-Paul CHAPUT              |
# |  E-mail      :            Jean-Paul.Chaput@lip6.fr              |
# | =============================================================== |
# |  Python      :       "./node45/freepdk45/devices.py"            |
# +-----------------------------------------------------------------+


import helpers.io
helpers.io.vprint( 2, '     - "%s".' % helpers.truncPath(__file__) )

import common.devices
from   common.devices import addDevice


chamsDir = helpers.sysConfDir + '/share/coriolis2/'
spiceDir = chamsDir + 'spice/'


addDevice( name       = 'DifferentialPairBulkConnected'
         , spice      = spiceDir+'DiffPairBulkConnected.spi'
         , connectors = ( 'D1', 'D2', 'G1', 'G2', 'S' )
         , layouts    = ( ('Horizontal M2'  , 'DP_horizontalM2.py'    )
                        , ('Symmetrical'    , 'DP_symmetrical.py'     )
                        , ('Common centroid', 'DP_2DCommonCentroid.py')
                        , ('Interdigitated' , 'DP_interdigitated.py'  )
                        , ('WIP DP'         , 'wip_dp.py'             )
                        )
         )
addDevice( name       = 'DifferentialPairBulkUnconnected'
         , spice      = spiceDir+'DiffPairBulkUnconnected.spi'
         , connectors = ( 'D1', 'D2', 'G1', 'G2', 'S', 'B' )
         , layouts    = ( ('Horizontal M2'  , 'DP_horizontalM2.py'    )
                        , ('Symmetrical'    , 'DP_symmetrical.py'     )
                        , ('Common centroid', 'DP_2DCommonCentroid.py')
                        , ('Interdigitated' , 'DP_interdigitated.py'  )
                        , ('WIP DP'         , 'wip_dp.py'             )
                        )
         )
addDevice( name       = 'LevelShifterBulkUnconnected'
         , spice      = spiceDir+'LevelShifterBulkUnconnected.spi'
         , connectors = ( 'D1', 'D2', 'S1', 'S2', 'B' )
         , layouts    = ( ('Horizontal M2'  , 'LS_horizontalM2.py'    )
                        , ('Symmetrical'    , 'LS_symmetrical.py'     )
                        , ('Common centroid', 'LS_2DCommonCentroid.py')
                        , ('Interdigitated' , 'LS_interdigitated.py'  )
                        )
         )
addDevice( name       = 'TransistorBulkConnected'
         , spice      = spiceDir+'TransistorBulkConnected.spi'
         , connectors = ( 'D', 'G', 'S' )
         , layouts    = ( ('Rotate transistor', 'Transistor_rotate.py')
                        , ('Common transistor', 'Transistor_common.py')
                        , ('WIP Transistor'   , 'wip_transistor.py'   )
                        )
         )
addDevice( name       = 'TransistorBulkUnconnected'
         , spice      = spiceDir+'TransistorBulkUnconnected.spi'
         , connectors = ( 'D', 'G', 'S', 'B' )
         , layouts    = ( ('Rotate transistor', 'Transistor_rotate.py')
                        , ('Common transistor', 'Transistor_common.py')
                        , ('WIP Transistor'   , 'wip_transistor.py'   )
                        )
         )
addDevice( name       = 'CrossCoupledPairBulkConnected'
         , spice      = spiceDir+'CCPairBulkConnected.spi'
         , connectors = ( 'D1', 'D2', 'S' )
         , layouts    = ( ('Horizontal M2'  , 'CCP_horizontalM2.py'    )
                        , ('Symmetrical'    , 'CCP_symmetrical.py'     )
                        , ('Common centroid', 'CCP_2DCommonCentroid.py')
                        , ('Interdigitated' , 'CCP_interdigitated.py'  )
                        )
         )
addDevice( name       = 'CrossCoupledPairBulkUnconnected'
         , spice      = spiceDir+'CCPairBulkUnconnected.spi'
         , connectors = ( 'D1', 'D2', 'S', 'B' )
         , layouts    = ( ('Horizontal M2'  , 'CCP_horizontalM2.py'    )
                        , ('Symmetrical'    , 'CCP_symmetrical.py'     )
                        , ('Common centroid', 'CCP_2DCommonCentroid.py')
                        , ('Interdigitated' , 'CCP_interdigitated.py'  )
                        )
         )
addDevice( name       = 'CommonSourcePairBulkConnected'
         , spice      = spiceDir+'CommonSourcePairBulkConnected.spi'
         , connectors = ( 'D1', 'D2', 'S', 'G' )
         , layouts    = ( ('Horizontal M2'  , 'CSP_horizontalM2.py'    )
                        , ('Symmetrical'    , 'CSP_symmetrical.py'     )
                        , ('Interdigitated' , 'CSP_interdigitated.py'  )
                        , ('WIP CSP'        , 'wip_csp.py'             )
                        )
         )
addDevice( name       = 'CommonSourcePairBulkUnconnected'
         , spice      = spiceDir+'CommonSourcePairBulkUnconnected.spi'
         , connectors = ( 'D1', 'D2', 'S', 'G', 'B' )
         , layouts    = ( ('Horizontal M2'  , 'CSP_horizontalM2.py'    )
                        , ('Symmetrical'    , 'CSP_symmetrical.py'     )
                        , ('Interdigitated' , 'CSP_interdigitated.py'  )
                        , ('WIP CSP'        , 'wip_csp.py'             )
                        )
         )
addDevice( name       = 'SimpleCurrentMirrorBulkConnected'
         , spice      = spiceDir+'CurrMirBulkConnected.spi'
         , connectors = ( 'D1', 'D2', 'S' )
         , layouts    = ( ('Horizontal M2'  , 'SCM_horizontalM2.py'    )
                        , ('Symmetrical'    , 'SCM_symmetrical.py'     )
                        , ('Common centroid', 'SCM_2DCommonCentroid.py')
                        , ('Interdigitated' , 'SCM_interdigitated.py'  )
                        )
         )
addDevice( name       = 'SimpleCurrentMirrorBulkUnconnected'
         , spice      = spiceDir+'CurrMirBulkUnconnected.spi'
         , connectors = ( 'D1', 'D2', 'S', 'B' )
         , layouts    = ( ('Horizontal M2'  , 'SCM_horizontalM2.py'    )
                        , ('Symmetrical'    , 'SCM_symmetrical.py'     )
                        , ('Common centroid', 'SCM_2DCommonCentroid.py')
                        , ('Interdigitated' , 'SCM_interdigitated.py'  )
                        )
         )
addDevice( name       = 'MultiCapacitor'
        #, spice      = spiceDir+'MIM_OneCapacitor.spi'
        #, connectors = ( 'T1', 'B1' )
         , layouts    = ( ('Matrix', 'capacitormatrix.py' ),
                        )
         )
addDevice( name       = 'Resistor'
        #, spice      = spiceDir+'MIM_OneCapacitor.spi'
         , connectors = ( 'PIN1', 'PIN2' )
         , layouts    = ( ('Snake', 'resistorsnake.py' ),
                        )
         )

