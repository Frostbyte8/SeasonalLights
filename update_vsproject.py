import sys, os, hashlib
from os import listdir
from os.path import isfile, join

# TODO: x64 support

SCRIPT_PATH     = os.path.dirname(os.path.realpath(__file__)) + "\\"

# Check to see what version the user specified, if any.

if len(sys.argv) < 2:
    print("You must specify which version of visual studio you want to use. Ex: python " + sys.argv[0] + " 2008")
    exit()

IDE_TYPE = sys.argv[1]
sys.path.insert(1, SCRIPT_PATH + "assets\\")
USE_VCX = False

if IDE_TYPE.lower() == "2008":
    import VC2008
    from VC2008 import *
elif IDE_TYPE.lower() == "2015":
    import VC2015
    from VC2015 import *
    USE_VCX = True
else:
    print("Version not support, or an invalid version was given.")
    exit()

# Default Settings (if not otherwise specified)

phash = hashlib.sha256()
phash.update(os.path.realpath(__file__))
phash = phash.hexdigest().upper()

PROJECT_GUID    = "{{{0}-{1}-{2}-{3}-{4}}}".format(phash[0:8], phash[8:12], phash[12:16], phash[16:20], phash[20:32])
ADDITIONAL_LIBS = [ "", "", "" ]
INCLUDE_DIRS    = ""
LIBRARY_DIRS    = ""

# Project Settings

SRC_ROOT        = "src\\"
OUT_PATH        = SCRIPT_PATH + "out\\"
TEMPLATE_PATH   = SCRIPT_PATH + "assets\\"
PROJECT_NAME    = "ProjectName"

# Override above from a usersettings.py outside of the source tree

origSettingValue = sys.dont_write_bytecode
sys.dont_write_bytecode = True
sys.path.insert(1, SCRIPT_PATH + "..")
import usersettings
from usersettings import *

sys.dont_write_bytecode = origSettingValue


# TODO: A way to detect what version is being used

TAG_LIST = {
"{{MSVC_VERSION}}" : MSVC_VERSION_NUMBER,
"{{MSVC_FORMAT_VERSION}}" : MSVC_FORMAT_VERSION,
"{{MSVC_IDE_VERSION}}" : MSVC_IDE_VERSION,
"{{PROJECT_GUID}}" : PROJECT_GUID,
"{{PROJECT_NAME}}" : PROJECT_NAME,
"{{STUDIO_VERSION}}" : STUDIO_VERSION,
"{{MINIMUM_STUDIO_VERSION}}" : MINIMUM_STUDIO_VERSION,
}

PROJECT_TAG_LIST = {
"{{MSVC_VERSION}}" : MSVC_VERSION_NUMBER,
"{{PROJECT_GUID}}" : PROJECT_GUID,
"{{PROJECT_NAME}}" : PROJECT_NAME,
"{{INCLUDE_DIRS}}" : INCLUDE_DIRS,
"{{LIBRARY_DIRS}}" : LIBRARY_DIRS,
"{{ADDITIONAL_LIBS}}" : ADDITIONAL_LIBS,
"{{FILES_TO_ADD}}" : "",
"{{CONFIGURATIONS}}" : "",
}

SOURCES_ROOT    = [ "Source Files", "", [".cpp", ".h"]]
SOURCES_MODEL   = [ "Model", "model\\", [".cpp", ".h"]]

SOURCES_ALL = [ SOURCES_ROOT] ##, SOURCES_MODEL ]

# TODO: The stuff below works fine for vcproj, but not vcxproj

def addFiles(isVCX):

    outString = ""
    
    for filter in SOURCES_ALL:
 
        files = ""
        curPath = SCRIPT_PATH + SRC_ROOT + filter[1]
        fileList = [file for file in listdir(curPath) if isfile(join(curPath, file))]

        for fileName in fileList:
            for exts in filter[2]:
                if fileName.endswith(exts):
                    if(isVCX):
                        #TODO: Relative Path?
                        if(fileName.endswith(".cpp") or fileName.endswith(".c")):
                            files += '    <ClCompile Include="..\\' + SRC_ROOT + filter[1] + fileName + '" />\n'
                        else:
                            files += '    <ClInclude Include="..\\' + SRC_ROOT + filter[1] + fileName + '" />\n'
                    else:
                        files += '\t\t\t<File RelativePath="..\\' + SRC_ROOT + filter[1] + fileName + '"></File>\n'
        
        if files != "":

            if isVCX:
                outString += files
            else:
                outString += '\t<Filter Name="' + filter[0] + '">\n'
                outString += files
                outString += '\t\t</Filter>'

    return outString


def addConfigurations():

    outString = ""

    for config in PROJECT_CONFIGURATIONS:

        outString += "<Configuration\n"
        configProps = config[0]
        for key, value in configProps.items():
            outString += "\t\t\t" + key + '="' + value + '"\n'
        outString += "\t\t\t>\n"

        for compTools in config[1]:
            outString += "\t\t\t<Tool\n"
            for key, value in compTools.items():
                outString += "\t\t\t\t" + key + '="' + value + '"\n'
            outString += "\t\t\t\t/>\n"

        outString += "\t\t</Configuration>\n\t\t"

    return outString

def replaceTags(line, tagList, isVCX = False):

    for key, value in tagList.iteritems():
        matchFound = line.find(key)
        if(matchFound != -1):
            if type(value) is list:


                if(key.find("_DIRS") != -1):

                    dirList = ""
                    
                    for idx, item in enumerate(value):

                        if(idx != 0):
                            dirList += ";"

                        dirList += "&quot;" + item + "&quot;"


                    line = line.replace(key, dirList)

                else:

                    fileList = ""
                    for item in value:
                        fileList += item + " "
                    
                    line = line.replace(key, fileList)


            else:
                if (key.find("{{FILES_TO_ADD}}") != -1):
                    line = line.replace(key, addFiles(isVCX))
                elif (key.find("{{CONFIGURATIONS}}") != -1):
                    line = line.replace(key, addConfigurations())
                    line = replaceTags(line, tagList, isVCX)
                else:
                    line = line.replace(key, value)

    return line

#TODO: Proper VCX support 

def createNewProject():

    projFile = TEMPLATE_PATH  + "project_base"
    ext = ""
    
    if(USE_VCX):
        projFile = TEMPLATE_PATH + "2015"
        ext += ".vcxproj"
    else:
        ext = ".vcproj"

    projFile += ext

    if(os.path.exists(projFile) == False):
        print(projFile + " not found.")
        return

    with open(projFile, "r") as ioFile:
        lines = ioFile.readlines()

    ioFile.close()

    with open(OUT_PATH + PROJECT_NAME + ext, "w") as ioFile:
        for line in lines:
            line = replaceTags(line, PROJECT_TAG_LIST, USE_VCX)
            ioFile.write(line)

    ioFile.close()
    return


def createNewSolution():

    if(USE_VCX):
        slnFile = TEMPLATE_PATH + "2015.sln"
    else:
        slnFile = TEMPLATE_PATH + "solution_base.sln"

    if(os.path.exists(slnFile) == False):
        print("solution_base.sln not found.")
        return

    with open(slnFile, "r") as ioFile:
        lines = ioFile.readlines()

    ioFile.close()

    with open(OUT_PATH + PROJECT_NAME + ".sln", "w") as ioFile:

        for line in lines:
            line = replaceTags(line, TAG_LIST, USE_VCX)
            ioFile.write(line)

    ioFile.close()
    return

createNewProject()
createNewSolution()
