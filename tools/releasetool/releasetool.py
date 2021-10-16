#
# LC7 Release Tool
#
########################################################################################################################

from __future__ import print_function
import sys

def eprint(*args, **kwargs):
    print(*args, file=sys.stderr, **kwargs)

import os
import argparse
import json
try:
    import boto
    import boto.s3.connection
except ImportError:
    eprint("Please install boto. `pip install boto`")
    sys.exit(1)
#try:
#    import xmltodict
#except ImportError:
#    alert("Please install xmltodict. `pip install xmltodict`")
#    sys.exit(1)
import platform
import hashlib
import subprocess
import urlparse
import shlex
import errno
import datetime
import fnmatch
import distutils.spawn
import shutil
import zipfile
import s3engine

## COLORS ##

try:
    import colorama
except ImportError:
    eprint("Please install colorama. `pip install colorama`")
    sys.exit(1)
from colorama import Fore, Back, Style
colorama.init()

########################################################################################################################

## LC7 ENVIRONMENTS ##

VERSIONFILE = r"..\..\lc7\include\appversion.h"
MANIFESTS = r"..\..\dist"
LINKS_BUCKET = r"s3://installers.lc7/"
ENVS = {
    "windows": {
        "release32": {
            "suffix": ".exe",
            "version_string": r"$$VERSION_NUMBER$$ Win32",
            "installerdir": r"win32",
            "root": r"..\..\build_win32\dist\RelWithDebInfo",
            "plugins": r"..\..\build_win32\dist\RelWithDebInfo\lcplugins",
            "output": r"..\..\build_win32\output\RelWithDebInfo_Win32",
            "exclude": "*.pdb;Thumbs.db;*.lib;*.exp;*.PreARM;*.manifest",
            "https_installers": "https://s3.amazonaws.com/installers.lc7/lc7/win32/release",
            "s3_installers": "s3://installers.lc7/lc7/win32/release",
            "https_updates": "https://s3.amazonaws.com/updates.lc7/lc7/win32/release",
            "s3_updates": "s3://updates.lc7/lc7/win32/release",
            "https_debugs": "https://s3.amazonaws.com/debugs.lc7/lc7/win32/release",
            "s3_debugs": "s3://debugs.lc7/lc7/win32/release",
            "https_plugins": "https://s3.amazonaws.com/updates.lc7/lc7/lc7/win32/release/lcplugins",
            "s3_plugins": "s3://updates.lc7/lc7/win32/release/lcplugins",
            "download_prefixes": [ "win32", "win32/release" ],
            "buildcommand": [
                "$$MSBUILD$$",
                "..\\..\\build_win32\\L0phtcrack 7.sln",
                "/t:Build",
                "/p:Configuration=RelWithDebInfo",
                "/p:Platform=Win32",
            ],
            "rebuildcommand": [
                "$$MSBUILD$$",
                "..\\..\\build_win32\\L0phtcrack 7.sln",
                "/t:Rebuild",
                "/p:Configuration=RelWithDebInfo",
                "/p:Platform=Win32"
            ],
            "makeinstaller": [
                r"$$NSISDIR$$\makensis.exe",
                r"/DVERSION_STRING=$$VERSION_STRING$$",
                r"/DVERSION_NUMBER=$$VERSION_NUMBER$$.$$VERSION_DATE$$$$VERSION_TIME$$",
                r"/DINPUTDIR=$$INPUTDIR$$",
                r"/DOUTFILE=$$OUTFILE$$",
                r"/V4",
                r"setup.nsi"
            ],
            "makeupdate": [
                r"$$NSISDIR$$\makensis.exe",
                r"/DVERSION_STRING=$$VERSION_STRING$$",
                r"/DVERSION_NUMBER=$$VERSION_NUMBER$$.$$VERSION_DATE$$$$VERSION_TIME$$",
                r"/DINPUTDIR=$$INPUTDIR$$",
                r"/DOUTFILE=$$OUTFILE$$",
                r"/DFULL=$$FULL$$",
                r"/V4",
                r"update.nsi"
            ],
            "signtool": [
                r"signtool",
                r"sign",
                r"/t",
                r"http://timestamp.digicert.com",
                r"/a",
                r"$$EXECUTABLE$$"
            ],
            "signfilepatterns": [ "*.exe", "*.dll" ]
        },
        "beta32": {
            "suffix": ".exe",
            "version_string": r"$$VERSION_NUMBER$$ Win32 BETA $$VERSION_DATE$$$$VERSION_TIME$$",
            "installerdir": r"win32",
            "root": r"..\..\build_win32\dist\Beta",
            "plugins": r"..\..\build_win32\dist\Beta\lcplugins",
            "output": r"..\..\build_win32\output\Beta_Win32",
            "exclude": "*.pdb;Thumbs.db;*.lib;*.exp;*.PreARM;*.manifest",
            "https_installers": "https://s3.amazonaws.com/installers.lc7/lc7/win32/beta",
            "s3_installers": "s3://installers.lc7/lc7/win32/beta",
            "https_updates": "https://s3.amazonaws.com/updates.lc7/lc7/win32/beta",
            "s3_updates": "s3://updates.lc7/lc7/win32/beta",
            "https_debugs": "https://s3.amazonaws.com/debugs.lc7/lc7/win32/beta",
            "s3_debugs": "s3://debugs.lc7/lc7/win32/beta",
            "https_plugins": "https://s3.amazonaws.com/updates.lc7/lc7/lc7/win32/beta/lcplugins",
            "s3_plugins": "s3://updates.lc7/lc7/win32/beta/lcplugins",
            "download_prefixes": [ "win32/beta" ],
            "buildcommand": [
                "$$MSBUILD$$",
                "..\\..\\build_win32\\L0phtcrack 7.sln",
                "/t:Build",
                "/p:Configuration=Beta",
                "/p:Platform=Win32"
            ],
            "rebuildcommand": [
                "$$MSBUILD$$",
                "..\\..\\build_win32\\L0phtcrack 7.sln",
                "/t:Rebuild",
                "/p:Configuration=Beta",
                "/p:Platform=Win32"
            ],
            "makeinstaller": [
                r"$$NSISDIR$$\makensis.exe",
                r"/DVERSION_STRING=$$VERSION_STRING$$",
                r"/DVERSION_NUMBER=$$VERSION_NUMBER$$.$$VERSION_DATE$$$$VERSION_TIME$$",
                r"/DINPUTDIR=$$INPUTDIR$$",
                r"/DOUTFILE=$$OUTFILE$$",
                r"/V4",
                r"setup.nsi"
            ],
            "makeupdate": [
                r"$$NSISDIR$$\makensis.exe",
                r"/DVERSION_STRING=$$VERSION_STRING$$",
                r"/DVERSION_NUMBER=$$VERSION_NUMBER$$.$$VERSION_DATE$$$$VERSION_TIME$$",
                r"/DINPUTDIR=$$INPUTDIR$$",
                r"/DOUTFILE=$$OUTFILE$$",
                r"/DFULL=$$FULL$$",
                r"/V4",
                r"update.nsi"
            ],
            "signtool": [
                r"signtool",
                r"sign",
                r"/t",
                r"http://timestamp.digicert.com",
                r"/a",
                r"$$EXECUTABLE$$"
            ],
            "signfilepatterns": [ "*.exe", "*.dll" ]
        },
        "release64": {
            "suffix": ".exe",
            "version_string": r"$$VERSION_NUMBER$$ Win64",
            "installerdir": r"win64",
            "root": r"..\..\build_win64\dist\RelWithDebInfo",
            "plugins": r"..\..\build_win64\dist\RelWithDebInfo\lcplugins",
            "output": r"..\..\build_win64\output\RelWithDebInfo_Win64",
            "exclude": "*.pdb;Thumbs.db;*.lib;*.exp;*.PreARM;*.manifest",
            "https_installers": "https://s3.amazonaws.com/installers.lc7/lc7/win64/release",
            "s3_installers": "s3://installers.lc7/lc7/win64/release",
            "https_updates": "https://s3.amazonaws.com/updates.lc7/lc7/win64/release",
            "s3_updates": "s3://updates.lc7/lc7/win64/release",
            "https_debugs": "https://s3.amazonaws.com/debugs.lc7/lc7/win64/release",
            "s3_debugs": "s3://debugs.lc7/lc7/win64/release",
            "https_plugins": "https://s3.amazonaws.com/updates.lc7/lc7/lc7/win64/release/lcplugins",
            "s3_plugins": "s3://updates.lc7/lc7/win64/release/lcplugins",
            "download_prefixes": [ "win64", "win64/release" ],
            "buildcommand": [
                "$$MSBUILD$$",
                "..\\..\\build_win64\\L0phtcrack 7.sln",
                "/t:Build",
                "/p:Configuration=RelWithDebInfo",
                "/p:Platform=x64"
            ],
            "rebuildcommand": [
                "$$MSBUILD$$",
                "..\\..\\build_win64\\L0phtcrack 7.sln",
                "/t:Rebuild",
                "/p:Configuration=RelWithDebInfo",
                "/p:Platform=x64"
            ],
            "makeinstaller": [
                r"$$NSISDIR$$\makensis.exe",
                r"/DVERSION_STRING=$$VERSION_STRING$$",
                r"/DVERSION_NUMBER=$$VERSION_NUMBER$$.$$VERSION_DATE$$$$VERSION_TIME$$",
                r"/DINPUTDIR=$$INPUTDIR$$",
                r"/DOUTFILE=$$OUTFILE$$",
                r"/V4",
                r"setup.nsi"
            ],
            "makeupdate": [
                r"$$NSISDIR$$\makensis.exe",
                r"/DVERSION_STRING=$$VERSION_STRING$$",
                r"/DVERSION_NUMBER=$$VERSION_NUMBER$$.$$VERSION_DATE$$$$VERSION_TIME$$",
                r"/DINPUTDIR=$$INPUTDIR$$",
                r"/DOUTFILE=$$OUTFILE$$",
                r"/DFULL=$$FULL$$",
                r"/V4",
                r"update.nsi"
            ],
            "signtool": [
                r"signtool",
                r"sign",
                r"/t",
                r"http://timestamp.digicert.com",
                r"/a",
                r"$$EXECUTABLE$$"
            ],
            "signfilepatterns": [ "*.exe", "*.dll" ]
        },
        "beta64": {
            "suffix": ".exe",
            "version_string": r"$$VERSION_NUMBER$$ Win64 BETA $$VERSION_DATE$$$$VERSION_TIME$$",
            "installerdir": r"win64",
            "root": r"..\..\build_win64\dist\Beta",
            "plugins": r"..\..\build_win64\dist\Beta\lcplugins",
            "output": r"..\..\build_win64\output\Beta_Win64",
            "exclude": "*.pdb;Thumbs.db;*.lib;*.exp;*.PreARM;*.manifest",
            "https_installers": "https://s3.amazonaws.com/installers.lc7/lc7/win64/beta",
            "s3_installers": "s3://installers.lc7/lc7/win64/beta",
            "https_updates": "https://s3.amazonaws.com/updates.lc7/lc7/win64/beta",
            "s3_updates": "s3://updates.lc7/lc7/win64/beta",
            "https_debugs": "https://s3.amazonaws.com/debugs.lc7/lc7/win64/beta",
            "s3_debugs": "s3://debugs.lc7/lc7/win64/beta",
            "https_plugins": "https://s3.amazonaws.com/updates.lc7/lc7/lc7/win64/beta/lcplugins",
            "s3_plugins": "s3://updates.lc7/lc7/win64/beta/lcplugins",
            "download_prefixes": [ "win64/beta" ],
            "buildcommand": [
                "$$MSBUILD$$",
                "..\\..\\build_win64\\L0phtcrack 7.sln",
                "/t:Build",
                "/p:Configuration=Beta",
                "/p:Platform=x64"
            ],
            "rebuildcommand": [
                "$$MSBUILD$$",
                "..\\..\\build_win64\\L0phtcrack 7.sln",
                "/t:Rebuild",
                "/p:Configuration=Beta",
                "/p:Platform=x64"
            ],
            "makeinstaller": [
                r"$$NSISDIR$$\makensis.exe",
                r"/DVERSION_STRING=$$VERSION_STRING$$",
                r"/DVERSION_NUMBER=$$VERSION_NUMBER$$.$$VERSION_DATE$$$$VERSION_TIME$$",
                r"/DINPUTDIR=$$INPUTDIR$$",
                r"/DOUTFILE=$$OUTFILE$$",
                r"/V4",
                r"setup.nsi"
            ],
            "makeupdate": [
                r"$$NSISDIR$$\makensis.exe",
                r"/DVERSION_STRING=$$VERSION_STRING$$",
                r"/DVERSION_NUMBER=$$VERSION_NUMBER$$.$$VERSION_DATE$$$$VERSION_TIME$$",
                r"/DINPUTDIR=$$INPUTDIR$$",
                r"/DOUTFILE=$$OUTFILE$$",
                r"/DFULL=$$FULL$$",
                r"/V4",
                r"update.nsi"
            ],
            "signtool": [
                r"signtool",
                r"sign",
                r"/t",
                r"http://timestamp.digicert.com",
                r"/a",
                r"$$EXECUTABLE$$"
            ],
            "signfilepatterns": [ "*.exe", "*.dll" ]
        }
    }

}

