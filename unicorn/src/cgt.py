#!/usr/bin/env python

try:
  import sys
  import os.path
  import optparse
  import Cfg
  import Hurricane
  import Viewer
  import CRL
  import Nimbus
  import Metis
  import Mauka
  import Katabatic
  import Kite
  import Unicorn
except ImportError, e:
  module = str(e).split()[-1]

  print '[ERROR] The <%s> python module or symbol cannot be loaded.' % module
  print '        Please check the integrity of the <coriolis> package.'
  sys.exit(1)
except Exception, e:
  print '[ERROR] A strange exception occurred while loading the basic Coriolis/Python'
  print '        modules. Something may be wrong at Python/C API level.\n'
  print '        %s' % e
  sys.exit(2)


def setCgtBanner ( banner ):
    banner.setName('cgt')
    banner.setPurpose('Coriolis Graphical Tool')
    return banner


def credits ():
    s  = ''
    s +=  '        Tool Credits\n'
    s +=  '        Hurricane .................... Remy Escassut & Christian Masson\n'
    s +=  '        Nimbus - Infrastructure .......................... Hugo Clement\n'
    s +=  '        Mauka - Placer ........................... Christophe Alexandre\n'
    s +=  '        Knik - Global Router ............................ Damien Dupuis\n'
    s +=  '        Kite - Detailed Router ....................... Jean-Paul Chaput\n\n'

    s +=  '        hMETIS software credits (used by Mauka)\n'
    s +=  '        Author ........................................ Georges Karypis\n'
    s +=  '        Prof. Ident. .......................... University of Minnesota\n'
    s +=  '        URL .......................... http://glaros.dtc.umn.edu/gkhome\n\n'

    s +=  '        FLUTE software credits (used by Knik)\n'
    s +=  '        Author ........................................ Chris C. N. CHU\n'
    s +=  '        Prof. Ident. ............................ Iowa State University\n'
    s +=  '        URL ........................ http://home.eng.iastate.edu/~cnchu\n\n'
    return s


def runScript ( scriptPath, editor ):
    try:
        script = __import__(scriptPath)
    except Exception, e:
      print '[ERROR] An exception occured while loading the Stratus script module:'
      print '        <%s>\n' % (scriptPath)
      print '        You should check for simple python errors in this module.'
      print '        Error was:'
      print '          %s\n' % e
      print '        Trying to continue anyway...'
      return

    if not hasattr(script,'ScriptMain'):
        print '[ERROR] Script module is missing function ScriptMain().'
        print '        <%s>' % scriptPath
        return

    script.ScriptMain(editor)

    return


