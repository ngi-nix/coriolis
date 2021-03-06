
# This file is part of the Coriolis Software.
# Copyright (c) UPMC 2019-2019, All Rights Reserved
#
# +-----------------------------------------------------------------+
# |                   C O R I O L I S                               |
# |          Alliance / Hurricane  Interface                        |
# |                                                                 |
# |  Author      :                    Jean-Paul CHAPUT              |
# |  E-mail      :       Jean-Paul.Chaput@asim.lip6.fr              |
# | =============================================================== |
# |  Python      :       "./etc/common/etesian.py"                  |
# +-----------------------------------------------------------------+


import Cfg


param = Cfg.getParamDouble( 'etesian.aspectRatio' )
param.setDouble( 1.0 )

Cfg.getParamDouble    ( 'etesian.spaceMargin'    ).setPercentage( 0.05 )
Cfg.getParamBool      ( 'etesian.uniformDensity' ).setBool      ( False )
Cfg.getParamBool      ( 'etesian.routingDriven'  ).setBool      ( False )
Cfg.getParamString    ( 'etesian.feedNames'      ).setString    ( 'tie_x0,rowend_x0' )
Cfg.getParamString    ( 'etesian.cell.zero'      ).setString    ( 'zero_x0' )
Cfg.getParamString    ( 'etesian.cell.one'       ).setString    ( 'one_x0' )
Cfg.getParamString    ( 'etesian.bloat'          ).setString    ( 'disabled' )

param = Cfg.getParamEnumerate( 'etesian.effort' )
param.setInt( 2 )
param.addValue( 'Fast'    , 1 )
param.addValue( 'Standard', 2 )
param.addValue( 'High'    , 3 )
param.addValue( 'Extreme' , 4 )

param = Cfg.getParamEnumerate( 'etesian.graphics' )
param.setInt( 2 )
param.addValue( 'Show every step' , 1 )
param.addValue( 'Show lower bound', 2 )
param.addValue( 'Show result only', 3 )

layout  = Cfg.Configuration.get().getLayout()
layout.addTab      ( 'Etesian', 'etesian' )
layout.addTitle    ( 'Etesian', 'Placement area' )
layout.addParameter( 'Etesian', 'etesian.aspectRatio'   , 'Aspect Ratio, X/Y (%)', 0 )
layout.addParameter( 'Etesian', 'etesian.spaceMargin'   , 'Space Margin'         , 1 )
layout.addRule     ( 'Etesian' )
layout.addTitle    ( 'Etesian', 'Etesian - Placer')
layout.addParameter( 'Etesian', 'etesian.uniformDensity', 'Uniform density'      , 0 )
layout.addParameter( 'Etesian', 'etesian.routingDriven' , 'Routing driven'       , 0 )
layout.addParameter( 'Etesian', 'etesian.effort'        , 'Placement effort'     , 1 )
layout.addParameter( 'Etesian', 'etesian.graphics'      , 'Placement view'       , 1 )
layout.addRule     ( 'Etesian' )
