
# This file is part of the Coriolis Software.
# Copyright (c) SU 2014-2020, All Rights Reserved
#
# +-----------------------------------------------------------------+
# |                   C O R I O L I S                               |
# |      C u m u l u s  -  P y t h o n   T o o l s                  |
# |                                                                 |
# |  Author      :                    Jean-Paul CHAPUT              |
# |  E-mail      :            Jean-Paul.Chaput@lip6.fr              |
# | =============================================================== |
# |  Python      :       "./plugins/chip/corona.py"                 |
# +-----------------------------------------------------------------+


from   __future__ import print_function
import bisect
from   operator   import methodcaller  
import Cfg        
from   Hurricane  import DbU
from   Hurricane  import Point
from   Hurricane  import Interval
from   Hurricane  import Box
from   Hurricane  import Transformation
from   Hurricane  import Path
from   Hurricane  import Occurrence
from   Hurricane  import Net
from   Hurricane  import Contact
from   Hurricane  import Horizontal
from   Hurricane  import Vertical
from   Hurricane  import Pad
import CRL          
from   CRL             import RoutingLayerGauge
from   helpers         import trace
from   helpers.io      import ErrorMessage
from   helpers.io      import WarningMessage
from   helpers.overlay import UpdateSession
import plugins      
from   plugins         import StackedVia
import plugins.alpha.chip


plugins.alpha.chip.importConstants( globals() )


# --------------------------------------------------------------------
# Class  :  "corona.IntervalSet"

class IntervalSet ( object ):

    def __init__ ( self ):
        self.chunks = []
        return

    def merge ( self, min, max ):
        toMerge = Interval( min, max )
        imerge  = len(self.chunks)
        length  = len(self.chunks)
        i       = 0
        while i < length:
            if imerge >= length:
                if toMerge.getVMax() < self.chunks[i].getVMin():
                    self.chunks.insert( i, toMerge )
                    length += 1
                    imerge  = 0
                    break
                if toMerge.intersect(self.chunks[i]):
                    imerge = i
                    self.chunks[imerge].merge( toMerge )
            else:
                if toMerge.getVMax() >= self.chunks[i].getVMin():
                    self.chunks[imerge].merge( self.chunks[i] )
                    del self.chunks[ i ]
                    length -= 1
                    continue
                else:
                    break
            i += 1
        if imerge >= length:
            self.chunks.insert( length, toMerge )


# --------------------------------------------------------------------
# Class  :  "corona.Rail"

class Rail ( object ):

    def __init__ ( self, side, order, axis ):
        self.side  = side
        self.order = order
        self.axis  = axis
        self.vias  = {}    # Key:pos Element:[pos,railContact,blockContact]

    @property
    def net ( self ): return self.side.getRailNet(self.order)


# --------------------------------------------------------------------
# Class  :  "corona.HorizontalRail"

