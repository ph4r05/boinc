from distutils.core import setup
import py2exe

setup(
	console=['run_simulation.py'], 
	options = {"py2exe": {
		"dll_excludes": ["w9xpopen.exe"],
        "excludes": ['_ssl', 'pyreadline', 'difflib', 'doctest', 'locale', 'optparse', 'pickle', 'calendar'],
        'compressed': True,
		"bundle_files": 1 # 1=Single .exe, 2=.exe with pythonXX.dll
	}},
    zipfile = None,
)
