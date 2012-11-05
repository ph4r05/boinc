import logging
import sys, os, os.path, shutil
import zipfile
import re

logging.basicConfig(format='%(asctime)s [%(levelname)s] %(message)s', level=logging.DEBUG)

class Evaluator():
    
    def __init__(self):
        self.individual_file = None
        self.network = 'BMacTest'
        self.simParams = '-u Cmdenv -f boinc.ini -f omnetpp.ini'
        
        self.currentDir = os.getcwd() # get current dir
        self.baseDir = os.path.join(self.currentDir, 'simulation')
        
    def parse_args(self, args):
        args.reverse()
        unknown_params = []
        while(len(args)):
            arg = args.pop()
            if (arg == '-individual' or arg == '--individual'):
                arg = args.pop()
                self.individual_file = arg
            elif (arg == '-network' or arg == '--network'):
                arg = args.pop()
                self.network = arg
            else:
                unknown_params.append(arg)
        
        self.simParams = self.simParams + " "+" ".join(unknown_params) # parameters will be propagated to simulation
        
        if self.individual_file is None:
            raise Exception("File with individual was not specified (--individual <file> or -individual <file>)")

    def boinc_resolve_filename(self, virtual_name):
        real_name = virtual_name
        try:
            f = open(virtual_name, 'r')
            line = f.readline()
            f.close()
            m = re.search('\<soft_link\>(.*)\<\/soft_link\>', line)
            real_name = os.path.relpath(os.path.join(os.path.dirname(virtual_name), m.group(1)))
        except:
            logging.debug("File " + virtual_name + " is not a soft link")
        return real_name
                
    def run(self):
        self.parse_args(sys.argv[1:])
        self.evaluate()
        logging.info("Done")
        
    def cleanup(self):
        logging.info("Cleaning up")
        
        #shutil.rmtree('results')
        os.chdir(self.currentDir)
        shutil.rmtree(self.baseDir)
        
    def evaluate(self):
        if (not os.path.exists(self.individual_file)):
            raise Exception("File \"" + self.individual_file+"\" does not exist")

        if os.path.exists(self.baseDir):
            shutil.rmtree(self.baseDir)            
        os.makedirs(self.baseDir)
        
        os.chdir(self.baseDir)  # go to root of simulation
        
        # get back path to boinc root
        backPath = os.path.relpath(self.currentDir)
        
        base_archive = self.boinc_resolve_filename(os.path.join(backPath, 'bundle.zip'))
        logging.info("Extracting files from "+base_archive)
        
        sourceZip = zipfile.ZipFile(base_archive, 'r')
        sourceZip.extractall(".")
        sourceZip.close()
        
        if not os.path.exists("results"):
            os.makedirs("results")
        shutil.copyfile(os.path.join(backPath, self.individual_file), "results/individual.ini")
        
        logging.info("Running simulation")
        networkDir = os.path.join('WSN', 'simulations', self.network)
        os.chdir(networkDir)
        networkBackPath = os.path.relpath(self.baseDir)
        
        # set PATH for external libraries
        os.environ['PATH']=";".join([os.path.join(networkBackPath, 'lib'), os.environ['PATH']])        
        
        cmd = "%s -n \"%s\" -l \"%s\" -l \"%s\" -l \"%s\" --result-dir=\"%s\" %s -f %s" % (
                # Network
                self.network,
                # NED files
                ";".join([".", os.path.join(networkBackPath, "MiXiM", "src"), os.path.join(networkBackPath, "WSN", "src")]), 
                # Simulation models
                os.path.join(networkBackPath, 'MiXiM/src/base/miximbase'),
                os.path.join(networkBackPath, 'MiXiM/src/modules/miximmodules'),
                os.path.join(networkBackPath, 'WSN/src/WSN'),
                # Where to put results
                os.path.join(networkBackPath, 'results'),
                # Other parameters
                self.simParams, 
                # Path to individual.ini
                os.path.join(networkBackPath, 'results/individual.ini')
        )
        
        logging.info("Running simulation: %s" % cmd)
        os.system(cmd)
        os.chdir(networkBackPath)
        
        logging.info("Collecting results, %d file(s)" % len(os.listdir("results")))
        resultsZip = zipfile.ZipFile(os.path.join(backPath, "results.zip"), 'w', zipfile.ZIP_DEFLATED)
        for fname in os.listdir("results"):
            resultsZip.write(os.path.join("results",fname), fname)
        resultsZip.close()
        
        self.cleanup()
                    

eval = Evaluator()
try:
    eval.run()
except Exception:
    eval.cleanup()
    raise