class HorizontalRail ( Rail ):

    def __init__ ( self, side, order, axis ):
        Rail.__init__( self, side, order, axis )
        trace( 550, '\t{}\n'.format(self) )

    def __str__ ( self ):
        return '<HorizontalRail "{}" ({}) @{}>'.format( self.side.getRailNet(self.order)
                                                      , self.order
                                                      , DbU.getValueString(self.axis) )

    def connect ( self, contact ):
        contactBb = contact.getBoundingBox()
        if    contactBb.getXMin() < self.side.innerBb.getXMin() \
           or contactBb.getXMax() > self.side.innerBb.getXMax():
            raise ErrorMessage( 1, [ '{} is outside rail/corona X range,'.format(contact)
                                  , 'power pad is likely to be to far off west or east.'
                                  , '(core:{})'.format(self.side.innerBb) ] ) 
        if self.vias.has_key(contact.getX()): return False
        keys = self.vias.keys()
        keys.sort()
        insertIndex = bisect.bisect_left( keys, contact.getX() )

        if len(keys) > 0:
            if insertIndex < len(keys):
                insertPosition = keys[ insertIndex ]
                if contactBb.getXMax() >= self.vias[insertPosition][2].getBoundingBox().getXMin():
                    return False
            if insertIndex > 0:
                if self.vias[keys[insertIndex-1]][2].getBoundingBox().getXMax() >= contactBb.getXMin():
                    return False
        self.vias[ contact.getX() ] = [ contact.getX()
                                      , StackedVia( self.net
                                                  , self.side.getLayerDepth(self.side.getHLayer())
                                                  , contact.getX()
                                                  , self.axis
                                                  , contact.getWidth()   - DbU.fromLambda(1.0)
                                                  , self.side.hRailWidth - DbU.fromLambda(1.0)
                                                  )
                                      , contact ]
        trace( 550, '\tADD "{}" contact "{}" @ [{} {}]\n' \
               .format( contact.getNet().getName()
                      , contact.getLayer().getName()
                      , DbU.getValueString(contact.getX())
                      , DbU.getValueString(self.axis)) )
        self.vias[ contact.getX() ][1].mergeDepth( self.side.getLayerDepth(contact.getLayer()) )
        return True

    def doLayout ( self ):
        trace( 550, ',+', '\tHorizontalRail.doLayout() order:{} @{}\n' \
                          .format( self.order,DbU.getValueString(self.axis )))
        railVias = [ self.side.corner0(self.order)
                   , self.side.corner1(self.order) ]
        for via in self.vias.values():
            if via[1].getNet() != via[2].getNet(): continue
            via[1].mergeDepth( self.side.getLayerDepth(self.side.getVLayer()) )
            via[1].doLayout()
            Vertical.create( via[1].getVia( via[2].getLayer() )
                           , via[2]
                           , via[2].getLayer()
                           , via[2].getX()
                           , via[2].getWidth()
                           )
            railVias.append( via[1].getVia( self.side.getVLayer()) )
        for i in range(1,len(railVias)):
            h = Horizontal.create( railVias[i-1]
                                 , railVias[i]
                                 , self.side.getHLayer()
                                 , self.axis
                                 , self.side.hRailWidth
                                 )
            trace( 550, '\t| {}\n'.format(h) )
        trace( 550, '-' )


# --------------------------------------------------------------------
# Class  :  "corona.VerticalRail"

