Boinc projects
=====

This repo contains BOINC related project.

Wrappers
=====
In my opinion wrapping is much simpler and universal way how to run application on BOINC with comparison to boincification with programming API.

Since standard wrapper is not flexible enough, it is preferable to use another options.

GenWrapper
=====
This is GenWrapper project clonned from http://genwrapper.sourceforge.net/
It is modified version of this projects, includes some fixes for problems that I experienced with compilation on linux & windows.

It works to compile on linux and windows, with small fixes modifications. 

For detailed documentation about GenWrapper, please reffer to provided web page, or:
http://indico.cern.ch/getFile.py/access?contribId=25&resId=1&materialId=slides&confId=56447

This wrapper can extract provided ZIP file (can contain application+libraries+data files) when is launched by boinc client, then it launches shell file (provided with work-unit) what makes it very flexible.
Shell file is executed with help of gitbox (modified version of busybox).

PyWrapper
=====
If GenWrapper is still not flexible enough to suit your needs, you can use your own wrapper written in python. 
BOINC API is reachable via PyBoinc compiled library (dist-package). With this you can simply call BOINC API functions just by importing boinc package in your python script.

To make your python script executable, you need to compile it to executable format, including all necessary libraries.
This can be done with:
* py2exe (http://www.py2exe.org/) for Windows. It builds all necessary files into one executable on windows, without need to have python installed on target machine
* bbfreeze (http://pypi.python.org/pypi/bbfreeze) or PyInstaller(http://www.pyinstaller.org/) for linux.

I have not figured out how to compile all necessary libraries to one file with bbfreeze. It can be problem when boinc will rename these files.

John the Ripper
=====
As simple application for BOINC, JtR was choosen (http://www.openwall.com/john/).
I am using  Jumbo community patch enabling to crack vast set of hashes/crypto algorithms.
My version is based on GIJohn (http://gijohn.info/) what is grid-enabled patch for john.

Basic principle: brute-force, incremental mode.
Work generator splits key-space between working units, each working unit includes XML file specifying start and stop word, john then tryies all words between these two words, with using defined charset.

It works with MPI, thus it can be effectively run in multiple sub-processes (not by default). Tested, working version not included here (ToDo).
It also SHOULD work with CUDA/OpenCL, but it was not tested.

