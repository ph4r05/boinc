from distutils.core import setup, Extension

# May need to adjust these to suit your system, but a standard installation of the BOINC client from source will put things in these places
pyboinc = Extension('boinc', runtime_library_dirs=['/usr/lib64'],
	extra_objects=['/usr/lib64/libboinc.a', '/usr/lib64/libboinc_api.a'],
	sources = ['boincmodule.C'],
	libraries=['stdc++', 'boinc', 'boinc_api', 'dl'],
	library_dirs=['/usr/lib64/'],
	include_dirs=['/usr/include/boinc/'])

setup (name = 'PyBoinc',
       version = '0.1',
       description = 'Basic python bindings for BOINC network computing package',
       ext_modules = [pyboinc])