# Detect environment
if platform.architecture()[1]=="WindowsPE":
    print("Selecting Windows environment")
    BUILDS = ENVS["windows"]
    
    # Get NSIS directory
    NSISDIR = os.getenv("NSISDIR",None)
    if not NSISDIR or not os.path.exists(NSISDIR):
        if os.path.exists("C:\\Program Files (x86)\\NSIS"):
            NSISDIR = "C:\\Program Files (x86)\\NSIS"
        elif os.path.exists("D:\\NSIS"):
            NSISDIR = "D:\\NSIS"
        else:
            print("Unknown NSIS location. Specify NSISDIR environment variable.")
            sys.exit(1)

    # Get MSBUILD location
    MSBUILD = os.getenv("MSBUILD", None)
    if not MSBUILD or not os.path.exists(MSBUILD):
        MSBUILD = distutils.spawn.find_executable("MSBuild.exe")
        if not MSBUILD:
            MSBUILD = r"C:\Program Files (x86)\MSBuild\12.0\Bin\amd64\MSBuild.exe"
            if not os.path.exists(MSBUILD):
                print("Unknown MSBuild location. Specify MSBUILD environment variable.")
                sys.exit(1)

else:
    BUILDS = None
    print("Unknown environment!")
    sys.exit(1)


# Get boto variables
LC7_RELEASE_ACCESS_KEY_ID = os.getenv("LC7_RELEASE_ACCESS_KEY_ID",None)
LC7_RELEASE_SECRET_ACCESS_KEY = os.getenv("LC7_RELEASE_SECRET_ACCESS_KEY",None)