if __name__ == '__main__':

    try:
      usage  = str(setCgtBanner(CRL.Banner()))
      usage += '\ncgt [options]'

      parser = optparse.OptionParser(usage)
      parser.add_option( '-c', '--cell'          , type='string'      , dest='cell'          , help='The name of the cell to load, whithout extension.')
      parser.add_option(       '--acm-sigda-89'  , type='string'      , dest='acmSigdaName'  , help='An ACM/SIGDA 89 bench name to load, whithout extension.')
      parser.add_option(       '--script'        , type='string'      , dest='script'        , help='Run a Python or Stratus script.')
      parser.add_option( '-v', '--verbose'       , action='store_true', dest='verbose'       , help='First level of verbosity.')
      parser.add_option( '-V', '--very-verbose'  , action='store_true', dest='veryVerbose'   , help='Second level of verbosity.')
      parser.add_option( '-i', '--info'          , action='store_true', dest='info'          , help='Display lots of informational messages.')
      parser.add_option( '-b', '--bug'           , action='store_true', dest='bug'           , help='Display bug related messages.')
      parser.add_option(       '--show-conf'     , action='store_true', dest='showConf'      , help='Display Kite configuration.')
      parser.add_option( '-D', '--core-dump'     , action='store_true', dest='coreDump'      , help='Enable core-dump when a crash occurs.')
      parser.add_option( '-L', '--log-mode'      , action='store_true', dest='logMode'       , help='Disable ANSI escape sequences in console output.')
      parser.add_option( '-t', '--text'          , action='store_true', dest='textMode'      , help='Run in command line mode.')
      parser.add_option( '-m', '--margin'        , type='float'       , dest='margin'        , help='Percentage of free area to add to the minimal placement area.')
      parser.add_option( '-Q', '--quadri-place'  , action='store_true', dest='quadPlace'     , help='Performs a quadri-partitionnement as first placement stage.')
      parser.add_option( '-P', '--annealing'     , action='store_true', dest='annealingPlace', help='Place using simulated annealing.')
      parser.add_option(       '--min-psize'     , type='int'         , dest='minPSize'      , help='Sets the size of a leaf partition (quadripartition stage).')
      parser.add_option( '-G', '--global-route'  , action='store_true', dest='globalRoute'   , help='Run the global router (Knik).')
      parser.add_option( '-g', '--load-global'   , action='store_true', dest='loadGlobal'    , help='Reload a global routing from disk.')
      parser.add_option(       '--save-global'   , action='store_true', dest='saveGlobal'    , help='Save the global routing solution.')
      parser.add_option( '-e', '--edge'          , type='float'       , dest='edgeCapacity'  , help='The egde density ratio applied on global router\'s edges.')
      parser.add_option(       '--events-limit'  , type='int'         , dest='eventsLimit'   , help='The maximum number of iterations (events) that the router is allowed to perform.')
      parser.add_option( '-R', '--detail-route'  , action='store_true', dest='detailRoute'   , help='Run the detailed router (Kite).')
      parser.add_option( '-M', '--dump-measures' , action='store_true', dest='dumpMeasures'  , help='Dump some statistical measurements on the disk.')
      parser.add_option( '-s', '--save-design'   , type='string'      , dest='saveDesign'    , help='Save the routed design.')
      (options, args) = parser.parse_args()
      args.insert(0, 'cgt')


     #Hurricane.trace(True)
      Cfg.Configuration.pushDefaultPriority(Cfg.Parameter.Priority.CommandLine)

      if options.coreDump:     Cfg.getParamBool      ('misc.catchCore'    ).setBool(False)
      if options.verbose:      Cfg.getParamBool      ('misc.verboseLevel1').setBool(True)
      if options.veryVerbose:  Cfg.getParamBool      ('misc.verboseLevel2').setBool(True)
      if options.info:         Cfg.getParamBool      ('misc.info'         ).setBool(True)
      if options.bug:          Cfg.getParamBool      ('misc.bug'          ).setBool(True)
      if options.logMode:      Cfg.getParamBool      ('misc.logMode'      ).setBool(True)
      if options.showConf:     Cfg.getParamBool      ('misc.showConf'     ).setBool(True)
      if options.margin:       Cfg.getParamPercentage('nimbus.spaceMargin').setPercentage(options.margin)
      if options.minPSize:     Cfg.getParamInt       ('metis.numberOfInstancesStopCriterion').setInt(options.minPSize)
      if options.edgeCapacity: Cfg.getParamPercentage('kite.edgeCapacity' ).setPercentage(options.edgeCapacity)
      if options.eventsLimit:  Cfg.getParamInt       ('kite.eventsLimit'  ).setInt(options.eventsLimit)

      quadPlace      = options.quadPlace
      annealingPlace = options.annealingPlace
      loadGlobal     = options.loadGlobal
      saveGlobal     = options.saveGlobal
      globalRoute    = options.globalRoute
      detailRoute    = options.detailRoute

      Cfg.Configuration.popDefaultPriority()

      af = CRL.AllianceFramework.get()

      cell = None
      if options.acmSigdaName:
          cell = CRL.AcmSigda.load(options.acmSigdaName)
      elif options.cell:
          cell = af.getCell(options.cell, CRL.Catalog.State.Views)
      else:
          quadPlace      = False
          annealingPlace = False
          loadGlobal     = False
          saveGlobal     = False
          globalRoute    = False
          detailRoute    = False
  
      if not options.textMode:
         # Run in graphic mode.
          ha = Viewer.HApplication.create(args)
          Viewer.Graphics.enable()
      
          unicorn = Unicorn.UnicornGui.create()
          unicorn.setApplicationName  ('cgt')
          unicorn.registerTool        (Mauka.GraphicMaukaEngine.grab())
          unicorn.registerTool        (Kite.GraphicKiteEngine.grab())
         #unicorn.setAnonNetSelectable(False)
          unicorn.setLayerVisible     ("grid"          , False);
          unicorn.setLayerVisible     ("text.instance" , False);
          unicorn.setLayerVisible     ("text.component", False);

          if options.script:
              runScript(options.script,unicorn)
    
          setCgtBanner(unicorn.getBanner())
          print unicorn.getBanner()
          print credits()
    
          if cell: unicorn.setCell(cell)
          unicorn.show()
          ha.qtExec()
      else:
         # Run in command line mode.
          kiteSuccess = False

          if quadPlace or annealingPlace:
              loadGlobal  = False
              globalRoute = True
          if quadPlace and annealingPlace:
              annealingPlace = False

          runMaukaTool = quadPlace or annealingPlace

          if runMaukaTool:
              nimbus = Nimbus.NimbusEngine.create(cell)
              if options.showConf: nimbus.printConfiguration()
      
              if annealingPlace:
                Cfg.getParamPercentage('mauka.standardAnnealing').setBool(True);
      
              if quadPlace:
                metis = Metis.MetisEngine.create(cell)
                if options.showConf: metis.printConfiguration()
      
                Metis.MetisEngine.doQuadriPart(cell)
                Mauka.MaukaEngine.regroupOverloadedGCells(cell)
      
              mauka = Mauka.MaukaEngine.create(cell)
              if options.showConf: mauka.printConfiguration()
      
              mauka.run()

          if detailRoute and not (loadGlobal or globalRoute): globalRoute = True
          runKiteTool = loadGlobal or globalRoute or detailRoute

          if runKiteTool:
              if loadGlobal: globalFlags = Kite.LoadGlobalSolution
              else:          globalFlags = Kite.BuildGlobalSolution

              routingNets = []
              kite = Kite.KiteEngine.create(cell)
              if options.showConf: kite.printConfiguration()
    
              kite.runGlobalRouter(globalFlags)
              if saveGlobal: kite.saveGlobalSolution()
    
              if detailRoute:
                  kite.loadGlobalRouting( Katabatic.LoadGrByNet, routingNets )
                  kite.layerAssign      ( Katabatic.NoNetLayerAssign )
                  kite.runNegociate     ()
                  kiteSuccess = kite.getToolSuccess()
                  kite.finalizeLayout()
                  if options.dumpMeasures:
                      kite.dumpMeasures()
                  kite.destroy()
    
          if options.saveDesign:
              views = CRL.Catalog.State.Physical
              if options.saveDesign != cell.getName():
                  cell.setName(options.saveDesign)
                  views |= CRL.Catalog.State.Logical
              af.saveCell(cell, views)

          sys.exit(kiteSuccess)

    except Exception, e:
      print e

    sys.exit(0)