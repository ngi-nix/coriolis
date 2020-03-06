#!/usr/bin/env python

import re
import os
import os.path
import sys
import time
import shutil
import socket
import subprocess
import logging
import optparse


def log ( message ):
    print message
    logging.info( message )
    return


class ErrorMessage ( Exception ):

    def __init__ ( self, code, *arguments ):
        self._code   = code
        self._errors = [ 'Malformed call to ErrorMessage()'
                       , '%s' % str(arguments) ]

        text = None
        if len(arguments) == 1:
            if isinstance(arguments[0],Exception): text = str(arguments[0]).split('\n')
            else:
                self._errors = arguments[0]
        elif len(arguments) > 1:
            text = list(arguments)

        if text:
            self._errors = []
            while len(text[0]) == 0: del text[0]

            lstrip = 0
            if text[0].startswith('[ERROR]'): lstrip = 8 

            for line in text:
                if line[0:lstrip  ] == ' '*lstrip or \
                   line[0:lstrip-1] == '[ERROR]':
                    self._errors += [ line[lstrip:] ]
                else:
                    self._errors += [ line.lstrip() ]
        return

    def __str__ ( self ):
        if not isinstance(self._errors,list):
            return "[ERROR] %s" % self._errors

        formatted = "\n"
        for i in range(len(self._errors)):
            if i == 0: formatted += "[ERROR] %s" % self._errors[i]
            else:      formatted += "        %s" % self._errors[i]
            if i+1 < len(self._errors): formatted += "\n"
        return formatted

    def addMessage ( self, message ):
        if not isinstance(self._errors,list):
            self._errors = [ self._errors ]
        if isinstance(message,list):
            for line in message:
                self._errors += [ line ]
        else:
            self._errors += [ message ]
        return

    def terminate ( self ):
        print self
        sys.exit(self._code)

    def _getCode ( self ): return self._code

    code = property(_getCode)


class Configuration ( object ):

    def __init__ ( self ):
        self.onSource = False
        self.onLepka  = False
        self.onDocker = False
        hostname = socket.gethostname()
        if hostname.startswith('lepka'): self.onLepka  = True
        else:                            self.onDocker = True

        scriptDir = os.path.abspath(os.getcwd())
        if scriptDir.endswith( 'coriolis/documentation' ):
          self.onLepka  = False
          self.onSource = True

        if self.onDocker:
          log( 'Using *Docker* configuration.' )
          self.pelicanDir = '/data/git/coriolis.lip6.fr/pelican'
          self.apacheDir  = '/var/www/html'
        if self.onLepka:
          log( 'Using *Lepka* configuration.' )
          self.pelicanDir = os.path.join( os.environ['HOME'], 'cms/coriolis.lip6.fr/pelican' )
          self.apacheDir  = '/dsk/l1/httpd/coriolis'
        if self.onSource:
          log( 'Using *Source* configuration.' )
          self.pelicanDir = scriptDir
          self.apacheDir  = None

        self.localDocDir      = '/dsk/l1/jpc/coriolis-2.x/Linux.el7_64/Release.Shared/install/share/doc/coriolis2/en/html/doc'
        self.remoteDocDir     = '/data'
        self.remoteGitDir     = '/data/git'
        self.remotePelicanDir = os.path.join( self.remoteGitDir, 'coriolis.lip6.fr/pelican' )
        self.pluginsDir       = os.path.join( self.remoteGitDir, 'pelican-plugins' )

        self.logDir = os.path.join( self.pelicanDir, 'logs' )

        self.target = 'coriolis-d'
        return


class Command ( object ):

    def __init__ ( self, command ):
        self.command = command
        self.outlog  = None
        self.errlog  = None
        return

    def asString ( self ):
        s = ''
        for i in range(len(self.command)):
          if i: s += ' '
          if ' ' in self.command[i]: s += '"'+self.command[i]+'"'
          else:                      s +=     self.command[i]
        return s

    def execute ( self ):
        sys.stdout.flush()
        sys.stderr.flush()
        log( self.asString() )
        child = subprocess.Popen( self.command, stdin =subprocess.PIPE
                                              , stdout=subprocess.PIPE
                                              , stderr=subprocess.PIPE )
        self.outlog, self.errlog = child.communicate()
        if len(self.outlog): logging.info ( self.outlog )
        if len(self.errlog): logging.error( self.errlog )

        status   = child.returncode
        status >>= 8
        if status != 0:
            ErrorMessage( status, "%s (status:%d)."%(error,status) ).terminate()
        return status

    @staticmethod
    def rmtree ( path ):
        command = 'rm -rf %s' % path
        try:
          log( command )
          shutil.rmtree( path )
        except shutil.Error, e:
          logging.error( str(e) )
          return 1
        return 0

    @staticmethod
    def copytree ( src, dst ):
        command = 'cp -r %s %s' % (src,dst)
        try:
          log( command )
          shutil.copytree( src, dst )
        except OSError, e:
          logging.error( e )
          return 1
        except shutil.Error, errors:
          for error in errors:
            logging.error( 'cp %s %s: %s' % error )
          return 1
        return 0