########################################################################################################################

## S3 Utilities ##
def get_s3_connection():
    conn = boto.s3.connection.S3Connection(LC7_RELEASE_ACCESS_KEY_ID, LC7_RELEASE_SECRET_ACCESS_KEY, is_secure=False)
    if not conn:
        print("Unable to connect to S3")
        sys.exit(3)
    return conn


def get_bucket_path(s3url):
    conn = get_s3_connection()
    parts = urlparse.urlparse(s3url)
    if parts.scheme!="s3":
        print("Invalid s3url")
        sys.exit(3)
    bucket = parts.netloc
    path = parts.path.strip("/")

    bucket = conn.get_bucket(bucket)
    if not bucket:
        print("Unable to get bucket")
        sys.exit(4)

    return bucket, path


def s3status(cur, max):
    done = (cur*20)/max
    notdone = 20-done
    sys.stdout.write("\r["+("#"*done)+("."*notdone)+"] %.2f%%" % (cur*100.0/max))
    sys.stdout.flush()



########################################################################################################################

## LC7 Utilities ##

def find_matching_files(rootdir, wildcard):
    matches = []
    for root, dirnames, filenames in os.walk(rootdir):
        for filename in fnmatch.filter(filenames, wildcard):
            matches.append(os.path.join(root, filename))
    return matches