class VerticalRail ( Rail ):

    def __init__ ( self, side, order, axis ):
        Rail.__init__( self, side, order, axis )
        trace( 550, '\t{}\n'.format(self) )

    def __str__ ( self ):
        return '<VerticalRail "{}" ({}) @{}>'.format( self.side.getRailNet(self.order)
                                                    , self.order
                                                    , DbU.getValueString(self.axis) )

    def doLayout ( self ):
        railVias = [ self.side.corner0(self.order)
                   , self.side.corner1(self.order) ]
        for via in self.vias.values():
            if via[1].getNet() != via[2].getNet(): continue
            via[1].doLayout()
            Horizontal.create( via[1].getVia( via[2].getLayer() )
                             , via[2]
                             , via[2].getLayer()
                             , via[2].getY()
                             , via[2].getHeight()
                             )
            railVias.append( via[1].getVia(self.side.getVLayer()) )
        railVias.sort( key=methodcaller('getY') )
        for i in range(1,len(railVias)):
            Vertical.create( railVias[i-1]
                           , railVias[i]
                           , self.side.getVLayer()
                           , self.axis
                           , self.side.vRailWidth
                           )

    def connect ( self, contact ):
        contactBb = contact.getBoundingBox()
        if    contactBb.getYMin() < self.side.innerBb.getYMin() \
           or contactBb.getYMax() > self.side.innerBb.getYMax():
            raise ErrorMessage( 1, [ '{} is outside rail/corona Y range'.format(contact)
                                   , 'power pad is likely to be to far off north or south.'
                                   , '(core:{})'.format(self.side.innerBb) ] ) 
        if self.vias.has_key(contact.getY()): return False
        trace( 550, ',+', '\tVerticalRail.connect() [{}] @{}\n'.format(self.order,DbU.toLambda(self.axis)) )
        trace( 550, '\t{}\n'.format(contact) )
        keys = self.vias.keys()
        keys.sort()
        insertIndex = bisect.bisect_left( keys, contact.getY() )
        trace( 550, ',+', '\tkeys:' )
        for key in keys:
            trace( 550, ' {}'.format(DbU.toLambda(key)) )
        trace( 550, '\n' )

        if len(keys) > 0:
            if insertIndex < len(keys):
                insertPosition = keys[ insertIndex ]
                trace( 550, '\tinsertIndex:{}'.format(insertIndex) )
                trace( 550, '\tCheck NEXT contactBb:{} via:{}\n' \
                            .format( contactBb
                                   , self.vias[insertPosition][2].getBoundingBox()) )
                if contactBb.getYMax() >= self.vias[insertPosition][2].getBoundingBox().getYMin():
                    trace( 550, ',--', '\tReject {} intersect NEXT\n'.format(contact) )
                    return False
            if insertIndex > 0:
                trace( 550, '\tcheck PREVIOUS contactBb:{} via:{}\n' \
                            .format( contactBb
                                   , self.vias[keys[insertIndex-1]][2].getBoundingBox()) )
                if self.vias[keys[insertIndex-1]][2].getBoundingBox().getYMax() >= contactBb.getYMin():
                    trace( 550, ',--', '\tReject {} intersect PREVIOUS\n'.format(contact) )
                    return False
        self.vias[ contact.getY() ] = [ contact.getY()
                                      , StackedVia( self.net
                                                  , self.side.getLayerDepth(self.side.getVLayer())
                                                  , self.axis
                                                  , contact.getY()
                                                  , self.side.vRailWidth - DbU.fromLambda(1.0)
                                                  , contact.getHeight()  - DbU.fromLambda(1.0)
                                                  )
                                      , contact ]
        trace(550, ',--' '\tADD {}\n'.format(contact) )
        self.vias[ contact.getY() ][1].mergeDepth( self.side.getLayerDepth(contact.getLayer()) )
        return True


# --------------------------------------------------------------------
# Class  :  "corona.Side"

class Side ( object ):

    def __init__ ( self, corona ):
        self.corona = corona

    @property
    def railsCount ( self ): return self.corona.railsCount

    @property           
    def innerBb ( self ): return self.corona.innerBb

    @property           
    def hRailWidth ( self ): return self.corona.hRailWidth

    @property           
    def hRailSpace ( self ): return self.corona.hRailSpace

    @property           
    def vRailWidth ( self ): return self.corona.vRailWidth

    @property           
    def vRailSpace ( self ): return self.corona.vRailSpace

    @property           
    def corners ( self ): return self.corona.corners

    @property
    def horizontalDepth ( self ): return self.corona.horizontalDepth

    @property
    def verticalDepth ( self ): return self.corona.verticalDepth

    @property
    def blockageNet ( self ): return self.corona.blockageNet

    def getLayerDepth   ( self, metal ): return self.corona.getLayerDepth(metal)
    def getRail         ( self, i ):     return self.rails[i]
    def getRailNet      ( self, i ):     return self.corona.getRailNet(i)
    def getHLayer       ( self ):        return self.corona.getHLayer()
    def getVLayer       ( self ):        return self.corona.getVLayer()

    def getRailAxis ( self, i ):
        raise ErrorMessage( 1, 'Side.getRailAxis(): Must never be called on base class.' )

    def getInnerRail ( self, i ):
        if i >= len(self.rails):
            raise ErrorMessage( 1, 'Side.getInnerRail(): no rail {} (only:{}).' \
                                   .format(i,len(self.rails)) )
        return self.rails[i]

    def getOuterRail ( self, i ):
        if i >= len(self.rails):
            raise ErrorMessage( 1, 'Side.getOuterRail(): no rail {} (only: {}).' \
                                   .format(i,len(self.rails)) )
        return self.rails[-(i+1)]

    def connect ( self, blockSide ):
        for terminal in blockSide.terminals:
            trace( 550, '\tterminal:{}\n'.format(terminal) )
            for rail in self.rails:
                rail.connect( terminal[1] )

    def connectPads ( self, padSide ):
        for contact in padSide.pins:
            if contact.getNet().isClock():
                for i in range(len(self.rails)):
                    trace( 550, '\tConnect to [-{}] @{}\n'.format(i, DbU.toLambda(self.getOuterRail(i).axis)) )
                    self.getOuterRail(i).connect( contact )
            elif contact.getNet().isSupply():
                self.getOuterRail( 0 ).connect( contact )
        halfRails = (len(self.rails)-1)/2
        trace( 550, '\thalfRails:{}\n'.format(halfRails) )
        for contact in padSide.pins:
            if not contact.getNet().isSupply() and not contact.getNet().isClock(): continue
            trace( 550, ',+', '\tConnect pad contact {}\n'.format(contact) )
            for i in range(halfRails):
                trace( 550, '\tConnect to [-{}] @{}\n'.format(i, DbU.toLambda(self.getOuterRail(i).axis)) )
                self.getOuterRail(i).connect( contact )
            trace( 550, '-' )

    def doLayout ( self ):
        for rail in self.rails: rail.doLayout()


