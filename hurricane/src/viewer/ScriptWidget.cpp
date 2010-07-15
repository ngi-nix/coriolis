
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
// |     V L S I   B a c k e n d   D a t a - B a s e                 |
// |                                                                 |
// |  Author      :                    Jean-Paul CHAPUT              |
// |  E-mail      :       Jean-Paul.Chaput@asim.lip6.fr              |
// | =============================================================== |
// |  C++ Module  :       "./ScriptWidget.cpp"                       |
// | *************************************************************** |
// |  U p d a t e s                                                  |
// |                                                                 |
// x-----------------------------------------------------------------x


#include  <iostream>
#include  <memory>
using namespace std;

#include  <boost/filesystem/operations.hpp>
namespace bfs = boost::filesystem;

#include  "hurricane/Warning.h"

#include  <QLabel>
#include  <QLineEdit>
#include  <QPushButton>
#include  <QRadioButton>
#include  <QButtonGroup>
#include  <QCheckBox>
#include  <QHBoxLayout>
#include  <QVBoxLayout>

#include  "hurricane/viewer/Graphics.h"
#include  "hurricane/viewer/Script.h"
#include  "hurricane/viewer/ScriptWidget.h"
#include  "hurricane/viewer/CellViewer.h"


namespace Hurricane {


// -------------------------------------------------------------------
// Class  :  "ScriptWidget".


  ScriptWidget::ScriptWidget ( QWidget* parent )
    : QDialog  (parent)
    , _lineEdit(NULL)
  {
    setWindowTitle ( tr("Run Script") );

    QLabel* label = new QLabel ();
    label->setText ( tr("Python script to execute") );
    label->setFont ( Graphics::getNormalFont(true) );

    _lineEdit = new QLineEdit ();
    _lineEdit->setMinimumWidth ( 400 );

    QPushButton* okButton = new QPushButton ();
    okButton->setText    ( "OK" );
    okButton->setDefault ( true );

    QPushButton* cancelButton = new QPushButton ();
    cancelButton->setText ( "Cancel" );

    QHBoxLayout* hLayout1 = new QHBoxLayout ();
    hLayout1->addStretch ();
    hLayout1->addWidget ( okButton );
    hLayout1->addStretch ();
    hLayout1->addWidget ( cancelButton );
    hLayout1->addStretch ();

  //QFrame* separator = new QFrame ();
  //separator->setFrameShape  ( QFrame::HLine );
  //separator->setFrameShadow ( QFrame::Sunken );

    QVBoxLayout* vLayout = new QVBoxLayout ();
    vLayout->setSizeConstraint  ( QLayout::SetFixedSize );
    vLayout->addWidget ( label );
    vLayout->addWidget ( _lineEdit );
    vLayout->addLayout ( hLayout1 );
  //vLayout->addWidget ( separator );

    setLayout ( vLayout );
  //setModal  ( true );

    connect (     okButton, SIGNAL(clicked())         , this, SLOT(accept()) );
    connect ( cancelButton, SIGNAL(clicked())         , this, SLOT(reject()) );
  }


  const QString  ScriptWidget::getScriptName () const
  {
    return _lineEdit->text();
  }


  bool  ScriptWidget::runScript ( QWidget* parent, Cell* cell )
  {
    ScriptWidget* dialog      = new ScriptWidget ( parent );
    bool          doRunScript = (dialog->exec() == Accepted);
    QString       scriptName  = dialog->getScriptName ();

    delete dialog;
    if ( not doRunScript ) return false;

    if ( scriptName.endsWith(".py",Qt::CaseInsensitive) )
      scriptName.truncate ( scriptName.size()-3 );

    bfs::path userScript    = scriptName.toStdString();
    bfs::path userDirectory = userScript.branch_path();

    if ( not userDirectory.is_complete() )
      userDirectory = bfs::current_path() / userDirectory;
    userDirectory.normalize();

    Isobar::Script::addPath ( userDirectory.string() );

    dbo_ptr<Isobar::Script> script = Isobar::Script::create(userScript.leaf());
    script->setEditor ( qobject_cast<CellViewer*>(parent) );

    bool returnCode = script->runFunction ( "__hurricane_main__", cell );

    Isobar::Script::removePath ( userDirectory.string() );

    return returnCode;
  }


} // End of Hurricane namespace.