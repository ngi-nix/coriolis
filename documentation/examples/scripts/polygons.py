#!/usr/bin/python

import sys
from   Hurricane import *
from   CRL       import *


def toDbU ( l ): return DbU.fromLambda(l)


def doBreak ( level, message ):
    UpdateSession.close()
    Breakpoint.stop( level, message )
    UpdateSession.open()


def buildPolygons ( editor ):
   #DbU.setPolygonStep( toDbU(0.1) )
    
    UpdateSession.open()

    cell = AllianceFramework.get().createCell( 'polygons' )
    cell.setTerminal( True )

    cell.setAbutmentBox( Box( toDbU(-5.0), toDbU(-5.0), toDbU(65.0), toDbU(75.0) ) )

    if editor:
      UpdateSession.close()
      editor.setCell( cell )
      editor.fit()
      UpdateSession.open()
    
    technology = DataBase.getDB().getTechnology()
    metal1     = technology.getLayer( "METAL1"     ) 
    metal2     = technology.getLayer( "METAL2"     ) 
    metal4     = technology.getLayer( "METAL4"     ) 
    poly       = technology.getLayer( "POLY"       )
    ptrans     = technology.getLayer( "PTRANS"     )
    ntrans     = technology.getLayer( "NTRANS"     )
    pdif       = technology.getLayer( "PDIF"       )
    ndif       = technology.getLayer( "NDIF"       )
    contdifn   = technology.getLayer( "CONT_DIF_N" )
    contdifp   = technology.getLayer( "CONT_DIF_P" )
    nwell      = technology.getLayer( "NWELL"      )
    contpoly   = technology.getLayer( "CONT_POLY"  )
    ntie       = technology.getLayer( "NTIE"       )


    net = Net.create( cell, 'my_net' )
    net.setExternal( True )
   #points = [ Point( toDbU(  0.0), toDbU(  0.0) )
   #         , Point( toDbU( 10.0), toDbU(  0.0) )
   #         , Point( toDbU(  7.0), toDbU(  8.0) )
   #         , Point( toDbU(  3.0), toDbU(  8.0) ) ]
   #p = Polygon.create( net, metal4, points )

   # Counter-clockwise, slope > 1.
    points = [ Point( toDbU(  3.0), toDbU(  0.0) )
             , Point( toDbU( 13.0), toDbU(  0.0) )
             , Point( toDbU( 16.0), toDbU( 10.0) )
             , Point( toDbU( 16.0), toDbU( 20.0) )
             , Point( toDbU( 13.0), toDbU( 30.0) )
             , Point( toDbU(  3.0), toDbU( 30.0) )
             , Point( toDbU(  0.0), toDbU( 20.0) )
             , Point( toDbU(  0.0), toDbU( 10.0) ) ]
    p = Polygon.create( net, metal2, points )
   #p.translate( toDbU(40.0), toDbU(0.0) )

   # clockwise, slope > 1.
    points = [ Point( toDbU(  0.0), toDbU( 10.0) )
             , Point( toDbU(  0.0), toDbU( 20.0) )
             , Point( toDbU(  3.0), toDbU( 30.0) )
             , Point( toDbU( 13.0), toDbU( 30.0) )
             , Point( toDbU( 16.0), toDbU( 20.0) )
             , Point( toDbU( 16.0), toDbU( 10.0) )
             , Point( toDbU( 13.0), toDbU(  0.0) )
             , Point( toDbU(  3.0), toDbU(  0.0) ) ]
    p = Polygon.create( net, metal2, points )
    p.translate( toDbU(0.0), toDbU(40.0) )

   # Counter-clockwise, slope < 1.
    points = [ Point( toDbU( 10.0), toDbU(  0.0) )
             , Point( toDbU( 20.0), toDbU(  0.0) )
             , Point( toDbU( 30.0), toDbU(  3.0) )
             , Point( toDbU( 30.0), toDbU( 13.0) )
             , Point( toDbU( 20.0), toDbU( 16.0) )
             , Point( toDbU( 10.0), toDbU( 16.0) )
             , Point( toDbU(  0.0), toDbU( 13.0) )
             , Point( toDbU(  0.0), toDbU(  3.0) ) ]
    p = Polygon.create( net, metal2, points )
    p.translate( toDbU(30.0), toDbU(0.0) )

   # clockwise.
    points = [ Point( toDbU(  0.0), toDbU(  3.0) ) 
             , Point( toDbU(  0.0), toDbU( 13.0) )
             , Point( toDbU( 10.0), toDbU( 16.0) )
             , Point( toDbU( 20.0), toDbU( 16.0) )
             , Point( toDbU( 30.0), toDbU( 13.0) )
             , Point( toDbU( 30.0), toDbU(  3.0) )
             , Point( toDbU( 20.0), toDbU(  0.0) )
             , Point( toDbU( 10.0), toDbU(  0.0) ) ]
    p = Polygon.create( net, metal2, points )
    p.translate( toDbU(30.0), toDbU(40.0) )

    UpdateSession.close()

   #AllianceFramework.get().saveCell( cell, Catalog.State.Views )
   # No saving as we don't have a GDSII driver (yet).
    return


def ScriptMain ( **kw ):
    editor = None
    if kw.has_key('editor') and kw['editor']:
      editor = kw['editor']

    buildPolygons( editor )
    return True 