# --------------------------------------------------------------------
# Class  :  "corona.HorizontalSide"

class HorizontalSide ( Side ):

    def __init__ ( self, corona ):
        Side.__init__( self, corona )
        self.rails = []
        for i in range(self.railsCount):
            self.rails.append( HorizontalRail(self,i,self.getRailAxis(i)) )
        return


# --------------------------------------------------------------------
# Class  :  "corona.SouthSide"

class SouthSide ( HorizontalSide ):

    def __init__ ( self, corona ):
        HorizontalSide.__init__( self, corona )
        return

    def getRailAxis ( self, i ):
        return self.innerBb.getYMin() -    self.hRailWidth/2 - self.hRailSpace \
                                      - i*(self.hRailWidth   + self.hRailSpace)

    def corner0 ( self, i ): return self.corners[SouthWest][i]
    def corner1 ( self, i ): return self.corners[SouthEast][i]


# --------------------------------------------------------------------
# Class  :  "corona.NorthSide"

class NorthSide ( HorizontalSide ):

    def __init__ ( self, corona ):
        HorizontalSide.__init__( self, corona )
        return

    def getRailAxis ( self, i ):
        return self.innerBb.getYMax() +    self.hRailWidth/2 + self.hRailSpace \
                                      + i*(self.hRailWidth   + self.hRailSpace)

    def corner0 ( self, i ): return self.corners[NorthWest][i]
    def corner1 ( self, i ): return self.corners[NorthEast][i]


# --------------------------------------------------------------------
# Class  :  "corona.VerticalSide"

class VerticalSide ( Side ):

    def __init__ ( self, corona ):
        Side.__init__( self, corona )

        self.rails = []
        for i in range(self.railsCount):
            self.rails.append( VerticalRail(self,i,self.getRailAxis(i)) )
        return

    def addBlockages ( self, sideXMin, sideXMax ):
        spans = IntervalSet()
        for rail in self.rails:
            for via in rail.vias.values():
                if via[1].getNet() != via[2].getNet(): continue
                spans.merge( via[1]._y - via[1]._height/2, via[1]._y + via[1]._height/2 )
        routingGauge = self.corona.routingGauge
        for depth in range(self.getInnerRail(0).vias.values()[0][1].bottomDepth
                          ,self.getInnerRail(0).vias.values()[0][1].topDepth ):
            blockageLayer = routingGauge.getRoutingLayer(depth).getBlockageLayer()
            pitch         = routingGauge.getLayerPitch(depth)
            for chunk in spans.chunks:
                Horizontal.create( self.blockageNet
                                 , blockageLayer
                                 , (chunk.getVMax() + chunk.getVMin())/2
                                 ,  chunk.getVMax() - chunk.getVMin() + pitch*2
                                 , sideXMin
                                 , sideXMax
                                 )
            depth -= 2
            if depth > 0:
                blockageLayer = routingGauge.getRoutingLayer(depth).getBlockageLayer()
                pitch         = routingGauge.getLayerPitch(depth)
                for chunk in spans.chunks:
                    Horizontal.create( self.blockageNet
                                     , blockageLayer
                                     , (chunk.getVMax() + chunk.getVMin())/2
                                     ,  chunk.getVMax() - chunk.getVMin() + pitch*2
                                     , sideXMin
                                     , sideXMax
                                     )


