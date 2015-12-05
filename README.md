# sysgeep
System Git Keeping, to keep record of and deploy easily your system and user configuration.

For the moment, only Linux systems are supported.
There is no plan to support Windows, although it could change in the future.

Features:
- Git-manage your files in-place
- System configuration files as well as user config files
- Transpose your changes to the updated "maintainer versions" of the config files
- Keep a list of important installed packages for your Linux distribution
- Dependency between config files and distribution's packages (ex: .vimrc depends on vim)
- Allow to take into account known differences between distributions:
  * config files locations
  * different package names to provide a given prerequisite

Language: C++
