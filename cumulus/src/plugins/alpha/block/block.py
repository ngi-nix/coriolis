#
# This file is part of the Coriolis Software.
# Copyright (c) SU 2020-2020, All Rights Reserved
#
# +-----------------------------------------------------------------+
# |                   C O R I O L I S                               |
# |      C u m u l u s  -  P y t h o n   T o o l s                  |
# |                                                                 |
# |  Author      :                    Jean-Paul CHAPUT              |
# |  E-mail      :            Jean-Paul.Chaput@lip6.fr              |
# | =============================================================== |
# |  Python      :       "./plugins/block/block.py"                 |
# +-----------------------------------------------------------------+


from   __future__ import print_function
import sys
import os.path
import Cfg
from   Hurricane import Breakpoint
from   Hurricane import DbU
from   Hurricane import Box
from   Hurricane import Transformation
from   Hurricane import Point
from   Hurricane import Box
from   Hurricane import Path
from   Hurricane import Layer
from   Hurricane import Occurrence
from   Hurricane import Net
from   Hurricane import NetExternalComponents
from   Hurricane import RoutingPad
from   Hurricane import Pad
from   Hurricane import Horizontal
from   Hurricane import Vertical
from   Hurricane import Contact
from   Hurricane import Pin
from   Hurricane import Plug
from   Hurricane import Instance
import CRL
from   CRL             import RoutingLayerGauge
from   helpers         import trace
from   helpers         import dots
from   helpers.io      import ErrorMessage
from   helpers.io      import WarningMessage
from   helpers.io      import catch
from   helpers.overlay import UpdateSession
import Etesian
import Anabatic
import Katana
import plugins.rsave
from   plugins                   import getParameter
from   alpha.block               import timing
from   alpha.block.spares        import Spares
from   alpha.block.clocktree     import ClockTree
#from   alpha.block.hfns1         import BufferTree
#from   alpha.block.hfns2         import BufferTree
from   alpha.block.hfns3         import BufferTree
from   alpha.block.configuration import IoPin
from   alpha.block.configuration import BlockConf
from   alpha.block.configuration import GaugeConf

timing.staticInit()


# ----------------------------------------------------------------------------
# Class  :  "block.Side".