# --------------------------------------------------------------------
# Class  :  "corona.WestSide"

class WestSide ( VerticalSide ):

    def __init__ ( self, corona ):
        VerticalSide.__init__( self, corona )

    def getRailAxis ( self, i ):
        return self.innerBb.getXMin() -    self.vRailWidth/2 - self.vRailSpace \
                                      - i*(self.vRailWidth   + self.vRailSpace)

    def corner0 ( self, i ): return self.corners[SouthWest][i]
    def corner1 ( self, i ): return self.corners[NorthWest   ][i]

    def addBlockages ( self ):
        sideXMin = self.getOuterRail(0).axis - self.vRailWidth
        sideXMax = self.getInnerRail(0).axis + self.vRailWidth
        VerticalSide.addBlockages( self, sideXMin, sideXMax )


# --------------------------------------------------------------------
# Class  :  "corona.EastSide"

class EastSide ( VerticalSide ):

    def __init__ ( self, corona ):
        VerticalSide.__init__( self, corona )

    def getRailAxis ( self, i ):
        return self.innerBb.getXMax() +    self.vRailWidth/2 + self.vRailSpace \
                                      + i*(self.vRailWidth   + self.vRailSpace)

    def corner0 ( self, i ): return self.corners[SouthEast][i]
    def corner1 ( self, i ): return self.corners[NorthEast   ][i]

    def addBlockages ( self ):
        sideXMin = self.getInnerRail(0).axis - self.vRailWidth
        sideXMax = self.getOuterRail(0).axis + self.vRailWidth
        VerticalSide.addBlockages( self, sideXMin, sideXMax )


# --------------------------------------------------------------------
# Class  :  "corona.Builder"

