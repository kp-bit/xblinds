#include <pgmspace.h>
// This is the web interface, stored in a const - ideally the html would be read into the const at compile-time
// I make changes in the actual html file as html is a lot easier to handle in VSCode, and paste the entire file in here
// Tried using 'const char webpage[] PROGMEM = R"=====(' here but it led to some instability on the ESP I couldn't explain
//
//
//const char *webpage = R"===(
const char webpage[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<script>
var pageNo;
var page = [ "divHome", "divSetup" , "divWifi" , "divMqtt" , "divCustom" ];

var req = new XMLHttpRequest();
req.open('GET', document.location, false);
req.send(null);

var headers = req.getAllResponseHeaders().toLowerCase();
var arr = headers.trim().split(/[\r\n]+/);
var headerMap = {};
arr.forEach(function (line) {
  var parts = line.split(': ');
  var header = parts.shift();
  var value = parts.join(': ');
  headerMap[header] = value;
});


function goPage(x) {
   pageNo = x ;
   for (var i = 0; i < page.length; i++) {
      document.getElementById(page[ i ]).style.display = "none";
   }
   document.getElementById( page[ x - 1 ] ).style.display = "block";

    document.getElementById('wifihost').value = headerMap["wifihost"];
    document.getElementById('wifissid').value = headerMap["wifissid"];
    document.getElementById('mqttgroup').value = headerMap["mqttgroup"];
    document.getElementById('mqttserver').value = headerMap["mqttserver"];
    document.getElementById('mqttport').value = headerMap["mqttport"];
    document.getElementById('mqttuser').value = headerMap["mqttuser"];
    document.getElementById('stepsize').value = headerMap["stepsize"];
    document.getElementById('xbversion').innerHTML = headerMap["xbversion"];

}

function sendButton(id, value){
    var xhr = new XMLHttpRequest();
    var url = "/read?" + id + "=" + value;
    xhr.open("POST", url, true);
    xhr.send(); 
}

function saveselect(divId){
  var xhr = new XMLHttpRequest();
  var inputs = document.getElementById(divId).getElementsByTagName('select');
  var url = "/read?"
  
  for (i = 0; i < inputs.length; i++) {
    if(inputs[i].value.length > 0) {
      //alert(inputs[i].value);
      var tmpVal = btoa(inputs[i].value);
      url += inputs[i].id + "=" + tmpVal + "&";
    }
  }
  xhr.open("POST", url, true);
  xhr.send();
}

function save(divId){
  var xhr = new XMLHttpRequest();
  var inputs = document.getElementById(divId).getElementsByTagName('input');
  var url = "/read?"
  
  for (i = 0; i < inputs.length; i++) {
    if(inputs[i].value.length > 0) {
      //alert(inputs[i].value);
      var tmpVal = btoa(inputs[i].value);
      url += inputs[i].id + "=" + tmpVal + "&";
    }
  }
  xhr.open("POST", url, true);
  xhr.send();
}

</script>
<head>
</head>
<style>
  body {
    background-color:black;
    color:dimgray;
    font-family: arial;
    zoom: 4;
  }
  h1 {
    margin-top:5px;
    margin-bottom:5px;
    color:dimgray;
    text-shadow: 2px 2px 8px rgba(0,0,0,1);
  }
  h2, h3, h4, h5, h6 {
    color:dimgray;
    margin-top:5px;
    margin-bottom:5px;
  }
  small {
    font-size: xx-small;
  }
  table {
    border-spacing: 4px;
  }
  td {
    background-color: rgba(32,33,36,1);
    padding: 8px;
  }
  .label {
    font-size: 12pt /*1em*/ !important;
  }
  .buttonpreset {
    background-color:dimgray;
    border-radius: 4px;
    border: 1px solid;
    padding: 15px 25px;
    box-shadow: 0 8px 16px 0 rgba(0,0,0,1), 0 6px 20px 0 rgba(0,0,0,0.19);
  }
  .buttonpreset:active {
  position: relative;
  top: 1px;
  }
  .buttonmenu {
    background-color:dimgray;
    border-radius: 4px;
    border: 1px solid;
    padding: 5px 5px;
  }
  .buttonmenu:active {
  position: relative;
  top: 1px;
  }
  .input {
    background-color:lightgrey;
    border-radius: 4px;
    border: 1px solid;
  }

  .input:invalid {
    box-shadow: 0 0 5px 1px red;
  }

  .input:focus:invalid {
    box-shadow: none;
  }
</style>
<body onload="goPage(1)">

<table width="100%"><tbody>
<tr>
    <td style="text-align:center">
    <h1>xBlinds</h1>
    </td>
</tr>
<tr>
    <td>
    <div id=divHome style="text-align:center">
    <button class="buttonpreset" id="preset" onclick="sendButton(this.id, 'close')">CLOSE</button><br><br>
    <button class="buttonpreset" id="preset" onclick="sendButton(this.id, 'open')">OPEN</button><br><br>
    <button class="buttonpreset" id="preset" onclick="sendButton(this.id, 'half')">HALF</button><br><br>
    <button class="buttonpreset" id="preset" onclick="sendButton(this.id, 'full')">FULL</button><br><br>
    <br>
    </div>  <!--  <div id=divHome>  -->

    <div id=divSetup>
    <h2>Blinds setup</h2>
    <label class="label" for="save">Current position:</label><br>
    <button class="buttonmenu" id="save" onclick="sendButton(this.id, 'closed')">Save as Closed and reset</button><br>
    <br>
    <button class="buttonmenu" id="save" onclick="sendButton(this.id, 'normal')">Save as Normal Open</button><br>
    <br>
    <button class="buttonmenu" id="save" onclick="sendButton(this.id, 'half')">Override Half Open</button>&nbsp;&nbsp;<button class="buttonmenu" id="save" onclick="sendButton(this.id, 'resethalf')">Reset Half</button><br>
    <br>
    <button class="buttonmenu" id="save" onclick="sendButton(this.id, 'full')">Save as Full Open</button><br>
    <br>
    <label class="label" for="stepper">Adjust position:</label><br>
    <button id="stepper" class="buttonmenu" onclick="sendButton(this.id, 'ccw')">&nbsp;&nbsp;&nbsp;-&nbsp;&nbsp;&nbsp;</button>&nbsp;&nbsp;
    <button id="stepper" class="buttonmenu" onclick="sendButton(this.id, 'cw')">&nbsp;&nbsp;&nbsp;+&nbsp;&nbsp;&nbsp;</button>
    <br>
    <br>
    <button class="buttonmenu" onclick="goPage(5)">Experimental</button>
    <br>
    <br>
    <button class="buttonmenu" onclick="location.href='/debug'">Debug page</button>
    <br>
    <br>
    <button class="buttonmenu" id="save" onclick="sendButton(this.id, 'reboot')">Reboot ESP</button>
    <p style="text-align:right;">Version: <span id="xbversion"></span></p>
    </div>  <!--  <div id=divSetup>  -->

    <div id=divWifi>
    <h2>WiFi setup</h2>
    <label class="label" for="wifihost">Hostname:</label><input class="input" type="text" id="wifihost" name="wifihost" readonly><br>
    <br>
    <label class="label" for="wifissid">SSID:</label><input class="input" type="text" id="wifissid" name="wifissid"><br>
    <label class="label" for="wifipwd">Password:</label><input class="input" type="password" id="wifipwd" name="wifipwd"><br>
<!--<label class="label" for="wifiap">Keep internal AP enabled:</label><input class="input" type="checkbox" id="wifiap" name="wifiap"><br>
    <small>(You should disable internal AP once connected to your home SSID)</small> --->
    <br>
    <br>
    <button class="buttonmenu" id="save" onclick="save('divWifi')">SAVE</button><br><br>
    </div>  <!--  <div id=divWifi>  -->

    <div id=divMqtt>
    <h2>MQTT setup</h2>
    <label class="label" for="mqttgroup">MQTT topic:</label><input class="input" type="text" id="mqttgroup" name="mqttgroup"><br>
    <br>
    <label class="label" for="mqttserver">MQTT server:</label><input class="input" type="text" id="mqttserver" name="mqttserver"><br>
    <label class="label" for="mqttport">MQTT port:</label><input class="input" type="text" id="mqttport" name="mqttport"><br>
    <label class="label" for="mqttuser">MQTT user:</label><input class="input" type="text" id="mqttuser" name="mqttuser"><br>
    <label class="label" for="mqttpwd">MQTT password:</label><input class="input" type="password" id="mqttpwd" name="mqttpwd"><br>
    <br>
    <br>
    <button class="buttonmenu" id="save" onclick="save('divMqtt')">SAVE</button><br><br>
    </div>  <!--  <div id=divMqtt>  -->

    <div id=divCustom>
    <h4>Changing the step size can seriously ruin your day if you haven't geared down the stepper or don't know exacly what you're doing.</h4>
    <label class="label" for="stepsize">Step size (default: 512):</label><input class="input" type="number" id="stepsize" name="stepsize"><br>
    <button class="buttonmenu" id="save" onclick="save('divCustom')">SAVE</button><br><br>
    <br>
    <button class="buttonmenu" id="saveselect" onclick="saveselect('divCustom')">SAVE</button><br><br>
    </div>  <!--  <div id=divCustom>  -->
    </td>
</tr>
<tr>
    <td style="text-align:center">
    <!-- sample buttons to expose pages -->
    <button class="buttonmenu" onclick="goPage(1)">Home</button>
    <button class="buttonmenu" onclick="goPage(2)">Setup</button>
    <button class="buttonmenu" onclick="goPage(3)">WiFi</button>
    <button class="buttonmenu" onclick="goPage(4)">MQTT</button>
    </td>
</tr>
</tbody></table>

</body>
</html>
)===";
// )====="; // To be used with PROGMEM, see explanation above.