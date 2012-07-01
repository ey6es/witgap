Witgap
------

An "ASCII sandbox MMO" with a Flash (ActionScript) client and a C++ server.


Building
--------

Dependencies are cmake (http://www.cmake.org/, v2.8+), Qt (http://qt.nokia.com/products/, v4.6.2+),
OpenSSL (http://www.openssl.org/), MaxMind GeoIP (http://www.maxmind.com/app/c/), the Adobe Flex
SDK (http://www.adobe.com/devnet/flex/flex-sdk-download.html, v4.6+), and doxygen
(http://www.stack.nl/~dimitri/doxygen/) for code documentation.

To build, create a build folder and run "cmake path-to-code-folder" (e.g., create a "build" folder
at the top level, cd into it, and run "cmake ..").  This creates a native build structure that can
then be built using "make" (or equivalent).

To generate the code documentation, run "doxygen" at the top level.


Running
-------

To run the server, copy etc/server.ini.dist to etc/server.ini and adjust configuration as
necessary.  Then run "./bin/witgap-server etc/server.ini".


Distribution
------------

All code is proprietary.  Please do not share without permission of the author.


Contact Information
-------------------

Send questions/comments to Andrzej Kapolka <drzej.k@gmail.com>.