class Side ( object ):
    """
    Collate the informations about all the IoPin on a side of a block
    and perform pins creation & placement.
    """

    def __init__ ( self, conf, side ):
        self.conf       = conf
        self.side       = side
        self.pinSpecs   = []
        self.expandPins = True
        self.pins       = {}

    @property
    def pinDirection ( self ):
        """Translate the IoPin side into a Pin direction."""
        if   self.side & IoPin.WEST:  return Pin.Direction.WEST
        elif self.side & IoPin.EAST:  return Pin.Direction.EAST
        elif self.side & IoPin.SOUTH: return Pin.Direction.SOUTH
        else:                         return Pin.Direction.NORTH

    def destroy ( self ):
       #with UpdateSession():
       #    for pinsAtPos in self.pins.values():
       #        for pin in pinsAtPos:
       #            pin.destroy()
        self.pins = {}

    def setupAb ( self ):
        """
        Initialise the side coordinate from the block abutmeent box.
        Needless to say that it must be called *after* the abutment
        box has been setup.
        """
        if self.side & IoPin.WEST:
            self.gauge   = self.conf.hDeepRG
            self.ubegin  = self.conf.yMin
            self.uend    = self.conf.yMax
            self.sidePos = self.conf.xMin
        elif self.side & IoPin.EAST:
            self.gauge   = self.conf.hDeepRG
            self.ubegin  = self.conf.yMin
            self.uend    = self.conf.yMax
            self.sidePos = self.conf.xMax
        elif self.side & IoPin.SOUTH:
            self.gauge   = self.conf.vDeepRG
            self.ubegin  = self.conf.xMin
            self.uend    = self.conf.xMax
            self.sidePos = self.conf.yMin
        elif self.side & IoPin.NORTH:
            self.gauge   = self.conf.vDeepRG
            self.ubegin  = self.conf.xMin
            self.uend    = self.conf.xMax
            self.sidePos = self.conf.yMax

    def getNextPinPosition ( self, flags, upos, ustep ):
        """
        Return the point of the next available Pin position on
        the side.

        :param flags: Whether to find the next position from the
                      begin of the side (A_BEGIN) or the end (A_END).
        :param upos:  Force the position to use, in that cas the
                      ``flags`` argument is  ignored. To let the
                      function choose, must be set to zero.
        :param ustep: The distance to space from the last created Pin.
        """
        if not upos:
            if not ustep: ustep = self.gauge.getPitch()
            while True:
                if flags & IoPin.A_BEGIN:
                    self.ubegin += ustep
                    pinOffset = self.ubegin
                    if not self.pins.has_key(self.ubegin):
                        break
                else: 
                    self.uend -= ustep
                    pinOffset = self.uend
                    if not self.pins.has_key(self.uend):
                        break
        else:
            pinOffset = upos
        if (self.side & IoPin.WEST) or (self.side & IoPin.EAST):
            return Point( self.sidePos, pinOffset )
        return Point( pinOffset, self.sidePos )

    def append ( self, pin ):
        """
        Append a newly created Pin to a map indexed by the position along
        the side. Create a list for each entry. If everything goes well
        each list will exacly one entry.

        This function is still too simple. It will consider two Pin of
        different layers as overlapping.
        """
        if (self.side & IoPin.WEST) or (self.side & IoPin.EAST):
            upos = pin.getY()
        else:
            upos = pin.getX()
        if not self.pins.has_key(upos):
            self.pins[upos] = [ pin ]
        else:
            self.pins[upos].append( pin )

    def place ( self, ioPin ):
        """
        Performs the actual creation of the Pin from it's ioPin specification.
        Should be called prior to any call to ```Etesian.place()```, so the Pin
        is taken into account when building the RoutingPads. Returns the number
        of *failed* pins, so zero means that all went well... Currently only
        check for out of bounds coordinates.
        """

        status = 0
        if self.side & (IoPin.NORTH | IoPin.SOUTH):
            gauge = self.conf.vDeepRG
            upos  = ioPin.upos
            for index in ioPin.indexes:
                pinName  = ioPin.stem.format( index )
                net      = self.conf.cell.getNet( pinName )
                if net is None:
                    print( ErrorMessage( 1, [ 'Side.place(IoPin): No net named "{}".'.format(pinName) ] ))
                    continue
                if net.isClock() and self.conf.useClockTree:
                    print( WarningMessage( 'Side.place(IoPin): Skipping clock IoPin "{}".'.format(pinName) ))
                    continue
                pinName += '.{}'.format(self.conf.getIoPinsCounts(net))
                pinPos   = self.getNextPinPosition( ioPin.flags, upos, ioPin.ustep )
                if pinPos.getX() > self.conf.xMax or pinPos.getX() < self.conf.xMin:
                    print( ErrorMessage( 1, [ 'Side.place(IoPin): Pin "{}" is outside north or south abutment box side.' \
                                              .format(pinName)
                                            , '(x:{}, xAB: [{}:{}])' \
                                              .format( DbU.getValueString(pinPos.getX())
                                                     , DbU.getValueString(self.conf.xMin)
                                                     , DbU.getValueString(self.conf.xMax) ) ] ))
                    status += 1
                trace( 550, '\tIoPin.place() N/S @{} "{}" of "{}".\n'.format(pinPos,pinName,net) )
                pin = Pin.create( net
                                , pinName
                                , self.pinDirection
                                , Pin.PlacementStatus.FIXED
                                , gauge.getLayer()
                                , pinPos.getX()
                                , pinPos.getY()
                                , gauge.getWireWidth()
                                , gauge.getWireWidth() / 2
                                )
                NetExternalComponents.setExternal( pin )
                self.append( pin )
                self.conf.incIoPinsCounts( net )
                if upos: upos += ioPin.ustep
        else:
            gauge = self.conf.hDeepRG
            upos  = ioPin.upos
            for index in ioPin.indexes:
                pinName  = ioPin.stem.format(index)
                net      = self.conf.cell.getNet( pinName )
                if net is None:
                    print( ErrorMessage( 1, [ 'Side.place(IoPin): No net named "{}".'.format(pinName) ] ))
                    continue
                pinName += '.{}'.format(self.conf.getIoPinsCounts(net))
                pinPos   = self.getNextPinPosition( ioPin.flags, upos, ioPin.ustep )
                if pinPos.getY() > self.conf.yMax or pinPos.getY() < self.conf.yMin:
                    print( ErrorMessage( 1, [ 'Side.place(IoPin): Pin "{}" is outside east or west abutment box side.' \
                                              .format(pinName)
                                              , '(y:{}, yAB: [{}:{}])' \
                                              .format( DbU.getValueString(pinPos.getY())
                                                     , DbU.getValueString(self.conf.yMin)
                                                     , DbU.getValueString(self.conf.yMax)) ] ))
                    status += 1
                trace( 550, '\tIoPin.place() E/W @{} "{}" of "{}".\n'.format(pinPos,pinName,net) )
                pin = Pin.create( net
                                , pinName
                                , self.pinDirection
                                , Pin.PlacementStatus.FIXED
                                , gauge.getLayer()
                                , pinPos.getX()
                                , pinPos.getY()
                                , gauge.getWireWidth() / 2
                                , gauge.getWireWidth()
                                )
                NetExternalComponents.setExternal( pin )
                self.append( pin )
                self.conf.incIoPinsCounts( net )
                if upos: upos += ioPin.ustep
        return status

    def expand ( self ):
        """
        After, and only after routing the block, move the I/O pins *outside*
        of the abutment box. THey will stick out for one pitch.
        """
        if not self.expandPins: return
        rg  = self.conf.routingGauge
        for pinsAtPos in self.pins.values():
            for pin in pinsAtPos:
                for lg in rg.getLayerGauges():
                    if lg.getLayer().getMask() == pin.getLayer().getMask():
                        offset = lg.getPitch()
                        if   self.side & IoPin.WEST:  pin.setX( pin.getDx()-offset )
                        elif self.side & IoPin.EAST:  pin.setX( pin.getDx()+offset )
                        elif self.side & IoPin.SOUTH: pin.setY( pin.getDy()-offset )
                        elif self.side & IoPin.NORTH: pin.setY( pin.getDy()+offset )

    def checkOverlaps ( self ):
        """
        Check for Pin overlap. For now we are only checking that the are not at the
        exact same position. We should check for neighboring overlap (Pin too close
        from each other, not only exactly at the position).
        """
        if   self.side & IoPin.EAST:  sideName = 'EAST'
        elif self.side & IoPin.WEST:  sideName = 'WEST'
        elif self.side & IoPin.SOUTH: sideName = 'SOUTH'
        elif self.side & IoPin.NORTH: sideName = 'NORTH'
        for upos in self.pins.keys():
            count = len(self.pins[upos])
            if count > 1:
                pinNames = self.pins[upos][0].getName()
                for pin in self.pins[upos][1:]:
                    pinNames += ', ' + pin.getName()
                print( ErrorMessage( 1, [ 'Side.checkOverlap(): On {} side of block "{}", {} pins ovelaps.' \
                                          .format(sideName,self.conf.cell.getName(),count)
                                        , '(@{}: {})' \
                                          .format(DbU.getValueString(upos),pinNames) ] ) )


