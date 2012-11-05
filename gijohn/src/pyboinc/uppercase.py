#!/usr/bin/env python
import sys, string
from boinc import *

INPUT_FILENAME = 'in'
OUTPUT_FILENAME = 'out'

def worker():
	input_path = boinc_resolve_filename(INPUT_FILENAME)
	infile = boinc_fopen(input_path, 'r')

	output_path = boinc_resolve_filename(OUTPUT_FILENAME)
	outfile = boinc_fopen(output_path, 'w')

	char = infile.read(1)
	while char != '':
		outfile.write(string.upper(char))
		char = infile.read(1)

	outfile.close()
	infile.close()
	


retval = boinc_init()

if (retval):
	sys.exit(retval)

worker()

boinc_finish()
