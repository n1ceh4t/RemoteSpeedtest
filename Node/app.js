const express = require('express');
const internal = require('stream');


// Initialize the Express server
const app = express();
app.use(express.static('public'));
app.use(express.json());

  function clean(toTrim) { // removes whitespace from dirty text sent by the agent, then returns as string
    toTrim = toTrim.trim(); // could pass only the string itself to this function rather than the whole array, but it will work
    return toTrim; // return the string
  }

  app.post('/api/hello', async (req, res) => {
    const importantInfo = {}; // init our json object
    var jsons = JSON.parse(JSON.stringify(req.headers)); // convert headers to json object. Maybe some useful things in here? tbd

    var system_info = Buffer.from(jsons["system_info"], 'base64').toString('utf-8'); // decode system_info from string(base64) to string(UTF-8)

    // var username = Buffer.from(jsons["username"], 'base64').toString('utf-8'); //unsure if this is necessary yet, would require manipulation to clean the output

    var external_stats = Buffer.from(jsons["external_network_stats"], 'base64').toString('utf-8'); // decode speedtest results from string(base64) to json string(UTF-8)

    // var internal_stats = Buffer.from(jsons["internal_network_stats"], 'base64').toString('utf-8'); // also a lot of cleanup needed, but could provide insight 

    var current_time = Buffer.from(jsons["current_time"], 'base64').toString('utf-8'); // decode current time (local to the agent) from string(base64) to string(UTF-8)

    const sysinfoArr = system_info.split("\n"); // start handling system info by splitting into an array by newline delimiter

    sysinfoArr.forEach ((val) => { // iterate through each line (array object)
      var newArr = val.split(":") // (e.g. 'Host Name: Desktop-RQWT5',) uses newArr[0] for determining the corresponding key and newArr[1] for it's value. 
      // populate our json object
      if (newArr[0] == 'Host Name') {
        importantInfo["hostname"] = (clean(newArr[1]));
      }
      else if (newArr[0] == 'OS Version' )
      {
        importantInfo["OS"] = (clean(newArr[1]));
      }
      else if (newArr[0] == 'System Boot Time' )
      {
        importantInfo["boot_time"] = (val.replace(newArr[0] + ":", '').trim()); // Cleaning this one would be troublesome if removing all instances of ":".
        //importantInfo.push(clean(array));
      }
      else if (newArr[0] == 'System Manufacturer' )
      {
        importantInfo["manufacturer"] = (clean(newArr[1]));
      }
      else if (newArr[0] == 'System Model' )
      {
        importantInfo["model"] = (clean(newArr[1]));
      }
      else if (newArr[0] == 'System Type' )
      {
        importantInfo["type"] = (clean(newArr[1]));
      }
      else if (newArr[0] == 'Total Physical Memory' )
      {
        importantInfo["totalMemory"] = (clean(newArr[1]));
      }
      else if (newArr[0] == 'Available Physical Memory' )
      {
        importantInfo["availableMemory"] = (clean(newArr[1]));
      }
      else {
        //pass
      }
    });

    importantInfo["current_time"] = current_time; // add the time of the speedtest to our json object
    importantInfo["speedtest"] = JSON.parse(external_stats); // adds speedtest results to our json object 

    
    console.log(JSON.stringify(importantInfo)); // we now have a valid json object with our data. What to do with it?
    res.send("*"); // let the agent know it can exit
  });

// Start the Express server
app.listen(3000, () => {
  console.log('Express server listening on port 3000');
});