def get_app_version():
    f = open(VERSIONFILE, "r")
    versionnumber = None
    versiondate = None
    versiontime = None
    for l in f.readlines():
        if "VERSION_NUMBER" in l:
            versionnumber = l.split()[2].strip("\"")
        if "VERSION_DATE" in l:
            versiondate = l.split()[2].strip("\"")
        if "VERSION_TIME" in l:
            versiontime = l.split()[2].strip("\"")
        if "END_EXTERNAL" in l:
            break
    if versionnumber is None:
        print("Unable to get current app version")
        sys.exit(5)
    return (versionnumber, versiondate, versiontime)


def compare_version(a, b):
    an=[int(n) for n in a.split(".")]
    bn=[int(n) for n in b.split(".")]
    if len(an)!=len(bn):
        print("Version number length inconsistent!")
        sys.exit(7)
    p=0
    while an[p] == bn[p]:
        p+=1
        if p==len(an):
            return 0
    if an[p]>bn[p]:
        return 1
    return -1


def parse_replacements(s, **kwargs):
    for kw in kwargs.iteritems():
        s = s.replace("$$"+kw[0].upper()+"$$",kw[1])
    return s
        

def parse_command(cmd, **kwargs):
    subcmd=[]
    for arg in cmd:
        arg = parse_replacements(arg, **kwargs)
        subcmd.append(arg)
    return subcmd