class SshCommand ( object ):

    def __init__ ( self, scriptlet ):
        self.scriptlet = scriptlet
        return

    def execute ( self, target ):
        command = [ 'ssh', '-x', target, self.scriptlet ]
        Command( command ).execute()
        return


class Document ( object ):

    def __init__ ( self, conf, document ):
        self.conf     = conf
        self.document = os.path.basename( document )
        self.rdir     = os.path.dirname ( document )
        return

    def toPdf ( self ):
        pdfDir         = '%s/content/pdfs'     % self.conf.pelicanDir
        stylesheet     = '%s/etc/SoC-ReST.tex' % self.conf.pelicanDir
        documentPdf    = '%s.pdf' % self.document
        documentRst    = '%s.rst' % self.document
        documentTex    = '%s.tex' % self.document
        documentRawTex = '%s-raw.tex' % self.document
        documentTmps   = [ documentTex
                         , documentRawTex
                         , '%s.log' % self.document
                         , '%s.out' % self.document
                         , '%s.aux' % self.document
                         , '%s.toc' % self.document
                         ]
        targetPdf      = os.path.join( pdfDir, documentPdf )
       
        cwd = os.getcwd()
        os.chdir( os.path.join(self.conf.pelicanDir,self.rdir) )
        os.environ[ 'TEXINPUTS' ] = '%s/etc/images//:./images//:' % self.conf.pelicanDir
        Command( [ 'rst2latex', '--use-latex-toc'
                              , '--stylesheet=%s' % stylesheet
                              , documentRst
                              , documentRawTex
                 ] ).execute()

        pMulticol = re.compile( r' \\& \\\\multicolumn\{2\}\{l\|\}\{' )
        fdi = open( documentRawTex, 'r' )
        fdo = open( documentTex   , 'w' )
        for line in fdi.readlines():
          fdo.write( pMulticol.sub('  \\& \\\\multicolumn{2}{p{0.6\\\\DUtablewidth}|}{',line) )
        fdi.close()
        fdo.close()

        Command( [ 'pdflatex', '-halt-on-error', documentTex ] ).execute()
        Command( [ 'pdflatex', '-halt-on-error', documentTex ] ).execute()

        for file in documentTmps: os.unlink( file )

        if not os.path.isdir( pdfDir ):
          log( 'mkdir %s' % pdfDir )
          os.mkdir( pdfDir )
        
        if os.path.exists( targetPdf ): os.unlink( targetPdf )
        log( 'mv %s %s' % (documentPdf,pdfDir) )
        shutil.move( documentPdf, pdfDir )

        os.chdir( cwd )
        return


class Site ( object ):

    def __init__ ( self, conf ):
        self.conf = conf
        return

    def build ( self ):
        print 'cd %s' % self.conf.pelicanDir
        os.chdir( self.conf.pelicanDir )
        status = Command( [ 'pelican', '-vD', '--ignore-cache', 'content' ] ).execute()
        if status: return status

        if self.conf.onLepka:
          Command.rmtree  ( self.conf.apacheDir )
          Command.copytree( './output', self.conf.apacheDir )
          Command.copytree( self.conf.localDocDir, os.path.join(self.conf.apacheDir,'doc') )
        return

    def gitPush ( self ):
        print 'cd %s' % self.conf.pelicanDir
        os.chdir( self.conf.pelicanDir )

        lines = ''
        cmd   = Command( ['git', 'status', 'content', 'common', 'theme'] )
        cmd.execute()
        if cmd.outlog: lines = cmd.outlog.split('\n')
        if lines[-2] != 'nothing to commit, working directory clean':
          message = [ 'There are some uncommited changes in the site contents.' ] + lines
          print ErrorMessage( 1, message )
         #return False

        Command( ['git', 'push'] ).execute()
        return True

    def gitCommitPdfs ( self ):
        print 'cd %s' % self.conf.pelicanDir
        os.chdir( self.conf.pelicanDir )

        lines = ''
        cmd   = Command( ['git', 'status', 'content/pdfs'] )
        cmd.execute()
        if cmd.outlog: lines = cmd.outlog.split('\n')
        if lines[-2] != 'nothing to commit, working directory clean':
          message = 'Updated PDFs, %s.' % time.strftime( '%B %d, %Y (%H:%M)' )
          Command( ['git', 'add'   , 'content/pdfs'] ).execute()
          Command( ['git', 'commit', '-m', message ] ).execute()

        return True

    def remoteBuild ( self ):
        Command( [ 'ssh', '-x', '-o', 'StrictHostKeyChecking no'
                 , self.conf.target, "echo 'Force SSH key loading.'" ] ).execute()

        Command( [ 'rsync', '--rsh=/usr/bin/ssh', '-avH'
                 , self.conf.localDocDir
                 , '%s:%s' % (self.conf.target,self.conf.remoteDocDir) ] ).execute()

        remoteScript = \
          ' if [ ! -d %(remotePelicanDir)s ]; then'                                       \
          '   cd %(remoteGitDir)s;'                                                       \
          '   git clone gitsoc@bop-t:coriolis.lip6.fr;'                                   \
          '   sudo pelican-themes -s %(remotePelicanDir)s/themes/nest-coriolis;'          \
          '   sudo chown -R pelican:pelican /var/www/html;'                               \
          ' fi;'                                                                          \
          ' cd %(remotePelicanDir)s;'                                                     \
          ' git pull;'                                                                    \
          ' if [ ! -d %(pluginsDir)s ]; then'                                             \
          '   cd %(remoteGitDir)s;'                                                       \
          '   git clone https://github.com/getpelican/pelican-plugins;'                   \
          '   cd pelican-plugins;'                                                        \
          '   patch -p1 -i %(remotePelicanDir)s/patchs/0001-bootstrap-rst.no-math.patch;' \
          ' fi;'                                                                          \
          ' cd %(remotePelicanDir)s;'                                                     \
          ' ./build.py -p;'                                                               \
          % { 'pluginsDir'       : self.conf.pluginsDir
            , 'remoteGitDir'     : self.conf.remoteGitDir
            , 'remotePelicanDir' : self.conf.remotePelicanDir
            }
        
        SshCommand( remoteScript ).execute( self.conf.target )
        return


