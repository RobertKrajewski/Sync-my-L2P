<p align="center">
  <img src="http://syncmyl2p.de/images/light-logo.png"/>
</p>

Sync-my-L2P is a program created by students of the RWTH Aachen University for the comfortable download of all available files in RWTH Moodle (formerly L²P). It allows you to easily mirror all files from the learning rooms to your hard disk, so that they are available without internet.

**Please note: After more than 9 years, I resign as maintainer. However, a team at the RWTH ITC and Moodle will continue to maintain and improve Sync-my-L2P. The new repository will be: https://github.com/rwthmoodle/Sync-my-L2P**

# Download and Installation

You can download Sync-my-L2P either from the [official Website](http://www.syncmyl2p.de) oder [here](https://github.com/Sync-my-L2P/Sync-my-L2P/releases/tag/latest) for Windows, Linux and macOS. While on Windows an installer is provided, on macOS a .dmg or homebrew package, and on Linux an AppImage is distributed.

## Installation on Windows
Just use the provided installer. 

## Installation on Linux Distributions (tested on Ubuntu 18.04)
Make the AppImage executable and open it with a double click.

## Installation on OSX (.dmg)
Open the .dmg and drag&drop Sync-my-L2P to your apps.

## Installation on OSX (Cask)
Users with home-brew installed can now install Sync-my-L2P with the following commands:
`brew tap caskroom/cask` followed by `brew cask install sync-my-l2p`. Updating the program will work with the command `brew update` to update the program formula, followed by `brew upgrade` to upgrade the program itself.

# Compile
For compilation we recommend C++11, Qt (5.11 or higher) and OpenSSL 1.1.1.
The easiest way is to load the .pro file into Qt Creator and compile Sync-my-L2P there. From the console you can also run qmake (with appropriate arguments) instead and then the preferred C++ compiler.

Note: A ClientID is required for the program to connect to the API of RWTH Moodle. The RWTH does not allow the publication of such a client ID. However, an individual ClientID can easily be requested.

# Distribute
For distributing Sync-my-L2P on Linux, two options are available:
1. Create an AppImage using the `run.sh` script provided in the `linux` subdirectory.
2. Create an deb package: https://github.com/justin-time/Sync-my-L2P-Linux

# Crashes? Feedback? Questions?
https://github.com/RobertKrajewski/Sync-my-L2P/issues
