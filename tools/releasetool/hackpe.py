import pefile
import sys
import pprint
import fnmatch
import os
import shutil

def modversion(f):
	
	pe = pefile.PE(f)
	pe.OPTIONAL_HEADER.MajorOperatingSystemVersion = 5 
	pe.OPTIONAL_HEADER.MinorOperatingSystemVersion = 2
	pe.write(filename=f + ".mod")
	pe.close()

	shutil.copy(f + ".mod", f)
	os.remove(f + ".mod")

matches = []
for root, dirnames, filenames in os.walk(sys.argv[1]):
  for filename in fnmatch.filter(filenames, '*.dll'):
    matches.append(os.path.join(root, filename))
  for filename in fnmatch.filter(filenames, '*.exe'):
    matches.append(os.path.join(root, filename))

for f in matches:
	modversion(f)