def get_digests(infile):
    md5 = hashlib.md5()
    md5.update(open(infile,"rb").read())
    md5digest = md5.hexdigest()

    sha256 = hashlib.sha256()
    sha256.update(open(infile,"rb").read())
    sha256digest = sha256.hexdigest()

    return (md5digest, sha256digest)


def mkdir_p(path):
    try:
        os.makedirs(path)
    except OSError as exc: # Python >2.5
        if exc.errno == errno.EEXIST and os.path.isdir(path):
            pass
        else: raise

def sign_one_executable(signtool, signfile):
    cmd = parse_command(signtool, executable=signfile)
    retval = subprocess.call(cmd)
    if retval != 0:
        print("Code signing error")
        sys.exit(retval)


def sign_all_executables(inputdir, signtool, signfilepatterns):
    for sfpat in signfilepatterns:
        signfiles = find_matching_files(inputdir, sfpat)
        for sf in signfiles:
            # skip dump dlls
            if sf.endswith("lc7dump.dll") or sf.endswith("lc7dump64.dll"):
                continue
            sign_one_executable(signtool, sf)


    
########################################################################################################################

## LC7 COMMANDS ##


def cmd_pushplugin(args):
    pass

def cmd_pushplugins(args):
    pass

def cmd_bumpversion(args):

    # Get in-code version number
    (version_number, version_date, version_time) = get_app_version()

    print("Current Version Number: "+version_number)
    print("Current Version Date: "+version_date)
    print("Current Version Time: "+version_time)

    # Bump minor version number if requested
    if args.newversion:
        version_parts = [int(n) for n in version_number.split(".")]
        version_parts[1] += 1
        version_parts[2] = 0
        version_number = ".".join([str(n) for n in version_parts])
    else:
        version_parts = [int(n) for n in version_number.split(".")]
        version_parts[2] += 1
        version_number = ".".join([str(n) for n in version_parts])

    now = datetime.datetime.now()
    version_date = now.strftime("%Y%m%d")
    version_time = now.strftime("%H%M%S")

    # Replace version in appversion.h
    infile = open(VERSIONFILE, "r")

    outlines = []
    replacing = False
    for l in infile.readlines():
        if replacing:
            if "// END_EXTERNAL" in l:
                outlines.append(l)
                replacing = False
        elif "// EXTERNAL" in l:
            outlines.append(l)

            outlines.append("#define VERSION_NUMBER \""+version_number+"\"\n")
            outlines.append("#define VERSION_DATE \""+version_date+"\"\n")
            outlines.append("#define VERSION_TIME \""+version_time+"\"\n")

            replacing = True
        else:
            outlines.append(l)

    infile.close()

    outfile = open(VERSIONFILE, "w")
    outfile.writelines(outlines)
    outfile.close()

    # Replace version in system plugin manifests

    manifests = find_matching_files(MANIFESTS, "manifest.json")
    for m in manifests:
        mf = json.loads(open(m, "r").read())
        if mf['isSystemLibrary']:
            print("Updating '"+m+"'")
            mf['internalVersion'] += 1
            mf['displayVersion'] = version_number+" ("+version_date+version_time+")"
            mf['releaseDate'] = now.isoformat()
        else:
            print("Skipping '"+m+"'")
        open(m, "w").write(json.dumps(mf, indent=4, sort_keys=True))

    # Done
    print("New Version Number: "+version_number)
    print("New Version Date: "+version_date)
    print("New Version Time: "+version_time)
    print("Build required for changes to take effect.")


def cmd_build(args):

    global BUILDS
    # Get build
    if args.build not in BUILDS:
        print("Unknown build!")
        sys.exit(1)
    ENV = BUILDS[args.build]

    if args.rebuild:
        cmd = ENV["rebuildcommand"]
    else:
        cmd = ENV["buildcommand"]

    cmd = parse_command(cmd, msbuild=MSBUILD)

    retval = subprocess.call(cmd)
    if retval != 0:
        print("Build error")
        sys.exit(retval)