if __name__ == '__main__':

   usage = \
     '\n'  \
     '\nThe "build.py" script rebuild the "coriolis.lip6.fr" website hosted'   \
     '\nat the Sorbonne Universite/LIP6. It can work in the following modes:'  \
     '\n'                                                                      \
     '\n1. Fully install the website on a freshly generated docker image.'     \
     '\n2. Update the "coriolis.lip6.fr" website.'                             \
     '\n3. Generate a static website on my laptop ("lepka").'                  \
     '\n4. Generate the static documentation shipped with coriolis (in git).'  \
     '\n'                                                                      \
     '\nNOTE: About the "<-C|--coriolis> remote rebuild option."'              \
     '\n      Your ${HOME}/.ssh/config file must contain an alias for the'     \
     '\n      "coriolis-d" host, as shown below:'                              \
     '\n'                                                                      \
     '\n          Host coriolis-d'                                             \
     '\n              User          pelican'                                   \
     '\n              HostName      localhost'                                 \
     '\n              HostKeyAlias  coriolis-d'                                \
     '\n              Port          2250'                                      \
     '\n              CheckHostIP   no'                                        \
     '\n              ForwardAgent  yes'                                       \
     '\n              ForwardX11    no'                                        \
     '\n              RemoteForward 2227 localhost:2227'                       \
     '\n'                                                                      \
     '\n      It logs as user "pelican" using the ssh port 2250 and creates'   \
     '\n      a tunnel between port 2227 on the remote host (the web server)'  \
     '\n      and port 2227 on the local host (your machine). You *must* have' \
     '\n      an ssh tunnel between your machine and "bop" on the local port'  \
     '\n      2227 for this to work. The "2227" tunnel between the web server' \
     '\n      and "bop" is used to perform "git pull".'                        \
     '\n'                                                                      \
     '\n./build.py <options>'

   parser = optparse.OptionParser( usage )
   parser.add_option( '-p', '--pelican' , action='store_true', dest='doPelican' , help='Run pelican.' )
   parser.add_option( '-P', '--pdfs'    , action='store_true', dest='doPdfs'    , help='Generate the PDFs.' )
   parser.add_option( '-C', '--coriolis', action='store_true', dest='doCoriolis', help='Build/update the web site on the server (docker).' )
   (options, args) = parser.parse_args()

   conf = Configuration()
   if not os.path.isdir( conf.logDir ):
     os.mkdir( conf.logDir )

   logging.basicConfig( filename='logs/build-%s.log' % time.strftime( '%Y%m%d-%H%M' )
                      ,   format='%(asctime)s:%(levelname)s| %(message)s'
                      ,    level=logging.INFO
                      )

   documents = [ Document( conf, 'content/pages/users-guide/UsersGuide' )
               , Document( conf, 'content/pages/python-tutorial/PythonTutorial' )
               , Document( conf, 'content/pages/python-cpp/PythonCpp' )
               , Document( conf, 'content/pages/stratus/Stratus' )
               , Document( conf, 'content/pages/rds/RDS' )
               ]

   coriolis = Site( conf )

   if options.doPdfs:
     for document in documents: document.toPdf()
     if not conf.onSource:
       coriolis.gitCommitPdfs()

   if options.doPelican:
     coriolis.build()

   if options.doCoriolis:
     coriolis.gitPush()
     coriolis.remoteBuild()

   sys.exit( 0 )
