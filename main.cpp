//-------------------------------------------------------------------------------------------//
//                                    Remote-Speedtest                                       //
//                                   TODO: auto-updater,                                     //
//                      We can also determine DNS failure with some checks(?)                //
//                      Additionally, use this to collect user feedback(?)                   //
//-------------------------------------------------------------------------------------------//

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

std::string speedtestMD5 = "d6d252bb777a18167d8e6183c0d0eefb"; // MD5 of current (windows build) speedtest-cli
std::string CWD = std::filesystem::current_path().generic_string(); // casts std::filesystem::path (executable path) to a string
std::string host = "http://127.0.0.1:3000"; // address of the middleware. Domain names supported (we should add an IP fallback in case of DNS failure, which should be included in the report.)
std::string speedtest_url = "http://127.0.0.1:3000/speedtest.exe"; // address of the speedtest executable (todo, may trip antivirus. Look into bundling executable?)


// Sleep times are in MS
int HELLO_INTERVAL = 100000; // how often to gather machine info and run a speedtest
int IDLE_TIME = 720000; // after exceeding MAX_FAILED_CONNECTIONS, how long to wait before trying again
int FAILED_CONNECTIONS = 0; // for counting concurrent failed connections (to either the middleware or the speed test server)
int MAX_FAILED_CONNECTIONS = 10; // maximum connection attempts before switching to IDLE (Sleep(IDLE_TIME))

const std::string runcmd(std::string cmd) { // let's get dangerous!

  try
  {
    std::array < char, 4096 > buffer; // ensure we have a large enough buffer to store the command's output
    std::string result; // where to store output
    auto pipe = popen(cmd.c_str(), "r"); // get rid of shared_ptr

    if (!pipe) throw std::runtime_error("popen() failed!");

    while (!feof(pipe)) { // makes sure EOF has not reached, stops at EOF
      if (fgets(buffer.data(), buffer.size(), pipe) != nullptr) // check if buffer data from pipe is not null then add data to output var
      {
        result += buffer.data(); // add piped buffer to result 
      }
    }

    auto rc = pclose(pipe); // close the pipe at EOF

    return std::string { // return the output as a string
      result
    };
  }
  catch (std::runtime_error er)
  {
    // logText(er.what()); // log that (*TODO*)
    // if we can't run commands on the agent, we should report that to the server if possible                                                                     *TODO*
    return "error"; 
  }

  return "error"; // obligatory; will not compile without this
}

void logText(std::string logText)
{
  std::ofstream outfile;

  if (std::filesystem::exists(CWD + "/log.txt") == true) // if logfile already exists...
  { 
    outfile.open(CWD + "/log.txt", std::ios_base::app); // append existing file
  }
  else
  {
    outfile.open(CWD + "/log.txt", std::ios_base::out); // open for write
  }
  outfile << runcmd("powershell get-date -format \"{dd-MMM-yyyy HH:mm}\"") + " [ERROR] : " + logText + "\n"; // insert log text; inefficient timestamp but does not require additional libraries
  outfile.close(); // close the file

  return;
}

std::string server_hello(std::string SYSTEMINFO, std::string EXTERNAL_NETWORK_STATS, std::string CURRENT_TIME) {
  
  std::string url = host + "/api/hello"; // build the URL at which to POST data
  http::Request request(url); // build request
  // try:catch for errors. Returns error and increments FAILED_CONNECTIONS on error (connection refused)
  try {
    const http::Response response = request.send("POST", "*", { // send request. syntax: request.send(<method>, <body data>, <headers>)
      "Content-Type: application/x-www-form-urlencoded",       // sending the data as headers so it can easily be parsed into JSON on the backend (without doing too much here)
      "system_info: " + SYSTEMINFO, // "systeminfo" command output (base64 encoded for server-side manipulation and ease of transport)
      "external_network_stats: " + EXTERNAL_NETWORK_STATS, // speedtest.exe output (base64-encoded json)
      "current_time: " + CURRENT_TIME // speedtest timestamp is not necessarily local time; this is the current system time (expect ssl errors or bad CMOS? base64 encoded)
    });
    std::string resp = std::string(response.body.begin(), response.body.end()); // store the response as a string to be returned
    return resp;
  } 
  catch (std::system_error er) // catches error (cannot connect to server / connection refused, etc. without this, application crashes on error)
  {
    logText(er.what()); // log that
    return "error"; // tells calling function to retry
  }
}






std::string get_speedtest() {
  if (std::filesystem::exists(CWD + "/speedtest.exe") == true) // if speedtest.exe already exists...
  { 
    logText("Speedtest exists, but is invalid!");
    // attempt to notify server                                                                                                                                   *TODO*
    runcmd("cd " + CWD + " && del \"speedtest.exe\""); // delete it, it is invalid
  }
  else
  {
    logText("Downloading speedtest (first run?)");
  }
  logText("Downloading speedtest.");
  // we download the binary from the server
  http::Request request(speedtest_url); // build request

  const http::Response response = request.send("GET"); //...send GET request
  std::string resp = std::string(response.body.begin(), response.body.end()); // store the response as a string to be written to binary (this might be bad) *TODO*
  std::ofstream outfile; // open file for writing
  outfile.open(CWD + "/speedtest.exe", std::ios::binary); // need to set the binary type or windows will add extra bytes. 
  if (outfile) // opening new file
  {
      outfile << resp; // output to binary (hopefully? << operators are supposedly not meant for this but I haven't experienced issues.)
  }
  else // unable to open new file. Invalid permissions?
  {
    logText("Unable to open speedtest.exe for writing! Bad permissions?"); // log that
    return "error"; // return error
  }

  outfile.close(); // close the new file   
  return "Nice, dude."; // Thanks man
}