def cmd_buildinstaller(args):

    global BUILDS
    # Get build
    if args.build not in BUILDS:
        print("Unknown build!")
        sys.exit(1)
    ENV = BUILDS[args.build]

    # Get in-code version number
    (version_number, version_date, version_time) = get_app_version()
    version_string = parse_replacements(ENV["version_string"], version_number=version_number, version_date=version_date, version_time=version_time)
    print("App Version: "+version_string)

    # Get currently released version number
    if LC7_RELEASE_ACCESS_KEY_ID is not None and LC7_RELEASE_SECRET_ACCESS_KEY is not None:
        bucket, path = get_bucket_path(ENV["s3_installers"])
        key = bucket.get_key(path+"/current.json")
        if key is not None and key.exists():
            current = json.loads(key.get_contents_as_string())
            print("Current Version: "+str(current['version']))

            # Verify we aren't building the current version without force option (check we updated version numbers)
            if not args.force and current["version"] == version_string:
                print("You need to bump the version number and rebuild.")
                sys.exit(6)
        else:
            print("Current Version: none")

    inputdir = os.path.abspath(ENV["root"])
    print("Input directory: "+inputdir)

    #### Sign all executables
    sign_all_executables(inputdir, ENV["signtool"], ENV["signfilepatterns"])

    #### Build full installer

    outname = "lc7setup_v"+version_string.replace(" ", "_")+ENV["suffix"]
    outfile = os.path.join(os.path.abspath(ENV["output"]), outname)

    print("Installer file: "+outfile)

    cmd = parse_command(ENV["makeinstaller"], nsisdir=NSISDIR, version_string=version_string, version_number=version_number, version_date=version_date, version_time=version_time, inputdir=inputdir, outfile=outfile)

    #print("Command: "+str(cmd))

    # Make output directory
    mkdir_p(os.path.abspath(ENV["output"]))

    # Switch to environment directory
    os.chdir("installer")
    os.chdir(ENV["installerdir"])

    retval = subprocess.call(cmd)
    if retval != 0:
        print("Error building installer")
        sys.exit(retval)

    os.chdir("..")
    os.chdir("..")

    sign_one_executable(ENV["signtool"], outfile)

    #### Build Updater

    update_outname = "lc7update_v"+version_string.replace(" ", "_")+ENV["suffix"]
    update_outfile = os.path.join(os.path.abspath(ENV["output"]), update_outname)

    # Build Partial Updater

    print("Partial Update file: "+update_outfile)

    cmd = parse_command(ENV["makeupdate"], full=("0" if args.partial else "1"), nsisdir=NSISDIR, version_string=version_string, version_number=version_number, version_date=version_date, version_time=version_time, inputdir=inputdir, outfile=update_outfile)

    #print("Command: "+str(cmd))

    # Make output directory
    mkdir_p(os.path.abspath(ENV["output"]))

    # Switch to environment directory
    os.chdir("installer")
    os.chdir(ENV["installerdir"])

    retval = subprocess.call(cmd)
    if retval != 0:
        print("Error building updater")
        sys.exit(retval)

    os.chdir("..")
    os.chdir("..")

    sign_one_executable(ENV["signtool"], update_outfile)

    # Write debug files

    debug_outname = "lc7debug_v"+version_string.replace(" ", "_")+".zip"
    debug_outfile = os.path.join(os.path.abspath(ENV["output"]), debug_outname)

    print("Debug file: "+debug_outfile)
    zf = zipfile.ZipFile(debug_outfile, "w")

    for root, folders, files in os.walk(inputdir):
        for f in files:
            if f.lower().endswith(".pdb"):
                zf.write(os.path.join(root, f), f)

    zf.close()

    print(Fore.CYAN)
    print(os.path.basename(outfile))
    (md5digest, sha256digest) = get_digests(outfile)
    print("MD5 Digest: \t"+md5digest)
    print("SHA256 Digest: \t"+sha256digest)
    print("")
    print(os.path.basename(update_outfile))
    (md5digest, sha256digest) = get_digests(update_outfile)
    print("MD5 Digest: \t"+md5digest)
    print("SHA256 Digest: \t"+sha256digest)
    print(Fore.RESET)

