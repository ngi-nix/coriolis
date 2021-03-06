.. -*- Mode: rst -*-


7. Working in real mode
=======================

The AllianceFramework_ only manages *symbolic* layout as |Alliance| does.
But |Coriolis| is also able to work directly in *real* mode, meaning
that distances will be expressed in microns instead of lambdas.

The *real* mode will be illustrated by working with the FreePDK45_.

We will assume that the FreePDK45_ archives is installed under: ::

  /home/dks/


7.1 Loading a |LEF| file
~~~~~~~~~~~~~~~~~~~~~~~~

Importing a |LEF| file is simple, you just call the static function
``LefImport.load()``. Multiple |LEF| file can be imported one after
another.

.. code-block:: Python

   # You must set "DKsdir" to where you did install the NCSU FreePDK 45nm DK.
   DKsdir  = '/home/dks'

   library = LefImport.load( DKsdir + '/FreePDK45/osu_soc/lib/files/gscl45nm.lef' )		


.. note:: **Technology checking.**  The first imported |LEF| file must contain the
	  technology. The technology described in the |LEF| file will be checked
	  against the one configured in the running instance of |Coriolis| to look
	  for any discrepencies.


7.2 Loading a |Blif| file -- |Yosys|
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The |Blif| format is generated by the Yosys_ logic synthetizer. Here again, it is
pretty straightforward: call the static function ``Blif.load()``. If you made
your synthesis on a cell library not managed by AllianceFramework_, for example
the one of the FreePDK45, you must load it prior to calling the |Blif| loader.

.. code-block:: Python

   cell = Blif.load( 'snx' ) # load "snx.blif" in the working directory.
