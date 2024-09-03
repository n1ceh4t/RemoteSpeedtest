const express = require('express');


// Initialize the Express server
const app = express();
const { check, validationResult } = require('express-validator');
app.use(express.static('public'));
app.use(express.json());
const safeChars = /^[A-Za-z :0-9_.]+$/;
const unsafeChars = /[A-Za-z :0-9_.]+$/;

const newJSON = {}

function clean(obj) {
  return Object.keys(obj).reduce((acc, key) => {
    const value = obj[key];
    if (typeof value === 'object' && !Array.isArray(value)) { // handle nested objects
      acc[key] = cleanObject(value);
    } else if (typeof value === 'string') {
      acc[key] = cleanString(value);
    } else {
      acc[key] = value;
    }
    return acc;
  }, {});
}

function cleanObject(obj) {
  const cleanedObj = {};
  for (const key in obj) {
    if (obj[key].type && obj[key].type === 'result') { // skip the root object
      continue;
    }
    let value = obj[key];
    if (typeof value === 'string') {
      value = cleanString(value);
    } else if (typeof value === 'object' && !Array.isArray(value)) {
      value = cleanObject(value);
    }
    cleanedObj[key] = value;
  }
  return cleanedObj;
}

function cleanString(str) {
  let newString = '';
  for (let i = 0; i < str.length; i++) {
    if (str.charAt(i).match(safeChars)) {
      newString += str.charAt(i);
    } else {
      newString += '-';
    }
  }
  if (check(str).notEmpty())
  {
    return newString.trim();
  }
  else
  {
    return "error.";
  }
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
        importantInfo["hostname"] = (cleanString(newArr[1]));
      }
      else if (newArr[0] == 'OS Version' )
      {
        importantInfo["OS"] = (cleanString(newArr[1]));
      }
      else if (newArr[0] == 'System Boot Time' )
      {
        importantInfo["boot_time"] = (cleanString(val.replace(newArr[0] + ":", '').trim())); // Cleaning this one would be troublesome if removing all instances of ":".
        //importantInfo.push(clean(array));
      }
      else if (newArr[0] == 'System Manufacturer' )
      {
        importantInfo["manufacturer"] = (cleanString(newArr[1]));
      }
      else if (newArr[0] == 'System Model' )
      {
        importantInfo["model"] = (cleanString(newArr[1]));
      }
      else if (newArr[0] == 'System Type' )
      {
        importantInfo["type"] = (cleanString(newArr[1]));
      }
      else if (newArr[0] == 'Total Physical Memory' )
      {
        importantInfo["totalMemory"] = (cleanString(newArr[1]));
      }
      else if (newArr[0] == 'Available Physical Memory' )
      {
        importantInfo["availableMemory"] = (cleanString(newArr[1]));
      }
      else {
        //pass
      }
    });

    importantInfo["current_time"] = cleanString(current_time); // add the time of the speedtest to our json object

    //broken
    const speedtestJSON = JSON.parse(external_stats)

    importantInfo["speedtest"] = clean(speedtestJSON); // adds speedtest results to our json object 

    
    console.log(JSON.stringify(importantInfo)); // we now have a valid json object with our data. What to do with it?
    res.send("*"); // let the agent know it can exit
  });

// Start the Express server
app.listen(3000, () => {
  console.log('Express server listening on port 3000');
});