def cmd_pushinstaller(args):

    # Get build
    if args.build not in BUILDS:
        print("Unknown build!")
        sys.exit(1)
    ENV = BUILDS[args.build]

    # Get release notes
    try:
        releasenotes = open(args.releasenotes, "r").read()
    except:
        print("Missing release notes")
        sys.exit(1)

    # Get in-code version number
    (version_number, version_date, version_time) = get_app_version()
    version_string = parse_replacements(ENV["version_string"], version_number=version_number, version_date=version_date, version_time=version_time)
    print("App Version: "+version_string)

    ###################### INSTALLER


    # Find built executable
    outname = "lc7setup_v"+version_string.replace(" ", "_")+ENV["suffix"]
    outfile = os.path.join(os.path.abspath(ENV["output"]), outname)
    print("Installer file: "+outfile)
    if not os.path.exists(outfile):
        print("Installer version "+version_string+" is not built, exiting.")
        sys.exit(1)

    (md5digest, sha256digest) = get_digests(outfile)
    print("MD5 Digest: \t"+md5digest)
    print("SHA256 Digest: \t"+sha256digest)
    print("Release Notes:\n"+releasenotes)

    # Upload release to S3
    bucket, path = get_bucket_path(ENV["s3_installers"])
    key = bucket.get_key(path+"/"+outname)
    if not key:
        key = bucket.new_key(path+"/"+outname)

    #key.set_contents_from_filename(outfile, cb=s3status, num_cb=100)
    s3 = s3engine.S3Engine(get_s3_connection(), progress_callback=s3engine.ConsoleProgressCallback())
    s3.put(outfile, ENV["s3_installers"]+"/"+outname, parallel=True)
    print(" Done.")

    # Print URL
    key.make_public()
    installer_url = ENV["https_installers"]+"/"+outname
    print(Fore.MAGENTA + "Installer URL:\n"+installer_url + Fore.RESET)

    # Get currently released version number
    currentkey = bucket.get_key(path+"/current.json")
    if not currentkey:
        currentkey = bucket.new_key(path+"/current.json")

    current = {"version": version_string, "md5digest": md5digest, "sha256digest": sha256digest, "url": installer_url, "releasenotes": releasenotes }

    currentkey.set_contents_from_string(json.dumps(current))
    currentkey.make_public()

    ###################### UPDATER

    # Find built executable
    outname = "lc7update_v"+version_string.replace(" ", "_")+ENV["suffix"]
    outfile = os.path.join(os.path.abspath(ENV["output"]), outname)
    print("Updater file: "+outfile)
    if not os.path.exists(outfile):
        print("Updater version "+version_string+" is not built, exiting.")
        sys.exit(1)

    (md5digest, sha256digest) = get_digests(outfile)
    print("MD5 Digest: \t"+md5digest)
    print("SHA256 Digest: \t"+sha256digest)

    print("Release Notes:\n"+releasenotes)

    # Upload release to S3
    bucket, path = get_bucket_path(ENV["s3_updates"])
    key = bucket.get_key(path+"/"+outname)
    if not key:
        key = bucket.new_key(path+"/"+outname)

    #key.set_contents_from_filename(outfile, cb=s3status, num_cb=100)
    s3 = s3engine.S3Engine(get_s3_connection(), progress_callback=s3engine.ConsoleProgressCallback())
    s3.put(outfile, ENV["s3_updates"]+"/"+outname, parallel=True)
    print(" Done.")

    # Print URL
    key.make_public()
    update_url = ENV["https_updates"]+"/"+outname
    print(Fore.MAGENTA + "Update URL:\n"+update_url + Fore.RESET)

    # Get currently released version number
    currentkey = bucket.get_key(path+"/current.json")
    if not currentkey:
        currentkey = bucket.new_key(path+"/current.json")

    current = {"version": version_string, "md5digest": md5digest, "sha256digest": sha256digest, "url": update_url, "releasenotes": releasenotes }

    currentkey.set_contents_from_string(json.dumps(current))
    currentkey.make_public()

    ###################### DEBUGS

    # Find built zipfile
    outname = "lc7debug_v"+version_string.replace(" ", "_")+".zip"
    outfile = os.path.join(os.path.abspath(ENV["output"]), outname)
    print("Debug file: "+outfile)
    if not os.path.exists(outfile):
        print("Debugs for version "+version_string+" are not built, exiting.")
        sys.exit(1)

    # Upload debugs to S3
    bucket, path = get_bucket_path(ENV["s3_debugs"])
    key = bucket.get_key(path+"/"+outname)
    if not key:
        key = bucket.new_key(path+"/"+outname)

    #key.set_contents_from_filename(outfile, cb=s3status, num_cb=100)
    s3 = s3engine.S3Engine(get_s3_connection(), progress_callback=s3engine.ConsoleProgressCallback())
    s3.put(outfile, ENV["s3_debugs"]+"/"+outname, parallel=True)
    print(" Done.")

    # Print URL
    # key.make_public()
    debugs_url = ENV["https_debugs"]+"/"+outname
    print("Debug URL:\n"+debugs_url)

    # Get currently released version number
    currentkey = bucket.get_key(path+"/current.json")
    if not currentkey:
        currentkey = bucket.new_key(path+"/current.json")

    current = {"version": version_string, "url": debugs_url }

    currentkey.set_contents_from_string(json.dumps(current))



