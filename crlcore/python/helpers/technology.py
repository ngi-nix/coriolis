# -*- mode:Python -*-
#
# This file is part of the Coriolis Software.
# Copyright (c) SU 2020-2020, All Rights Reserved
#
# +-----------------------------------------------------------------+ 
# |                   C O R I O L I S                               |
# |          Alliance / Hurricane  Interface                        |
# |                                                                 |
# |  Author      :                    Jean-Paul Chaput              |
# |  E-mail      :            Jean-Paul.Chaput@lip6.fr              |
# | =============================================================== |
# |  Python      :   "./crlcore/helpers/technology.py"              |
# +-----------------------------------------------------------------+

"""
Some helpers to create or load a technology and it's libraries.
"""

from __future__ import print_function
from Hurricane  import DataBase, Library, BasicLayer, Layer


__all__ = [ 'safeGetLibrary', 'createBL', 'setEnclosures' ]


def safeGetLibrary ( frameworkName, libName ):
    """
    Return the library named ``libName`` if it already exists,
    or create it if needed. To avoid potential collision, design
    kits libraries should not be created directly under the DataBase
    root library but be grouped under a common technology library,
    in this case ``CM018``.
    """
    db       = DataBase.getDB()
    tech     = db.getTechnology()
    frameLib = db.getRootLibrary().getLibrary( frameworkName )
    if frameLib is None:
        frameLib = Library.create( db.getRootLibrary(), frameworkName )
    lib = frameLib.getLibrary( libName )
    if lib is None:
        lib = Library.create( frameLib, libName )
    return lib


def createBL ( tech, layerName, material, size=None, spacing=None, gds2Layer=None, gds2DataType=0 ):
    """
    Create a new BasicLayer. Parameters ``tech``, ``layerName`` and ``material``
    are mandatory.

    :param tech:         The technology the basic layer will be part of.
    :param layerName:    The name of the layer.
    :param material:     The kind of layer, see BasicLayer.Material.
    :param size:         The minimal size (i.e. width).
    :param spacing:      The minimal distance, edge to edge between two wires.
    :param gds2layer:    The GDSII layer number (for the GDSII driver).
    :param gds2DataType: The GDSII DataType (i.e purpose).
    """
    layer = BasicLayer.create( tech, layerName, BasicLayer.Material(material) )
    if size is not None:
        layer.setMinimalSize( size )
    if spacing is not None:
        layer.setMinimalSpacing( spacing )
    if gds2Layer is not None:
        layer.setGds2Layer   ( gds2Layer )
        layer.setGds2Datatype( gds2DataType )
    return layer


def setEnclosures ( layer, subLayer, enclosures ):
    """
    Set horizontal & vertical enclosure for a given ``subLayer`` in a
    composite ``layer`` (typically a ViaLayer). If ``enclosures`` is a
    number, both H/V will be set to that same value. If it is a tuple
    (a pair), then the first value is horizontal and the seconf is
    vertical.
    """
    if isinstance(enclosures,tuple):
       henclosure = enclosures[0]
       venclosure = enclosures[1]
    else:
       henclosure = enclosures
       venclosure = enclosures
    layer.setEnclosure( subLayer, henclosure, Layer.EnclosureH )
    layer.setEnclosure( subLayer, venclosure, Layer.EnclosureV )
    return
