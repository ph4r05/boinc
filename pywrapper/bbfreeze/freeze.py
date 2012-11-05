# bb_setup.py
from bbfreeze import Freezer
 
f = Freezer(distdir="bb-binary")
f.include_py = True
f.use_compression = True
f.addScript("test.py")
f()
