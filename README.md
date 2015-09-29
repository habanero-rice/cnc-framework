CnC Framework
=============

CnC Framework, version 1.1.x-dev

Released on YYYY-MM-DD

Some basic setup and usage instructions are included in this readme.
More detailed instructions, tutorials, and API documentation are
available online through our GitHub wiki:

https://github.com/habanero-rice/cnc-framework/wiki


Installation prerequisites
--------------------------

You will need the following utilities installed,
most of which should already be present on a standard Linux box:

* GNU make
* gcc or clang
* curl or wget
* python 2.6 or 2.7

You will also need one or more of the supported runtime backends:

* OCR 1.0
* Intel CnC


Setting up the environment for OCR
----------------------------------

If you have a copy of the xstack repository, the CnC root directory (which
contains this README) should be in the xstack/hll directory. The expected
directory structure is as follows:

   - `xstack/` : the root xstack source directory
   - `xstack/ocr/` : the source root for OCR
   - `xstack/hll/` : high-level-libraries for OCR
   - `xstack/hll/cnc/` : CnC root directory

If you don't have a copy of the xstack repository, you can still mimic this
directory structure to get the expected behavior. To set the needed environment
variables, navigate to the CnC root directory and source `setup_env.sh`:

    source setup_env.sh

This script sets the `XSTACK_ROOT` variable to point to the xstack repository
root, `UCNC_ROOT` to point to the CnC root, and updates the `PATH` to include
the CnC `bin` directory (which contains the graph translator tool).

OCR is the current default backend runtime for the CnC framework.


Setting up the environment for iCnC
-----------------------------------

Intel CnC (iCnC) comes with its own environment setup script. Locate your iCnC
installation directory, and source the `bin/cncvars.sh` script:

    source /path/to/icnc/bin/cncvars.sh

Detailed installation instructions for iCnC can be found on its GitHub pages:

https://icnc.github.io/

You will also need to set up the environment variables for the CnC framework.
Navigate to the CnC root directory and source `setup_env.sh`:

    source setup_env.sh

This script sets the `UCNC_ROOT` to point to the CnC root, and updates the `PATH`
to include the CnC `bin` directory (which contains the graph translator tool).

To use the iCnC runtime with the CnC framework, you must explicitly specify a
platform when invoking the CnC translator tool:

    ucnc_t --platform=icnc


Verifying the installation
--------------------------

You can verify the installation by running the `$UCNC_ROOT/test/run_tests.sh`
script.  The script builds, runs, and verifies the results of a few simple CnC
test programs. More interesting example applications can be found in the
`$UCNC_ROOT/examples` directory. Each example has an accompanying README with
instructions on how to build and run the application, a general explanation
of the program structure, and a sample of the expected output.


Creating CnC applications
-------------------------

1. For any CnC application, the first step is to create the CnC graph file
   (`GraphName.cnc`) which will define the steps and item collections, as
   well as the relations among them.

2. The second step is invoking the translator: `ucnc_t GraphName.cnc`

   This will generate a bunch of files. The files generated in the current
   directory should be edited by the application author. The files in the
   `cnc_support` directory interface the user code with the underlying OCR
   runtime and should not need any editing.

   Note that the first time you run the graph translator, it will download
   and install several Python dependencies, which will be saved in
   `${UCNC_ROOT}/py` for future reuse.

3. The third step is editing the code in the project directory to implement the
   graph's init, finalize, and step functions. Additionally, you may need to
   edit `cncMain` (in `Main.c`) to parse command-line arguments and pass them
   to the CnC graph when launched.

4. `make run WORKLOAD_ARGS="arg1 arg2 arg3 ..."`

See the examples (in the `$UCNC_ROOT/examples` directory) for sample code. For
more details on the CnC toolchain, API, workflow, etc., please refer to the
online documentation.


Further documentation and support
---------------------------------

For more in-depth documentation, please visit the CnC wiki on GitHub:

https://github.com/habanero-rice/cnc-framework/wiki

You can also report bugs and other issues on our GitHub page:

https://github.com/habanero-rice/cnc-framework


Acknowledgments
---------------

Partial support for CnC was provided through the CDSC program of
the National Science Foundation with an award in the 2009 Expedition
in Computing Program.

This work is actively supported as part of the DOE-funded Traleika Glacier
X-Stack project (Intel CS1924113).
