# RemoteSpeedtest
Client/Server utilizing speedtest-cli to check network and machine statistics on remote workstations. Returns data as a json string.
# USE AT YOUR OWN RISK.
uses: 
  + https://github.com/elnormous/HTTPRequest (for http requests)
  + https://github.com/tobiaslocker/base64 (for encoding strings)

usage:
  Run the server, drop speedtest.exe in the public folder in Node, or in the same directory as the compiled agent. 
  If speedtest.exe is not in the same directory as the agent, it will be downloaded if available.
  if the speedtest fails it will retry until successful.
  You can edit the addresses in main.cpp for the agent.
  I recommend using cygwin's g++ compiler, as in main.cpp there are workarounds for how cygwin handles paths.
  The code itself is heavily commented, so everything should be pretty self-explanatory.
  This software comes with no guarantees.

  Compiled using x86_64-w64-mingw32-g++ (from cygwin)
    C:/>x86_64-w64-mingw32-g++ main.cpp -static -o main.exe -lwsock32 -lWs2_32
  
  
  # USE AT YOUR OWN RISK.
  
