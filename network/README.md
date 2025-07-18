# Xfce4 Network Plugin
DHCPCD based Network plugin for Xfce4 panel (Third party).  
It is ported from lxplug-network for Raspberry Pi OS.  

## Build
install these build dependencies...  
* libxfce4panel-2.0-dev  
* libgtk-3-dev  

Open terminal and change directory to src directory.  
Run this command...  
`make -j4`  

### Install and Uninstall
To install run...  
`sudo make install`  

To uninstall run...  
`sudo make uninstall`  

After installing go to panel settings and add this plugin to your panel.  
