ConfigConstants = {

    #   General Settings

    "Charset" : {
        "Unicode"   : "0",
        "Multibyte" : "1",
    },

    "ConfigurationType" : {
        "EXE"       : "1",
        "DLL"       : "2",
    },
    
    "WholeProgramOptimization" : {
        "None"          : "0",
        "TimeLink"      : "1",
        "Instrument"    : "2",
        "Optimize"      : "3",
        "Update"        : "4",
    },

    #   C/C++ Settings
    ##  General

    "DebugFormat" : {
        "Disable"           : "0",
        "C7_Compatible"     : "1",
        "ProgramDatabase"   : "3",
        "EditAndContinue"   : "4",
    },

    "WarningLevel" : {
        "Off"               : "0",
        "Level1"            : "1",
        "Level2"            : "2",
        "Level3"            : "3",
        "Level4"            : "4",
    },

    ##  Optimization

    "Optimization" : {
        "Disabled"          : "0",
        "MinimumSize"       : "1",
        "MaxSpeed"          : "2",
        "Full"              : "3",
    },

    "UseIntrinsic" : {
        "Yes"               : "true",
        "No"                : "",
    },

    ## Code Generation

    "MinimalRebuild" : {
        "Yes"               : "true",
        "No"                : "",
    },

    "RuntimeChecks" : {
        "StackFrames"       : "1",
        "Variables"         : "2",
        "Both"              : "3",
    },
    
    "RuntimeLibrary" : {
        "Multithreaded"             : "1",
        "Multithreaded_Debug"       : "2",
        "Multithreaded_DLL"         : "3",
        "Multithreaded_DLL_Debug"   : "4",
    },
    
    "UseFunctionLevelLinking" : {
        "No"                        : "",            
        "Yes"                       : "true",
    },

    # Linker

    ## Debugging

    "GenerateDebugInfo" : {
        "No"                        : "",
        "Yes"                       : "true",
    },

    ## Optimization

    "OptimizeReferences" : {
        "Keep"                      : "1",
        "Eliminate"                 : "2",
    },

    "COMDATFolding" : {
        "DontRemove"            : "1",
        "Remove"                : "2",
    },

    ## Advanced

    "TargetMachine" : {
        "MachineX86"                : "1",
        "MachineX64"                : "2",
    },
}

# VS2015 Settings

MSVC_VERSION_NUMBER     = "14.0.25420.1"
MSVC_FORMAT_VERSION     = "12.00"
MSVC_IDE_VERSION        = "14"

# SLNs in 2008 didn't support these

STUDIO_VERSION          = "14.0.25420.1"
MINIMUM_STUDIO_VERSION  = "10.0.40219.1"


# These are the default settings that are normally set when making a new empty project.
# Debug

DEBUG_CONFIG_PROPERTIES = {

        'Name'                              : "Debug|Win32",
        'OutputDirectory'                   : "$(SolutionDir)$(ConfigurationName)",
        'IntermediateDirectory'             : "$(ConfigurationName)",

        'ConfigurationType'                 : ConfigConstants['ConfigurationType']['EXE'], 
        'CharacterSet'                      : ConfigConstants['Charset']['Unicode'],
}

DEBUG_COMPILER_TOOL = {

        'Name'                              : "VCCLCompilerTool",
        'AdditionalIncludeDirectories'      : "{{INCLUDE_DIRS}}",

        'Optimization'                      : ConfigConstants['Optimization']['Disabled'],
        'MinimalRebuild'                    : ConfigConstants['MinimalRebuild']['Yes'],
        'BasicRuntimeChecks'                : ConfigConstants['RuntimeChecks']['Both'],
        'RuntimeLibrary'                    : ConfigConstants['RuntimeLibrary']['Multithreaded_DLL_Debug'],
        'WarningLevel'                      : ConfigConstants['WarningLevel']['Level3'],
        'DebugInformationFormat'            : ConfigConstants['DebugFormat']['EditAndContinue'],
}

DEBUG_LINKER_TOOL = {
        'Name'                              : "VCLinkerTool", 
        'AdditionalDependencies'            : "{{ADDITIONAL_LIBS}}",
        'AdditionalLibraryDirectories'      : "{{LIBRARY_DIRS}}",

        'TargetMachine'                     : ConfigConstants['TargetMachine']['MachineX86'],
}

# Release Build

RELEASE_CONFIG_PROPERTIES = {

        'Name'                              : "Release|Win32",
        'OutputDirectory'                   : "$(SolutionDir)$(ConfigurationName)",
        'IntermediateDirectory'             : "$(ConfigurationName)",

        'ConfigurationType'                 : ConfigConstants['ConfigurationType']['EXE'], 
        'CharacterSet'                      : ConfigConstants['Charset']['Unicode'],
        'WholeProgramOptimization'          : ConfigConstants['WholeProgramOptimization']['TimeLink'],
}

RELEASE_COMPILER_TOOL = {

        'Name'                              : "VCCLCompilerTool",
        'AdditionalIncludeDirectories'      : "{{INCLUDE_DIRS}}",

        'Optimization'                      : ConfigConstants['Optimization']['MaxSpeed'],
        'EnableIntrinsicFunctions'          : ConfigConstants['UseIntrinsic']['Yes'],
        'RuntimeLibrary'                    : ConfigConstants['RuntimeLibrary']['Multithreaded_DLL'],
        'EnableFunctionLevelLinking'        : ConfigConstants['UseFunctionLevelLinking']['Yes'],
        'WarningLevel'                      : ConfigConstants['WarningLevel']['Level3'],
        'DebugInformationFormat'            : ConfigConstants['DebugFormat']['EditAndContinue'],
}

RELEASE_LINKER_TOOL = {
        'Name'                              : "VCLinkerTool", 
        'AdditionalDependencies'            : "{{ADDITIONAL_LIBS}}",
        'AdditionalLibraryDirectories'      : "{{LIBRARY_DIRS}}",

        'OptimizeReferences'                : ConfigConstants['OptimizeReferences']['Eliminate'],
        'EnableCOMDATFolding'               : ConfigConstants['COMDATFolding']['Remove'],
        'GenerateDebugInfo'                 : ConfigConstants['GenerateDebugInfo']['Yes'],
        'TargetMachine'                     : ConfigConstants['TargetMachine']['MachineX86'],
}

# Now compile the dictionaries into a list

DEBUG_TOOL_LIST     = [ DEBUG_COMPILER_TOOL, DEBUG_LINKER_TOOL ]
RELEASE_TOOL_LIST   = [ RELEASE_COMPILER_TOOL, RELEASE_LINKER_TOOL ]

PROJECT_CONFIGURATIONS = [
        [DEBUG_CONFIG_PROPERTIES, DEBUG_TOOL_LIST],
        [RELEASE_CONFIG_PROPERTIES, RELEASE_TOOL_LIST],
]
                            

# for i in RELEASE_TOOL_LIST:
# 
#     print("\n")
#     print(i['Name'])
#     for k,v in i.items():
#         if(k != "Name"):
#             print(k + "\t = " + v)
