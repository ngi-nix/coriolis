
// -*- C++ -*-
//
// This file is part of the Coriolis Software.
// Copyright (c) UPMC/LIP6 2008-2010, All Rights Reserved
//
// ===================================================================
//
// $Id$
//
// x-----------------------------------------------------------------x
// |                                                                 |
// |                   C O R I O L I S                               |
// |              M a u k a  -  P l a c e r                          |
// |                                                                 |
// |  Author      :                    Jean-Paul Chaput              |
// |  E-mail      :            Jean-Paul.Chaput@lip6.fr              |
// | =============================================================== |
// |  C++ Header  :       "./GraphicMaukaEngine.cpp"                 |
// | *************************************************************** |
// |  U p d a t e s                                                  |
// |                                                                 |
// x-----------------------------------------------------------------x


#include  <functional>
#include  <boost/bind.hpp>

#include  <QAction>
#include  <QMenu>
#include  <QMenuBar>
#include  <QApplication>
#include  <hurricane/Warning.h>
#include  <hurricane/Error.h>
#include  <hurricane/Breakpoint.h>
#include  <hurricane/DebugSession.h>
#include  <hurricane/Go.h>
#include  <hurricane/Net.h>
#include  <hurricane/Cell.h>
#include  <hurricane/UpdateSession.h>
#include  <hurricane/viewer/Graphics.h>
#include  <hurricane/viewer/CellWidget.h>
#include  <hurricane/viewer/CellViewer.h>
#include  <hurricane/viewer/ControllerWidget.h>
#include  <crlcore/Utilities.h>
#include  <crlcore/AllianceFramework.h>
#include  <nimbus/NimbusEngine.h>
#include  <mauka/Container.h>
#include  <mauka/GraphicMaukaEngine.h>
#include  <mauka/ConfigurationWidget.h>


namespace Mauka {


  using namespace std;
  using Hurricane::Error;
  using Hurricane::Warning;
  using Hurricane::Breakpoint;
  using Hurricane::DebugSession;
  using Hurricane::UpdateSession;
  using Hurricane::Net;
  using Hurricane::Graphics;
  using Hurricane::ColorScale;
  using Hurricane::ControllerWidget;
  using CRL::Catalog;
  using CRL::AllianceFramework;
  using Nimbus::NimbusEngine;


  size_t              GraphicMaukaEngine::_references = 0;
  GraphicMaukaEngine* GraphicMaukaEngine::_singleton  = NULL;


  void  GraphicMaukaEngine::initMaukaContainer ( CellWidget* widget )
  {
    widget->getDrawingPlanes().setPen   ( Graphics::getPen  ("mauka.container",widget->getDarkening()) );
    widget->getDrawingPlanes().setBrush ( Graphics::getBrush("mauka.container",widget->getDarkening()) );
  }


  void  GraphicMaukaEngine::drawMaukaContainer ( CellWidget*           widget
                                               , const Go*             go
                                               , const BasicLayer*     basicLayer
                                               , const Box&            box
                                               , const Transformation& transformation
                                               )
  {
    const Container* container = static_cast<const Container*>(go);

    QPainter& painter = widget->getPainter();
    painter.drawRect ( widget->dbuToDisplayRect(container->getBoundingBox()) );
  }


  MaukaEngine* GraphicMaukaEngine::createEngine ()
  {
    Cell* cell = getCell ();

    MaukaEngine* mauka = MaukaEngine::get ( cell );
    if ( mauka == NULL ) {
      NimbusEngine* nimbus = NimbusEngine::create ( cell, AllianceFramework::get()->getLibrary(1) );
      mauka = MaukaEngine::create ( cell );
    } else
      cerr << Warning("%s already has a Mauka engine.",getString(cell).c_str()) << endl;

    return mauka;
  }


  MaukaEngine* GraphicMaukaEngine::getForFramework ()
  {
    MaukaEngine* mauka = MaukaEngine::get ( getCell() );
    if ( mauka != NULL ) return mauka;

    mauka = createEngine ();
    
    if ( mauka == NULL ) 
      throw Error("Failed to create Mauka engine on %s.",getString(getCell()).c_str());

    return mauka;
  }


  void  GraphicMaukaEngine::run ()
  {
    MaukaEngine* mauka = createEngine ();
    if ( mauka == NULL ) {
      throw Error("MaukaEngine not created yet, run the global router first.");
    } 
    emit cellPreModificated ();

    _viewer->clearToolInterrupt ();
    _viewer->getCellWidget()->fitToContents ();

    mauka->Run ();
    emit cellPostModificated ();
  }


  void  GraphicMaukaEngine::save ()
  {
    MaukaEngine* mauka = createEngine ();
    if ( mauka == NULL ) {
      throw Error("MaukaEngine not created yet, run the placer first.");
    } 
    emit cellPreModificated ();

    _viewer->clearToolInterrupt ();

    mauka->Save ();
    emit cellPostModificated ();
  }


