#!/usr/bin/env python

import boinc_path_config
import sys, os, os.path, shutil
import hashlib, base64
import logging
import random
import hashlib

logging.basicConfig(format='%(asctime)s [%(levelname)s] %(message)s', level=logging.DEBUG)

class BruteGen:
    """Word generator for brute force attacks"""
    def __init__(self):
        # basic key space characteristics
        self.length  = 4
        self.charset = "abcdefghijklmnopqrstuvwxyz0123456789"
        
        # how many fragments to split?
        self.fragments = 10
        
        # compute size of key space
        self._initCharset()
        
    def _initCharset(self):
        """Update charset settings"""
        self.charsetl = len(self.charset)
        self.lastIndex = pow(self.charsetl, self.length)-1L
        self.increment = self.lastIndex/self.fragments
        
    def setCharset(self, newCharset):
        """Set new alphabet"""
        self.charset = newCharset
        self._initCharset()
        
    def setFragments(self, newFrags):
        self.fragments = newFrags
        self._initCharset()
        
    def setLength(self, newLen):
        self.length = newLen
        self._initCharset()
        
    def makeword(self, code, ln, charset):
        """make word representation from long code"""
        charsetl = len(charset)
        result=''
        for i in range(ln-1, -1, -1):
            fac = code/pow(charsetl,i)
            result = result + str(charset[min(fac, charsetl-1)])
            code -= fac*pow(charsetl,i)
        return result
    
    def makeFragments(self, fragNum=-1):
        """Returns fragments over key space"""
        wu = []
        lwordc=0
        
        # just one particular fragment
        if fragNum>=0:
            startNum = min(self.increment * fragNum, self.lastIndex)
            stopNum  = min(self.increment * (fragNum + 1), self.lastIndex)
            wu.append((self.makeword(startNum, self.length, self.charset), self.makeword(stopNum, self.length, self.charset)))
            return wu
        
        # all fragments
        for i in range(0,self.fragments):
            fword = self.makeword(min(lwordc, self.lastIndex), self.length, self.charset)
            lwordc+=self.increment+1
            lword = self.makeword(min(lwordc-1, self.lastIndex), self.length, self.charset)
            wu.append((fword, lword))
            #print "fragment ", i, " fword: ", fword, " lword: ", lword, ' space: ', self.increment
        
        return wu
    
    def getRandom(self, num=1):
        """Generates num of random words from this key space"""
        res = []
        for i in range(0,num):
            strr=''
            for j in range(0, self.length):
                strr += self.charset[random.randint(0, self.charsetl-1)]
            res.append(strr)
        
        return res
    
class WorkGenerator:

    def __init__(self):
        self.bgen = BruteGen()
        self.wu_name = ""
        

    def parse_args(self, args):
        args.reverse()
        tc_args = []
        while(len(args)):
            arg = args.pop()
            if arg == '-s':
                arg = args.pop()
                self.wordlist_size = int(arg)
            elif arg == '-w':
                arg = args.pop()
                self.wordlist = arg
            elif arg == '-t':
                arg = args.pop()
                self.truecrypt_volume = arg
            elif arg == '-n':
                arg = args.pop()
                self.wu_name = arg            
            else:
                raise Exception("Unknown argument %s" % arg)
        
    def getSpecXML(self, fword='aaaa', lword='zzzz', charset='abcdefghijklmnopqrstuvwxyz0123456789', formatHash='raw-md5', hashes=()):
        xmlPattern="""
<format>%s</format>
<start>%s</start>
<stop>%s</stop>
<charset>%s</charset>
%s
"""     
        hashXml=''
        if len(hashes)>0:
            hashXml='<newhashes>\n\t<hash>' + '</hash>\n\t<hash>'.join(hashes) + '</hash>\n</newhashes>'
        return xmlPattern % (formatHash, fword, lword, charset, hashXml)
        
    def getGenWrapperWu(self):
        pattern="""
IN=`boinc resolve_filename specxml.xml`
OUT=`boinc resolve_filename johnresult_hash.xml`
echo "Going to execute john, in: $IN out: $OUT"
./john --verbose --gijohn=127.0.0.1:80 --incxml=$IN --outxml=$OUT
echo "John executed"       
cp johnresult_hash.xml $OUT
echo "Ending..."
"""
        return pattern
        
    def get_download_path(self, fname):
        cmd = "./bin/dir_hier_path " + fname
        res_fname = os.popen(cmd, "r").read().strip()
        return res_fname
        
    def get_fname(self, fname):
        res_fname = self.get_download_path(fname)
        return res_fname
        
    def save_file(self, fname, data):
        fpath = os.path.dirname(fname)
        if not os.path.exists(fpath):
            os.makedirs(fpath)
        
        f = open(fname,'w')
        f.write(data)
        f.flush()
        f.close()
        return fname
        
    def save_config_file(self, counter, xml):       
        fname = os.path.join("specxml_%s_%04d" % (self.wu_name, counter)) 
        fname = self.get_fname(fname)
        return self.save_file(fname, xml)
    
    def save_wuscript(self, counter, data):
        fname = os.path.join("wu_%s_%04d.sh" % (self.wu_name, counter)) 
        fname = self.get_fname(fname)
        return self.save_file(fname, data)
    
    def getRandomHashes(self, num=100):
        """Returns list of random hashes"""
        l = self.bgen.getRandom(num)
        return [hashlib.md5(x).hexdigest() for x in l]
        
    def generate_v3(self):
        self.bgen.setLength(6)
        self.bgen.setFragments(15)
        frags = self.bgen.makeFragments()
        
        # generate random hashes
        hashes = self.getRandomHashes(1000)
        
        for (id,frag) in enumerate(frags):
            wu_name = "%s[%04d]" % (self.wu_name, id)
            
            # stage config file
            xml = self.getSpecXML(frag[0], frag[1], self.bgen.charset, 'raw-md5', hashes)
            self.save_config_file(id, xml)
            
            # wu config
            wudata = self.getGenWrapperWu()
            self.save_wuscript(id, wudata)
                
            # Call create_work
            cmd =  "./bin/create_work -appname JohnTheRipper -wu_name %s" % wu_name
            cmd += " -wu_template templates/JohnTheRipper/default_in.xml" 
            cmd += " -result_template templates/JohnTheRipper/default_out.xml" 
            cmd +=  " " + "wu_%s_%04d.sh " % (self.wu_name, id) + "specxml_%s_%04d" % (self.wu_name, id)
            
            os.system(cmd)
            logging.debug("Command line: %s",  cmd)
            logging.info("Generated workunit %s",  "%s[%04d]" % (self.wu_name, id))
        
        logging.info("Generated %d workunits", len(frags))
        
    def run(self):
        logging.info("Starting work generator")
        self.parse_args(sys.argv[1:])
        
        if self.wu_name == "":
            raise Exception("Please specify wu_name (argument -n)")
        
        self.generate_v3()
        
gen = WorkGenerator()
gen.run()