def cmd_updatelinks(args):

    global BUILDS
    global LINKS_BUCKET
    
    # Collect all download prefixes, and pair with links from current.json
    download_prefixes={}
    for build_key in BUILDS:
        ENV = BUILDS[build_key]
        
        bucket, path = get_bucket_path(ENV["s3_installers"])
        key = bucket.get_key(path+"/current.json")
        if key is not None and key.exists():
            current = json.loads(key.get_contents_as_string())
            for prefix in ENV["download_prefixes"]:
                download_prefixes[prefix] = current["url"]
                print("{0} -> {1}".format(prefix, download_prefixes[prefix]))
        else:
            print("No download for build '{0}'".format(build_key))

    # Sort download prefixes from longest to shortest
    keys = sorted(download_prefixes.keys(), key=len, reverse=True)

    # Write routing rules out in order
    rules = boto.s3.website.RoutingRules()
    for key in keys:
        newkey=download_prefixes[key]
        if newkey.startswith("https://s3.amazonaws.com/"):
            newkey=newkey[25:]
        rules.add_rule(boto.s3.website.RoutingRule.when(key_prefix = key).then_redirect(
            hostname = "s3.amazonaws.com", 
            protocol = "https",
			http_redirect_code = 307,
            replace_key = newkey
            ))

    # Configure website
    bucket, path = get_bucket_path(LINKS_BUCKET)
    if bucket.configure_website(suffix = "index.html", error_key = "error.html", routing_rules = rules):
        print("Links updated successfully.")
    else:
        print("There was an error updating the links.")
    

########################################################################################################################

## MAIN ENTRYPOINT ##

if __name__ == "__main__":

    parser = argparse.ArgumentParser(prog=Fore.CYAN+sys.argv[0]+Fore.RESET, description=Fore.GREEN + "LC7 Release Tool" + Fore.RESET)
    subparsers = parser.add_subparsers()

    parser_bumpversion = subparsers.add_parser("bumpversion")
    parser_bumpversion.add_argument("-n", "--newversion", action="store_true", help="new minor version number")
    parser_bumpversion.set_defaults(func=cmd_bumpversion)

    parser_build = subparsers.add_parser("build")
    parser_build.add_argument("-r", "--rebuild", action="store_true")
    parser_build.add_argument("build", type=str)
    parser_build.set_defaults(func=cmd_build)

    parser_buildinstaller = subparsers.add_parser("buildinstaller")
    parser_buildinstaller.add_argument("-f", "--force", action="store_true")
    parser_buildinstaller.add_argument("-p", "--partial", action="store_true", help="make update incremental/partial install")
    parser_buildinstaller.add_argument("build", type=str)
    parser_buildinstaller.set_defaults(func=cmd_buildinstaller)

    if LC7_RELEASE_ACCESS_KEY_ID is not None and LC7_RELEASE_SECRET_ACCESS_KEY is not None:
        parser_pushplugin = subparsers.add_parser("pushplugin")
        parser_pushplugin.add_argument("name", type=str)
        parser_pushplugin.set_defaults(func=cmd_pushplugin)
    
        parser_pushplugins = subparsers.add_parser("pushplugins")
        parser_pushplugins.set_defaults(func=cmd_pushplugins)

        parser_pushinstaller = subparsers.add_parser("pushinstaller")
        parser_pushinstaller.add_argument("build", type=str)
        parser_pushinstaller.add_argument("releasenotes", type=str)
        parser_pushinstaller.set_defaults(func=cmd_pushinstaller)

        # parser_revert = subparsers.add_parser("revert")
        # parser_revert.add_argument("build", type=str)
        # parser_revert.add_argument("version", type=str)
        # parser_revert.add_argument("releasenotes", type=str)
        # parser_revert.set_defaults(func=cmd_revert)

        parser_updatelinks = subparsers.add_parser("updatelinks")
        parser_updatelinks.set_defaults(func=cmd_updatelinks)

    args = parser.parse_args()

    args.func(args)
