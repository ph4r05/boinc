import os, sys, shutil 
import fnmatch, logging, zipfile

lib_ext = 'so'
if sys.platform == 'win32':
    lib_ext = 'dll'

logging.basicConfig(format='%(asctime)s [%(levelname)s] %(message)s', datefmt='%Y-%m-%d,%H:%M:%S', level=logging.DEBUG)

def scan_files(dir, pattern):
    fileList = []
    for root, subFolders, files in os.walk(dir):
        for file in files:
            if fnmatch.fnmatch(file, pattern):
                fileList.append(os.path.join(root,file))
    return fileList

if (not os.path.exists('dist')):
    os.makedirs('dist')
    
currentDir = os.getcwd() # save current dir
os.chdir('../..')  # go to root of simulation

distPath = os.path.join(currentDir, 'bundle') # where to put files
scanData = [
    ['WSN/simulations', '*.ned', '', True],
    ['WSN/simulations', '*.xml', '', True],
    ['WSN/simulations', '*.exe', '', True],
    ['WSN/simulations', '*.ini', '', True],
    ['WSN/src', '*.ned', '', True],
    ['WSN/src', '*.%s' % lib_ext, '', True],
    ['MiXiM/src', '*.ned', '', True],
    ['MiXiM/src', '*.%s' % lib_ext, '', True],
    ['MiXiM/src/base', '*.%s' % lib_ext, 'lib', False],
    ['MiXiM/src/modules', '*.%s' % lib_ext, 'lib', False],
    [os.path.join(currentDir, 'lib'), '*.%s' % lib_ext, 'lib', False],
]

# remove old bundle
if (os.path.exists(distPath)):
    shutil.rmtree(distPath)

# copy neccessary files
for data in scanData:
    
    for file in scan_files(data[0], data[1]):
        
        if (data[3]):
            newSubPath = file            
        else:
            newSubPath = os.path.basename(file)
            
        newPath = os.path.relpath(os.path.join(distPath, data[2], newSubPath))
        newDir = os.path.dirname(newPath)
        
        if (not os.path.exists(newDir)):
            os.makedirs(newDir)
        
        logging.info('Copying %s to %s' % (file, newPath))
        shutil.copyfile(file, newPath)

logging.info("Creating archive")
bundleZip = zipfile.ZipFile(os.path.join(currentDir, 'dist', "bundle.zip"), 'w', zipfile.ZIP_DEFLATED)
for root, subFolders, files in os.walk(distPath):
    for file in files:
        # make path relative to distPath
        newPath = os.path.join(root, file).replace(distPath, '')
        # add files to zip
        bundleZip.write(os.path.join(root, file), newPath)
bundleZip.close()
logging.info("Done")

os.chdir(currentDir) # go back