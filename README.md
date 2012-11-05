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
ToDO
