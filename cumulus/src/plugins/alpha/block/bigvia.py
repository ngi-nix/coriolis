
# This file is part of the Coriolis Software.
# Copyright (c) Sorbonne Universit√© 2020-2021, All Rights Reserved
#
# +-----------------------------------------------------------------+
# |                   C O R I O L I S                               |
# |      C u m u l u s  -  P y t h o n   T o o l s                  |
# |                                                                 |
# |  Author      :                    Jean-Paul CHAPUT              |
# |  E-mail      :            Jean-Paul.Chaput@lip6.fr              |
# | =============================================================== |
# |  Python      :       "./plugins/block/bigvia.py"                |
# +-----------------------------------------------------------------+


import sys
import os.path
import Cfg
from   Hurricane import Breakpoint, DbU, Box, Transformation, Point, \
                        Box, Path, Layer, Occurrence, Net,           \
                        NetExternalComponents, RoutingPad, Pad,      \
                        Horizontal, Vertical, Contact, Pin, Plug,    \
                        Instance
import CRL
from   CRL             import RoutingLayerGauge
from   helpers         import trace, dots
from   helpers.io      import ErrorMessage, WarningMessage, catch
from   helpers.overlay import UpdateSession

rg = None


# ----------------------------------------------------------------------------
# Class  :  "bigvia.BigVia".