# ----------------------------------------------------------------------------
# Class  :  "block.Block".

class Block ( object ):
    """
    The workhorse for turning a Cell into a hierarchically reusable placed and
    routed block. Provide support for:

    * Clock tree.
    """

    LUT = {}

    @staticmethod
    def lookup ( cell ):
        if Block.LUT.has_key(cell): return Block.LUT[cell]
        return None

    def __init__ ( self, conf ):
        """
        Create a Block object. The only parameter ``conf`` must be a BlockConf
        object which contains the complete block configuration.
        """
        self.flags          = 0
        self.conf           = conf
        self.spares         = Spares( self )
        self.clockTrees     = []
        self.hfnTrees       = []
        self.blockInstances = []
        self.sides          = { IoPin.WEST  : Side( self.conf, IoPin.WEST  )
                              , IoPin.EAST  : Side( self.conf, IoPin.EAST  )
                              , IoPin.SOUTH : Side( self.conf, IoPin.SOUTH )
                              , IoPin.NORTH : Side( self.conf, IoPin.NORTH )
                              }
        if not self.conf.cell.getAbutmentBox().isEmpty():
            print( '  o  Block "{}" is already done, reusing layout.' \
                   .format(self.conf.cell.getName()) )
            self.conf.cell.setTerminalNetlist( True )
            self.conf.isBuilt = True
        else:
            print( '  o  Block "{}" will be generated.' \
                   .format(self.conf.cell.getName()) )
        Block.LUT[ self.conf.cell ] = self

    def setUnexpandPins ( self, sides ):
        """
        Prevent Pins from the selected sides to be stick out of one pitch.
        This may be useful if you plan to route by abutment on a side.
        """
        for side in (IoPin.WEST, IoPin.EAST, IoPin.SOUTH, IoPin.NORTH):
            if sides & side: self.sides[side].expandPins = False

    def setupAb ( self ):
        """
        Compute and set the Block's abutment box. Three cases:
        1. The AB is already set. assume the user as already set it by itself
           so do nothing.
        2. The block has *routed* sub-blocks. Then the width and height of the
           AB must be specified, and the placement of the sub-blocks, as the
           placer cannot guess them. P&R block are placed and any unplaced
           block has it's AB & position set to be the same as the one of the
           top cell.
        3. Otherwise, let the placer (Etesian) compute the size accorging the
           various configuration parameters (aspect ratio, space margin, fixed
           height or width, ...).
        """
        trace( 550, '\tBlockConf.setupAb() {}\n'.format(self.conf.cell) )
        if not self.conf.cell.getAbutmentBox().isEmpty():
            pass
        elif not self.conf.coreAb.isEmpty():
            self.conf.core.setAbutmentBox( self.conf.coreAb )
        elif len(self.blockInstances):
            with UpdateSession():
                ab = Box( 0, 0, self.conf.fixedWidth, self.conf.fixedHeight )
                self.conf.cell.setAbutmentBox( ab )
                for occurrence in self.conf.cell.getNonTerminalNetlistInstanceOccurrences():
                    instance = occurrence.getEntity()
                    subCell  = instance.getMasterCell()
                    subCell.setAbutmentBox( ab )
                for occurrence in self.conf.cell.getNonTerminalNetlistInstanceOccurrences():
                    instance = occurrence.getEntity()
                    instance.setTransformation( Transformation() )
                for blockInstance in self.blockInstances:
                    blockInstance.place()
        else:
            sysSpaceMargin   = self.conf.cfg.etesian.spaceMargin
            blockSpaceMargin = sysSpaceMargin + self.spares.getSpareSpaceMargin()
            self.conf.cfg.etesian.spaceMargin = blockSpaceMargin
            self.conf.cfg.apply()
            with UpdateSession():
                etesian = Etesian.EtesianEngine.create( self.conf.cell )
                if self.conf.fixedWidth:  etesian.setFixedAbWidth ( self.conf.fixedWidth  )
                if self.conf.fixedHeight: etesian.setFixedAbHeight( self.conf.fixedHeight )
                etesian.setDefaultAb()
                etesian.destroy()
            self.conf.cfg.etesian.spaceMargin = sysSpaceMargin
            self.conf.cfg.apply()
        ab = self.conf.cell.getAbutmentBox()
        self.conf.coreSize = (ab.getWidth(), ab.getHeight())
        trace( 550, '\tSetting core ab from Cell:{}\n'.format(self.conf.coreAb) )
        for side in self.sides.values(): side.setupAb()
        trace( 550, '\tCORE AB is {}\n'.format(self.conf.cell.getAbutmentBox()) )
        if self.conf.isCoreBlock:
            self.conf.setupICore()

    def addClockTrees ( self ):
        """Create the trunk of all the clock trees (recursive H-Tree)."""
        print( '  o  Building clock tree(s).' )
        af = CRL.AllianceFramework.get()
        clockNets = []
        for net in self.conf.cellPnR.getNets():
            if af.isCLOCK(net.getName()): 'CLOCK: {}'.format(net)
            if net.isClock():
                trace( 550, '\tBlock.addClockTrees(): Found clock {}.\n'.format(net) )
                clockNets.append( net )
        if not clockNets:
            raise ErrorMessage( 3, 'Block.clockTree(): Cell "{}" has no clock net(s).'.format(self.conf.cell.getName()) )
        with UpdateSession():
            for clockNet in clockNets:
                print( '     - "{}".'.format(clockNet.getName()) )
                trace( 550, ',+', '\tBlock.addClockTrees(): Build clock tree for {}.\n'.format(clockNet) )
                self.clockTrees.append( ClockTree(self.spares,clockNet,len(self.clockTrees)) )
                self.clockTrees[-1].buildHTree()
                trace( 550, '-' )

    def splitClocks ( self ):
        """
        Break the clock net and attach all it's Pins to the closest leaf
        if the H-Tree.
        """
        for clockTree in self.clockTrees:
            clockTree.splitClock()

    def findHfnTrees ( self ):
        """Create the trunk of all the high fanout nets."""
        print( '  o  Building high fanout nets trees.' )
        if self.spares:
            maxSinks = timing.tech.getSinksEstimate( self.conf.bufferConf.name )
            dots( 82
                , '     -  Max sinks for buffer "{}"'.format(self.conf.bufferConf.name)
                , ' {}'.format(maxSinks) )
            nets = []
            for net in self.conf.cell.getNets():
                sinksCount = 0
                for rp in net.getRoutingPads(): sinksCount += 1
                if sinksCount > maxSinks:
                    nets.append( (net,sinksCount) )
            with UpdateSession():
                for net,sinksCount in nets:
                    trace( 550, '\tBlock.addHfnTrees(): Found high fanout net "{}" ({} sinks).\n' \
                                .format(net.getName(),sinksCount) )
                   #if not net.getName().startswith('alu_m_muls_b(1)'): continue
                   #if not net.getName().startswith('abc_75177_new_n12236'): continue
                    sys.stderr.flush()
                    print( '     - "{}", {} sinks.'.format(net.getName(),sinksCount) )
                    sys.stdout.flush()
                    self.hfnTrees.append( BufferTree( self.spares, net ) )
                    self.hfnTrees[-1].buildBTree()
                self.spares.rshowPoolUse()
           #Breakpoint.stop( 0, 'block.findHfnTrees() done.' )
        else:
            print( '     (No spares buffers, disabled)' )
        return len(self.hfnTrees)

    def addHfnBuffers ( self ):
        for hfnTree in self.hfnTrees:
            hfnTree.rcreateBuffer()

    def placeIoPins ( self ):
        """
        Place the Pins on all the sides. Raise an exception in case of failure.
        (mainly due to Pins outside the side range)
        """
        faileds = 0
        with UpdateSession():
            for ioPin in self.conf.ioPins:
                if   ioPin.flags & IoPin.SOUTH: side = self.sides[IoPin.SOUTH]
                elif ioPin.flags & IoPin.NORTH: side = self.sides[IoPin.NORTH]
                elif ioPin.flags & IoPin.EAST:  side = self.sides[IoPin.EAST ]
                else:                           side = self.sides[IoPin.WEST ]
                faileds += side.place( ioPin )
        if faileds:
            raise ErrorMessage( 3, 'Block.placeIoPins(): Cell "{}" has {} badly placed pins.' \
                                   .format(self.conf.cell.getName(),faileds) )

    def checkIoPins ( self ):
        """
        Check for Pin overlap and that all external Net have at least one Pin.
        """
        for side in self.sides.values():
            side.checkOverlaps()
        for net in self.conf.cell.getNets():
            if not net.isExternal(): continue
            if net.isSupply(): continue
            hasPins = False
            for pin in net.getPins():
                hasPins = True
                break
            if not hasPins:
                print( ErrorMessage( 1, 'Block.checkIoPins() External net "{}" has no pin(s).' \
                                        .format(net.getName() )))

    def expandIoPins ( self ):
        """
        Stick out the block I/O pins of one pitch on all sides.
        See setUnexpandPins() to prevent it to happen on a specific side.
        """
        with UpdateSession():
            for side in self.sides.values():
                side.expand()

    def place ( self ):
        if self.conf.isCoreBlock:
            etesian = Etesian.EtesianEngine.create( self.conf.corona )
            etesian.setBlock( self.conf.icore )
        else:
            etesian = Etesian.EtesianEngine.create( self.conf.cell )
        etesian.place()
        etesian.destroy()

    def route ( self ):
        routedCell = self.conf.corona if self.conf.isCoreBlock else self.conf.cell
        katana = Katana.KatanaEngine.create( routedCell )
       #katana.printConfiguration   ()
        katana.digitalInit          ()
       #katana.runNegociatePreRouted()
       #Breakpoint.stop( 0, 'Block.route() Before global routing.' )
        katana.runGlobalRouter      ( Katana.Flags.NoFlags )
        katana.loadGlobalRouting    ( Anabatic.EngineLoadGrByNet )
       #Breakpoint.stop( 0, 'Block.route() After global routing.' )
        katana.layerAssign          ( Anabatic.EngineNoNetLayerAssign )
        katana.runNegociate         ( Katana.Flags.NoFlags )
        success = katana.isDetailedRoutingSuccess()
       #Breakpoint.stop( 0, 'Block.route() done, success:{}.'.format(success) )
        katana.finalizeLayout()
        katana.destroy()
        return success

    def addBlockages ( self ):
        with UpdateSession():
            net = self.conf.cell.getNet( 'blockagenet' )
            ab  = self.conf.cell.getAbutmentBox()
            rg  = self.conf.routingGauge
            for lg in rg.getLayerGauges():
                if lg.getType() == RoutingLayerGauge.PinOnly: continue
                blockage = lg.getBlockageLayer()
                if not blockage: continue
                if lg.getDirection() == RoutingLayerGauge.Horizontal:
                    dxBorder = lg.getPitch() + lg.getWireWidth()
                    Horizontal.create( net
                                     , blockage
                                     , ab.getCenter().getY()
                                     , ab.getHeight()
                                     , ab.getXMin() + dxBorder
                                     , ab.getXMax() - dxBorder
                                     )
                else:
                    dyBorder = lg.getPitch() + lg.getWireWidth()
                    Vertical.create( net
                                   , blockage
                                   , ab.getCenter().getX()
                                   , ab.getWidth()
                                   , ab.getYMin() + dyBorder
                                   , ab.getYMax() - dyBorder
                                   )

    def doPnR ( self ):
        """
        Perform all the steps required to build the layout of the block.
        The first step is to build all the blockInstance it depends upon,
        so they will appear as ``NetListTerminal`` and we can place them
        in their parent cell.
        """
        editor = self.conf.editor
        print( '  o  Builing block "{}".'.format(self.conf.cell.getName()) )
        for blockInstance in self.blockInstances:
            blockInstance.block.conf.editor = editor
            if not blockInstance.block.conf.isBuilt:
                print( '     - Build sub-block "{}".' \
                       .format(blockInstance.block.conf.cell.getName()) )
                blockInstance.block.build()
        if editor: editor.setCell( self.conf.cell )
        self.conf.cfg.apply()
        iteration = -1
        while True:
            iteration += 1
            if iteration > 0: break
            self.setupAb()
            if not self.conf.isCoreBlock:
                self.placeIoPins()
                self.checkIoPins()
            self.spares.build()
            if self.conf.useClockTree: self.addClockTrees()
            self.addHfnBuffers()
            if editor: editor.fit()
           #Breakpoint.stop( 0, 'Clock tree(s) done.' )
            self.place()
            self.findHfnTrees()
            break
        if self.conf.useClockTree: self.splitClocks()
        if self.conf.isCoreBlock:  self.doConnectCore()
        self.spares.removeUnusedBuffers()
        status = self.route()
        if not self.conf.isCoreBlock:
            self.addBlockages()
            self.expandIoPins()
        self.conf.isBuilt = True
        return status

    def useBlockInstance ( self, instancePathName , transf ):
        """
        Make a sub-block known to the top block. Can be trans-hierarchical.

        :param instancePathName: A string of instances names, separated by
                                 dots ('.') pinpointing the instance of the
                                 block.
        :param transf:           The position to place the instance of the
                                 sub-block.

        Prior to calling this function, a block *must* have been created for
        the master cell of the instance. But it need not to have been placed
        and routed.
        """
        iNames = instancePathName.split('.')
        path   = Path()
        for iName in iNames:
            if path.isEmpty(): parentCell = self.conf.cell
            else:              parentCell = path.getTailInstance().getMasterCell()
            instance = parentCell.getInstance( iName )
            if not instance:
                raise ErrorMessage( 1, [ 'Block.useBlockInstance(): Cell "{}" has no instance "{}".' \
                                         .format(parentCell.getName(),iName)
                                       , '(in path:"{})"' \
                                         .format(instancePathName) ] )
            path = Path( path, instance )
        
        tailInstance = path.getTailInstance()
        for blockIns in self.blockInstances:
            if blockIns.instance == tailInstance:
                print( ErrorMessage('Block.useBlockInstance(): In Cell "{}", duplicate call for "{}"' \
                                    .format(self.cell.getName(),tailInstance.getName()) ))
                return
        blockIns = BlockInstance( tailInstance, transf )
        self.blockInstances.append( blockIns )
  
    def save ( self ):
        if not self.conf.validated:
            raise ErrorMessage( 1, 'block.save(): Chip is not valid, aborting.' )
        self.spares.save()


# ----------------------------------------------------------------------------
# Class  :  "blockinstance.BlockInstance".

class BlockInstance ( object ):
    """
    Light helper class to hold a sub-block information.
    """

    def __init__ ( self, instance, transf ):
        self.transf   = transf
        self.instance = instance
        self.block    = Block.lookup( instance.getMasterCell() )
        if not self.block:
            raise ErrorMessage( 1, [ 'BlockInstance.__init__(): Instance "{}" of cell "{}" has no Block defined.' \
                                     .format( instance.getName(), instance.getMasterCell().getName() )
                                   ] )
        return

    def place ( self ):
        self.instance.setTransformation( self.transf )
        self.instance.setPlacementStatus( Instance.PlacementStatus.FIXED )
        return