int check_speedtest() { // checks if speedtest.exe is in current directory
  if (std::filesystem::exists(CWD + "/speedtest.exe") == true) // check if speedtest exists
  {
    std::string hashCheck = runcmd("powershell cd " + CWD + "&& certutil -hashfile speedtest.exe MD5"); // built-in for checking MD5. not very efficient...
    std::size_t found = hashCheck.find(speedtestMD5); // but it works
    if (found != std::string::npos) { // check for valid md5 until end of output string (this is bad, but not too bad...)
      // hacky MD5 Comparison
      return 1; // return and continue
    }
    else
    {
      logText("Speedtest.exe MD5 verification failed!"); // log that
      // notify dashboard?                                                                                                                                      *TODO*
      // if MD5 comparison failed, download speedtest again
      return 0; // download again
    }
  }
  else // if speedtest.exe is not found in current path
  {
    logText("Speedtest.exe not found in current path!");
    return 0; //download again
  }
  logText("Something is very wrong in check_speedtest()");
  return 0; // return is required outside of if:else logic
}

std::string speedtest() { // running the speedtest
  std::string EXTERNAL_NETWORK_STATS = runcmd("powershell cd " + CWD + " && speedtest.exe -f json --accept-license"); // need to change to current working directory inside of powershell, then execute speedtest.exe and accept EULA 
  std::string err = "error"; // for error checking
  if (EXTERNAL_NETWORK_STATS.find(err) != std::string::npos) { // check if output of speedtest.exe includes "error" (This does not work? EOF not reached?)
    logText(EXTERNAL_NETWORK_STATS); // log that
    return err; // if so, return generic error string
  } else {// else, it must have succeeded! Return network statistics.
    return EXTERNAL_NETWORK_STATS; 
  }
}

int main(int argc,
  const char * argv[]) {
  
  while (check_speedtest() == 0) // check if speedtest.exe is in current directory and matches MD5
  {
    logText("Speedtest not in directory or MD5 mismatch!");
    get_speedtest(); //if not, retrieve it from the server and revalidate
  }

  while (1 == 1) {

    // here we gather info inside the loop, otherwise we will continue sending the same initial data
    // we call powershell in the commands because why not? Might circumvent AV false-positives.
    // std::string INTERNAL_NETWORK_STATS = runcmd("powershell net statistics workstation"); // for internal network statistics(?) TBD, but this is extendable.
    std::string SYSTEMINFO = std::string(base64::to_base64(runcmd("powershell systeminfo"))); // returns dirty system info, encoded into base64 then cast to a string to be decoded then parsed on the server/middleware (not great!)
    std::string CURRENT_TIME = std::string(base64::to_base64(runcmd("powershell get-date -format \"{dd-MMM-yyyy HH:mm}\""))); //same as above, but for system date/time
    std::string EXTERNAL_NETWORK_STATS = speedtest(); // we change this to a base64 encoded string before sending the request, but after error handling (for retries; speedtest-cli is finnicky sometimes and we need reliable data)

    while (EXTERNAL_NETWORK_STATS == "error") { // error handling for speedtest, if speedtest does not return an error, we skip this. (This condition is never met. Why?)
      logText(EXTERNAL_NETWORK_STATS); // log that
      if (FAILED_CONNECTIONS <= MAX_FAILED_CONNECTIONS) // Sleep for default time
      {
        Sleep(HELLO_INTERVAL); // utilize this to allow some time if speedtest fails (unknown impact, probably speedtest chooses a different server)
      }
      else // connection to server failed more than the allowed number of times. This can tell us things. Go into idle
      {
        Sleep(IDLE_TIME); // Sleep longer
      }

      EXTERNAL_NETWORK_STATS = speedtest(); // we try again

      if (EXTERNAL_NETWORK_STATS != "error") { // if speedtest() does not return an error, we can break the loop and send the request.
        break; // break the loop and send the request
      } else { // if we get another error, we keep looping until it is successful.
        logText(EXTERNAL_NETWORK_STATS);
        continue;
      }
    }

    if (FAILED_CONNECTIONS > MAX_FAILED_CONNECTIONS) { // if it has failed more than the allowed number of times...
      Sleep(IDLE_TIME); // we sleep for the idle time, no need to try for awhile. Something is wrong.
    }
    else
    {
      Sleep(HELLO_INTERVAL); // now that we have valid data from speedtest to send, we wait the minimum time before continuing.
    }

    EXTERNAL_NETWORK_STATS = std::string(base64::to_base64(EXTERNAL_NETWORK_STATS)); // speedtest output has been validated, we can now encode->cast it 
    std::string hel = server_hello(SYSTEMINFO, EXTERNAL_NETWORK_STATS, CURRENT_TIME); // pass collected data to server_hello() and return response as string
    
    // check the server's response
    if (hel == "*") { // valid response from the server, success!
      FAILED_CONNECTIONS = 0; // reset FAILED_CONNECTIONS
      exit(0); // We can exit here! No need to run all of the time. Only if there are issues or it is scheduled. Lack of data sent to server means something is likely amiss.
    } else { // we know speedtest was successful, but connection to remote server has failed.
      logText("Connection to speedtest may have been successful, but connection to remote server failed."); // log that
      FAILED_CONNECTIONS++; // failed connection? restart the loop.
    }
  }
}