  void  GraphicMaukaEngine::postEvent ()
  {
    static unsigned int count = 0;

    if ( not (count++ % 500) ) {
    //UpdateSession::close ();

    //_viewer->getCellWidget()->refresh ();
      QApplication::processEvents ();

    //UpdateSession::open ();

      if ( _viewer->isToolInterrupted() ) {
      //MaukaEngine* mauka = MaukaEngine::get ( getCell() );
      //if ( mauka ) mauka->setInterrupt ( true );
        
        _viewer->clearToolInterrupt ();
      }
    }
  }


  void  GraphicMaukaEngine::addToMenu ( CellViewer* viewer )
  {
    assert ( _viewer == NULL );

    _viewer = viewer;

    QMenu* prMenu   = _viewer->findChild<QMenu*>("viewer.menuBar.placeAndRoute");
    QMenu* stepMenu = _viewer->findChild<QMenu*>("viewer.menuBar.placeAndRoute.stepByStep");
    if ( prMenu == NULL ) {
      QMenuBar* menuBar = _viewer->findChild<QMenuBar*>("viewer.menuBar");
      if ( menuBar == NULL ) {
        cerr << Warning("GraphicMaukaEngine::addToMenu() - No MenuBar in parent widget.") << endl;
        return;
      }
      prMenu = menuBar->addMenu ( tr("P&&R") );
      prMenu->setObjectName ( "viewer.menuBar.placeAndRoute" );

      stepMenu = prMenu->addMenu ( tr("&Step by Step") );
      stepMenu->setObjectName ( "viewer.menuBar.placeAndRoute.stepByStep" );

      prMenu->addSeparator ();
    }

    QAction* placeAction = _viewer->findChild<QAction*>("viewer.menuBar.placeAndRoute.maukaPlace");
    if ( placeAction != NULL )
      cerr << Warning("GraphicMaukaEngine::addToMenu() - Mauka placer already hooked in.") << endl;
    else {
      QAction* placeAction = new QAction  ( tr("Mauka - &Place"), _viewer );
      placeAction->setObjectName ( "viewer.menuBar.placeAndRoute.maukaPlace" );
      placeAction->setStatusTip  ( tr("Run the <b>Mauka</b> placer") );
      placeAction->setVisible    ( true );
      prMenu->addAction ( placeAction );

      connect ( placeAction, SIGNAL(triggered()), this, SLOT(run()) );
    }

    connect ( this, SIGNAL(cellPreModificated ()), _viewer->getCellWidget(), SLOT(cellPreModificate ()) );
    connect ( this, SIGNAL(cellPostModificated()), _viewer->getCellWidget(), SLOT(cellPostModificate()) );

    ControllerWidget*    controller = _viewer->getControllerWidget();
    ConfigurationWidget* setting     = controller->getSettings()
      ->findChild<ConfigurationWidget*>("controller.tabSettings.setting.mauka");

    if ( setting == NULL ) {
      setting = new ConfigurationWidget ();
      setting->setObjectName    ( "controller.tabSettings.setting.mauka" );
      setting->setConfiguration ( Nimbus::Configuration::getDefault(), Configuration::getDefault() );
      controller->addSetting ( setting, "Mauka" );
    }
  }


  const Name& GraphicMaukaEngine::getName () const
  {
    return MaukaEngine::staticGetName ();
  }


  Cell* GraphicMaukaEngine::getCell ()
  {
    if ( _viewer == NULL ) {
      throw Error ( "<b>Mauka:</b> GraphicMaukaEngine not bound to any Viewer." );
      return NULL;
    }

    if ( _viewer->getCell() == NULL ) {
      throw Error ( "<b>Mauka:</b> No Cell is loaded into the Viewer." );
      return NULL;
    }

    return _viewer->getCell();
  }


  GraphicMaukaEngine* GraphicMaukaEngine::grab ()
  {
    if ( not _references ) {
      _singleton = new GraphicMaukaEngine ();
    }
    _references++;

    return _singleton;
  }


  size_t  GraphicMaukaEngine::release ()
  {
    _references--;
    if ( not _references ) {
      delete _singleton;
      _singleton = NULL;
    }
    return _references;
  }


  GraphicMaukaEngine::GraphicMaukaEngine ()
    : GraphicTool()
    , _viewer    (NULL)
  {
    addDrawGo ( "Mauka::Container", initMaukaContainer, drawMaukaContainer );
    addDrawGo ( "Mauka::Bin"      , initMaukaContainer, drawMaukaContainer );
  }


  GraphicMaukaEngine::~GraphicMaukaEngine ()
  { }


}  // End of Mauka namespace.