# L0phtCrack 7 #

### Building ###

* Install CMake
  * Download from `https://cmake.org/download/`
  * Install it, ensuring CMake is avaiable on the system path at least for the current user performing the build.

* Install Visual Studio 2017 Community Edition. You can get this for free from Microsoft with the Dev Essentials Program:
  `https://my.visualstudio.com/Downloads?q=visual%20studio%202017&wt.mc_id=o~msft~vscom~older-downloads`

* Install CUDA SDK
  * Download from `https://developer.nvidia.com/cuda-downloads`
  * Run the installer with default settings, or with custom settings, leaving off 'GeForce Experience' if you want to reduce bloat.

* Install Qt 5.12.11
  * Download the online installer from `https://www.qt.io/download-qt-installer`
  * Run the installer and choose the 'Archive' category, use Qt 5.12.11, MSVC 2017 32-bit and 64-bit builds, and include sources for debugging purposes, and Qt Debug Information Files
  * Note the location where it is installed, typically in the folder `C:\Qt`

* Install NSIS
  * Download the NSIS installer (3.08 or newer) from `https://nsis.sourceforge.io/Download`
  * Add the NSIS plugin:
    *  `nsProcess` from `https://nsis.sourceforge.io/NsProcess_plugin`. Get the v1.6 version and put the dlls in your `C:\Program Files (x86)\NSIS\Plugins` folders in the appropriate places, renaming nsProcessW.dll to nsProcess.dll for the unicode version
    *  `ShellExecAsUser` plugin from `https://nsis.sourceforge.io/ShellExecAsUser_plug-in`, get the 'Unicode Update' version and install the same way as `nsProcess`

* Choose a root folder for the repository. Windows often has path length limitations so pick something short, like `C:\` if possible
    ```
    cd /d C:\
    ```
    
* Clone the project recursively
    ```
    git clone --recurse-submodules git@github.com:L0phtCrack/lc7.git
    ```
* Run the setup script, pointing it to the location of Qt (ensure you are using a default `CMD` command prompt with no MSVC environment settings)
    ```
    cd lc7
    setup C:\Qt
    ```
  This will build OpenSSL and configure all of the project dependencies

* Build the 'L0phtCrack Remote Agent' for Windows, by opening the `lc7\win-agent\win-agent.sln` solution file in VS2017 and building all configurations (use batch build, select all to build every combination of the agent)
  * This will likely fail at the 'signtool.exe' build step because code signing certificates cost money and there is no 'LetsEncrypt' for code signing yet, and you don't have the cert/private key for l0phtcrack.
  
* Build the 64-bit 'L0phtCrack 7' project by opening `lc7\build_win64\L0phtCrack 7.sln` and compiling it
* Build the 32-bit 'L0phtCrack 7' project by opening `lc7\build_win32\L0phtCrack 7.sln` and compiling it

### Building the installer
* Build the `RelWithDebInfo` versions of the Windows x86 and x64 builds of L0phtCrack
* Run a VS2017 Native Tools Command Prompt and ensure signtool.exe is in your path.
* Also ensure Python 2.7.14 is in your path
* cd to `tools\releasetool` and run:
  * 'python releasetool.py buildinstaller release64`
  * 'python releasetool.py buildinstaller release32`
* This will likely fail at the 'signtool.exe' build step because code signing certificates cost money and there is no 'LetsEncrypt' for code signing yet, and you don't have the cert/private key for l0phtcrack.

### Documentation ###

To edit and rebuild the L0phtCrack documentation, use the tool 'HelpNDoc' available here: ```https://www.helpndoc.com/download/```