class Builder ( object ):

    def __init__ ( self, block ):
        self.block      = block
        self.innerBb    = self.block.bb
        print( 'Builder.__init__(): innerBb: {}'.format(self.innerBb) )
        print( self.block.path.getTransformation())
        self.block.path.getTransformation().applyOn( self.innerBb )
        self.innerBb.inflate( self.hRailSpace/2, self.vRailSpace/2 )
        self.southSide  = SouthSide( self )
        self.northSide  = NorthSide( self )
        self.westSide   = WestSide ( self )
        self.eastSide   = EastSide ( self )

    @property
    def conf ( self ): return self.block.conf

    @property
    def routingGauge ( self ): return self.conf.routingGauge

    @property
    def topLayerDepth ( self ): return self.conf.topLayerDepth

    @property
    def horizontalDepth ( self ): return self.conf.horizontalDepth

    @property
    def verticalDepth ( self ): return self.conf.verticalDepth

    @property
    def blockageNet ( self ): return self.conf.blockageNet

    @property
    def railsCount ( self ): return self.conf.railsCount

    @property
    def hRailWidth ( self ): return self.conf.cfg.chip.block.rails.hWidth

    @property
    def vRailWidth ( self ): return self.conf.cfg.chip.block.rails.vWidth

    @property
    def hRailSpace ( self ): return self.conf.cfg.chip.block.rails.hSpacing

    @property
    def vRailSpace ( self ): return self.conf.cfg.chip.block.rails.vSpacing

    def getLayerDepth ( self, metal ):
        return self.conf.routingGauge.getLayerDepth( metal )

    def getRailNet ( self, i ):
        if self.conf.useClockTree and i == 0: return self.conf.coronaCk
        if i % 2: return self.conf.coronaVss
        return self.conf.coronaVdd

    def getHLayer ( self ):
        return self.routingGauge.getLayerGauge( self.horizontalDepth ).getLayer()

    def getVLayer ( self ):
        return self.routingGauge.getLayerGauge( self.verticalDepth ).getLayer()

    def connectCore ( self ):
        for plane in self.block.planes.values():
            for side in plane.sides.values():
                self.southSide.connect( side[South] )
                self.northSide.connect( side[North] )
                self.westSide .connect( side[West ] )
                self.eastSide .connect( side[East ] )

    def connectPads ( self, padsCorona ):
        self.southSide.connectPads( padsCorona.southSide )
        self.northSide.connectPads( padsCorona.northSide )
        self.eastSide .connectPads( padsCorona.eastSide  )
        self.westSide .connectPads( padsCorona.westSide  )
        return

    def doLayout ( self ):
        self.corners = { SouthWest : []
                       , SouthEast : []
                       , NorthWest : []
                       , NorthEast : []
                       }
        contactDepth = self.horizontalDepth
        if self.horizontalDepth > self.verticalDepth:
            contactDepth = self.verticalDepth
        with UpdateSession():
            blBox = Box()
            brBox = Box()
            tlBox = Box()
            trBox = Box()
            for i in range(self.railsCount):
                xBL = self.westSide .getRail(i).axis
                yBL = self.southSide.getRail(i).axis
                xTR = self.eastSide .getRail(i).axis
                yTR = self.northSide.getRail(i).axis
                net = self.getRailNet( i )
                blBox.merge( xBL, yBL )
                brBox.merge( xTR, yBL )
                tlBox.merge( xBL, yTR )
                trBox.merge( xTR, yTR )
                self.routingGauge.getContactLayer(contactDepth)
                self.corners[SouthWest].append( 
                    Contact.create( net
                                  , self.routingGauge.getContactLayer(contactDepth)
                                  , xBL, yBL
                                  , self.hRailWidth
                                  , self.vRailWidth
                                  ) )
                self.corners[NorthWest].append( 
                    Contact.create( net
                                  , self.routingGauge.getContactLayer(contactDepth)
                                  , xBL, yTR
                                  , self.hRailWidth
                                  , self.vRailWidth
                                  ) )
                self.corners[SouthEast].append( 
                    Contact.create( net
                                  , self.routingGauge.getContactLayer(contactDepth)
                                  , xTR, yBL
                                  , self.hRailWidth
                                  , self.vRailWidth
                                  ) )
                self.corners[NorthEast].append( 
                    Contact.create( net
                                  , self.routingGauge.getContactLayer(contactDepth)
                                  , xTR, yTR
                                  , self.hRailWidth
                                  , self.vRailWidth
                                  ) )
            self.southSide.doLayout()
            self.northSide.doLayout()
            self.westSide .doLayout()
            self.eastSide .doLayout()
            self.westSide.addBlockages()
            self.eastSide.addBlockages()
            blBox.inflate( self.hRailWidth, self.vRailWidth )
            brBox.inflate( self.hRailWidth, self.vRailWidth )
            tlBox.inflate( self.hRailWidth, self.vRailWidth )
            trBox.inflate( self.hRailWidth, self.vRailWidth )
            for depth in range( 1, self.conf.topLayerDepth + 1 ):
                blockageLayer = self.routingGauge.getRoutingLayer(depth).getBlockageLayer()
                Pad.create( self.blockageNet, blockageLayer, blBox )
                Pad.create( self.blockageNet, blockageLayer, brBox )
                Pad.create( self.blockageNet, blockageLayer, tlBox )
                Pad.create( self.blockageNet, blockageLayer, trBox )
