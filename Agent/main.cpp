#include <iostream>

#include "includes/HTTPRequest.hpp"

#include <string>

#include <stdio.h>

#include <cstring>

#include <filesystem>

#include "includes/base64.hpp"

#include <fstream>

#if WINDOWS
#define popen _popen
#endif

std::string host = "http://127.0.0.1:3000"; // address of the middleware. Domain names supported
std::string speedtest_url = "http://127.0.0.1:3000/speedtest.exe"; // address of the speedtest executable (todo, may trip antivirus. Look into bundling executable?)

std::string get_speedtest(std::string CWD) {
  http::Request request(speedtest_url); // build request

  const http::Response response = request.send("GET");

  std::string resp = std::string(response.body.begin(), response.body.end()); // store the response as a string to be returned
  std::ofstream outfile;      // open file for writing
  outfile.open(CWD + "/speedtest.exe", std::ios::binary); // need to set the binary type or windows will add extra bytes

  if (outfile) 
  {
      outfile << resp; // output to binary (hopefully? << operators are supposedly not meant for this.)
  }

  outfile.close();   
  return "Nice, dude.";
}

int check_speedtest(std::string CWD) {
  if (std::filesystem::exists(CWD + "/speedtest.exe") == true)
  {
    return 1;
  }
  else
  {
    return 0;
  }
}


int HELLO_INTERVAL = 10; // how often to gather machine info and run a speedtest
int IDLE_TIME = 720; // after exceeding MAX_FAILED_CONNECTIONS, how long to wait before trying again
int FAILED_CONNECTIONS = 0; // for counting concurrent failed connections (to either the middleware or the speed test server)
int MAX_FAILED_CONNECTIONS = 10; // maximum connection attempts before switching to IDLE mode

std::string server_hello(std::string SYSTEMINFO, std::string EXTERNAL_NETWORK_STATS, std::string CURRENT_TIME) {
  
  std::string url = host + "/api/hello"; // build the URL at which to POST data
  http::Request request(url); // build request
  // try:catch for errors. Returns error and increments FAILED_CONNECTIONS on error (connection refused)
  try {
    const http::Response response = request.send("POST", "*", { // send request. syntax: request.send(<method>, <body data>, <headers>)
      "Content-Type: application/x-www-form-urlencoded",       // sending the data as headers so it can easily be parsed into JSON on the backend (without doing too much here)
      "system_info: " + SYSTEMINFO,
      "external_network_stats: " + EXTERNAL_NETWORK_STATS,
      "current_time: " + CURRENT_TIME
      });
      std::string resp = std::string(response.body.begin(), response.body.end()); // store the response as a string to be returned
      std::cout << resp;
      return resp;
  } 
  catch (std::system_error er)
  {
    return "error";
  }
}

const std::string runcmd(std::string cmd) {

  std::array < char, 4096 > buffer; // ensure we have a large enough buffer to store the command's output
  std::string result; //
  auto pipe = popen(cmd.c_str(), "r"); // get rid of shared_ptr

  if (!pipe) throw std::runtime_error("popen() failed!");

  while (!feof(pipe)) {
    if (fgets(buffer.data(), buffer.size(), pipe) != nullptr) //check if null
      result += buffer.data(); // add piped buffer to result 
  }

  auto rc = pclose(pipe); // close the pipe

  if (rc == EXIT_SUCCESS) { // == 0

  } else if (rc == EXIT_FAILURE) { // EXIT_FAILURE is not used by all programs, maybe needs some adaptation.
    // pass
  }
  return std::string {
    result
  };

}

std::string speedtest(std::string CWD) {
  // powershell echo YES | speedtest.exe -f json // working auto-accept EULA for speedtest, working with json output for external network statistics. tested with windows 10.
  std::string EXTERNAL_NETWORK_STATS = runcmd("powershell cd " + CWD + " && echo yes | speedtest.exe -f json"); // need to change to current working directory inside of powershell, then execute speedtest.exe
  std::string err = "error"; // for error checking
  if (EXTERNAL_NETWORK_STATS.find(err) != std::string::npos) { // check if output of speedtest.exe includes "error"
    return "error"; // if so, return precise error string
  } else {
    return EXTERNAL_NETWORK_STATS; // else, it must have succeeded! Return network statistics.
  }
}

int main(int argc,
  const char * argv[]) {

  std::string CWD = std::filesystem::current_path().generic_string(); // casts std::filesystem::path to a string, this is prefixed and will be modified below
  
  if (check_speedtest(CWD) == 0) 
  {
    get_speedtest(CWD);
  } 
  else
  {
    //pass
  }

  while (1 == 1) {

    // here we gather info inside the loop, otherwise we will continue sending the same initial data
    // we call powershell in the commands because why not? Except for speedtest where it is needed to bypass eula on first run (mildly annoying, requires testing on other windows builds but works here)
    // std::string INTERNAL_NETWORK_STATS = runcmd("net statistics workstation"); // for internal network statistics. TBD, but this is extendable.
    std::string SYSTEMINFO = std::string(base64::to_base64(runcmd("powershell systeminfo"))); // returns dirty system info, encoded into base64 then cast to a string to be decoded then parsed on the server/middleware
    std::string CURRENT_TIME = std::string(base64::to_base64(runcmd("powershell get-date -format \"{dd-MMM-yyyy HH:mm}\""))); // same as above
    std::string EXTERNAL_NETWORK_STATS = speedtest(CWD); // we change this to a base64 encoded string before sending the request, but after error handling (for retries, speedtest-cli is finnicky)

    
    while (EXTERNAL_NETWORK_STATS == "error") { // error handling for speedtest, if speedtest does not return an error, we skip this.
      FAILED_CONNECTIONS++; // maybe should be a seperate counter for failed speedtest runs vs failed to post data to server, but this is fine for now.
      Sleep(HELLO_INTERVAL); // utilize this to allow some time if speedtest fails (unknown impact)
      EXTERNAL_NETWORK_STATS = speedtest(CWD); // we try again

      if (EXTERNAL_NETWORK_STATS != "error") { // if speedtest() does not return an error, we can break the loop and send the request. 
        break;
      } else { // if we get another error, we keep looping until it is successful.
        continue;
      }
    }

    Sleep(HELLO_INTERVAL); // now that we have valid data to send, we wait the minimum time before continuing.

    if (FAILED_CONNECTIONS > MAX_FAILED_CONNECTIONS) { // if it has failed more than the allowed number of times...
      Sleep(IDLE_TIME); // we sleep for the idle time, no need to try for awhile. Something is wrong.
    }

    std::string hel = server_hello(SYSTEMINFO, std::string(base64::to_base64(EXTERNAL_NETWORK_STATS)),CURRENT_TIME); // speedtest output has been validated, we can now encode/cast it while passing to server_hello()
    // check the server's response
    if (hel == "*") { // default response from the server, success!
      FAILED_CONNECTIONS = 0; // I don't suppose we really need this...
      exit(0); // if this is a scheduled task, we can exit here! No need to run all of the time.
    } else { // we know something's wrong (probably.)
      FAILED_CONNECTIONS++; // failed connection? restart the loop.
    }
  }
}