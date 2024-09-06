# RemoteSpeedtest
Client/Server utilizing speedtest-cli to check network and machine statistics on remote workstations coded in C++ and JavaScript. Returns data as a json string.

uses: 
  + https://github.com/elnormous/HTTPRequest (for http requests)
  + https://github.com/tobiaslocker/base64 (for encoding strings)

usage:

  Run the server, drop speedtest.exe in the public folder in Node, or in the same directory as the compiled agent. 
  If speedtest.exe is not in the same directory as the agent, it will be downloaded from the server if available.
  
  if the speedtest fails it will retry until successful, at incremented intervals that you can set in the source.
  You can edit the server addresses in main.cpp for the agent as well.
  
  The code itself is heavily commented, so everything should be pretty self-explanatory.
  This software comes with no guarantees.

  Compiled using x86_64-w64-mingw32-g++ (from cygwin)
  
    C:/>x86_64-w64-mingw32-g++ main.cpp -static -o main.exe -lwsock32 -lWs2_32
  
  
  # USE AT YOUR OWN RISK.
  