class BigVia ( object ):
    """
    Draw a large are VIA and manage the matrix of cuts.
    """
    AllowTopMetalExpand   = 0x0001
    AllowBotMetalExpand   = 0x0002
    AllowHorizontalExpand = 0x0004
    AllowVerticalExpand   = 0x0008
    AllowAllExpand        = AllowTopMetalExpand   \
                          | AllowBotMetalExpand   \
                          | AllowHorizontalExpand \
                          | AllowVerticalExpand

    def __init__ ( self, net, depth, x, y, width, height, flags=0 ):
        self.flags       = flags
        self.hasLayout   = False
        self.net         = net
        self.bottomDepth = depth
        self.topDepth    = depth
        self.x           = x
        self.y           = y
        self.widths      = {}
        self.heights     = {}
        self.plates      = {}
        self.vias        = {}
        self.widths [depth] = width
        self.heights[depth] = height

    def __str__ ( self ):
        global rg
        if rg is None: rg = CRL.AllianceFramework.get().getRoutingGauge()
        return '<BigVia {} [{} {}] @({} {}) {} x {}>' \
               .format( self.net.getName()
                      , rg.getRoutingLayer(self.bottomDepth).getName()
                      , rg.getRoutingLayer(self.topDepth).getName()
                      , DbU.getValueString(self.x)
                      , DbU.getValueString(self.y)
                      , DbU.getValueString(self.widths [self.topDepth])
                      , DbU.getValueString(self.heights[self.topDepth]) )

    @property
    def height ( self ):
        maxHeight = 0
        for depth in range(self.bottomDepth,self.topDepth+1):
            maxHeight = max( maxHeight, self.heights[depth] )
        return maxHeight

    @property
    def width ( self ):
        maxWidth = 0
        for depth in range(self.bottomDepth,self.topDepth+1):
            maxWidth = max( maxWidth, self.widths[depth] )
        return maxWidth

    def getNet ( self ): return self.net

    def getPlate ( self, metal ):
        if not self.hasLayout: return None
        for plate in self.plates.values():
            if plate.getLayer().contains(metal): return plate
        return None

    def mergeDepth ( self, depth ):
        if self.hasLayout:
            print( WarningMessage( 'BigVia.mergeDepth(): Cannot be called *after* BigVia.doLayout()' ))
            return
        if depth < self.bottomDepth:
            bdepth = depth
            while depth < self.bottomDepth:
                self.widths [depth] = self.widths [self.topDepth]
                self.heights[depth] = self.heights[self.topDepth]
                depth += 1
            self.bottomDepth = bdepth
        if depth > self.topDepth:
            tdepth = depth
            while depth > self.topDepth:
                self.widths [depth] = self.widths [self.topDepth]
                self.heights[depth] = self.heights[self.topDepth]
                depth -= 1
            self.topDepth = tdepth

    def doLayout ( self ):
        global rg
        if rg is None: rg = CRL.AllianceFramework.get().getRoutingGauge()
        for depth in range(self.bottomDepth,self.topDepth+1):
            minSize = rg.getRoutingLayer( depth ).getMinimalSize()
            if self.widths[depth] < minSize and (self.flags & BigVia.AllowHorizontalExpand):
                self.widths[depth] = minSize
            if self.heights[depth] < minSize and (self.flags & BigVia.AllowVerticalExpand):
                self.heights[depth] = minSize
        for depth in range(self.bottomDepth,self.topDepth+1):
            minArea   = rg.getRoutingLayer( depth ).getMinimalArea()
            plateArea = DbU.toPhysical( self.widths [depth], DbU.UnitPowerMicro ) \
                      * DbU.toPhysical( self.heights[depth], DbU.UnitPowerMicro )
            if plateArea < minArea:
                if depth == self.bottomDepth and not (self.flags & BigVia.AllowBotMetalExpand):
                    print( WarningMessage( 'BigVia::doLayout(): @({},{}) Area too small for {} in layer "{}"' \
                                           .format( DbU.getValueString(self.x)
                                                  , DbU.getValueString(self.y)
                                                  , rg.getRoutingLayer(depth).getName()
                                                  , self.net
                                                  ) ))
                if depth == self.topDepth and not (self.flags & BigVia.AllowTopMetalExpand):
                    print( WarningMessage( 'BigVia::doLayout(): @({},{}) Area too small for {} in layer "{}"' \
                                           .format( DbU.getValueString(self.x)
                                                  , DbU.getValueString(self.y)
                                                  , rg.getRoutingLayer(depth).getName()
                                                  , self.net
                                                  ) ))
            self.plates[ depth ] = Contact.create( self.net
                                                 , rg.getRoutingLayer(depth)
                                                 , self.x            , self.y
                                                 , self.widths[depth], self.heights[depth]
                                                 )
        if rg.isSymbolic():
            for depth in range(self.bottomDepth,self.topDepth):
                Contact.create( self.net
                              , rg.getContactLayer( depth )
                              , self.x
                              , self.y
                              , self.widths [depth] - DbU.fromLambda( 1.0 )
                              , self.heights[depth] - DbU.fromLambda( 1.0 ) )
        else:
            for depth in range(self.bottomDepth,self.topDepth):
                self._doCutMatrix( depth )
        self.hasLayout = True

    def _doCutMatrix ( self, depth ):
        viaLayer     = rg.getContactLayer( depth )
        cutLayer     = viaLayer.getCut()
        cutSide      = cutLayer.getMinimalSize()
        cutSpacing   = cutLayer.getMinimalSpacing()
        if not cutSide:
            raise ErrorMessage( 1, 'BigVia._doCutMatrix(): Minimal side of cut layer "{}" is zero.' \
                                   .format( cutLayer.getName() ))
        if not cutSpacing:
            raise ErrorMessage( 1, 'BigVia._doCutMatrix(): Cut spacing on layer "{}" is zero.' \
                                   .format( cutLayer.getName() ))
        topEnclosure = max( viaLayer.getTopEnclosure( Layer.EnclosureH )
                          , viaLayer.getTopEnclosure( Layer.EnclosureV ))
        botEnclosure = max( viaLayer.getBottomEnclosure( Layer.EnclosureH )
                          , viaLayer.getBottomEnclosure( Layer.EnclosureV ))
        enclosure    = max( topEnclosure, botEnclosure )
        trace( 550, '\tBigVia: {}\n'.format(self) )
        trace( 550, '\t| topEnclosure[{}]: {}\n'.format(depth,DbU.getValueString(topEnclosure)) )
        trace( 550, '\t| botEnclosure[{}]: {}\n'.format(depth,DbU.getValueString(botEnclosure)) )
        trace( 550, '\t| enclosure   [{}]: {}\n'.format(depth,DbU.getValueString(enclosure)) )
        cutArea    = self.plates[ depth ].getBoundingBox()
        hEnclosure = enclosure + cutSide//2
        vEnclosure = hEnclosure
        if hEnclosure*2 > cutArea.getWidth():
            if self.flags & BigVia.AllowHorizontalExpand:
                hEnclosure = cutArea.getWidth()//2
            else:
                raise ErrorMessage( 1, [ 'BigVia._doCutMatrix(): Cannot create cut of {} in {}.' \
                                         .format( cutLayer.getName(), self )
                                       , 'Width is too small to fit a single VIA cut.' 
                                       ] )
        if vEnclosure*2 > cutArea.getHeight():
            if self.flags & BigVia.AllowVerticalExpand:
                vEnclosure = cutArea.getHeight()//2
            else:
                raise ErrorMessage( 1, [ 'BigVia._doCutMatrix(): Cannot create cut of {} in {}.' \
                                         .format( cutLayer.getName(), self )
                                       , 'Height is too small to fit a single VIA cut.' 
                                       ] )
        cutArea.inflate( -hEnclosure, -vEnclosure )
        xoffset = (cutArea.getWidth () % (cutSide+cutSpacing)) // 2
        yoffset = (cutArea.getHeight() % (cutSide+cutSpacing)) // 2
        cutArea.translate( xoffset, yoffset )
        self.vias[ depth ] = []
        y = cutArea.getYMin()
        while y <= cutArea.getYMax():
            x = cutArea.getXMin()
            self.vias[ depth ].append( [] )
            while x <= cutArea.getXMax():
                cut = Contact.create( self.net, cutLayer, x, y, cutSide, cutSide )
                self.vias[ depth ][ -1 ].append( cut )
                x += cutSide + cutSpacing
            y += cutSide + cutSpacing
        
