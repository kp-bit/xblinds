#include <PubSubClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266WiFi.h>
#include <Pinger.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <AccelStepper.h>
#include <EEPROM.h>
#include "Base64.h"
extern "C"
{
  #include <lwip/icmp.h>                    // needed for icmp packet definitions
}

#define motorPin1  D1                       // IN1 on the ULN2003 driver
#define motorPin2  D2                       // IN2 on the ULN2003 driver
#define motorPin3  D3                       // IN3 on the ULN2003 driver
#define motorPin4  D4                       // IN4 on the ULN2003 driver
#define STEPS 2048
#define MotorInterfaceType 8


const bool resetEEPROM = false;
const bool enableEEPROM = true;
const int buttonPin = 15;                   // Physical D8 PIN on D1 mini
size_t EEPROM_MEMORY_SIZE = 512;




// EEPROM variables
char xbidentifier[8] = "xBlinds";           // Pos 10, 10 bytes

char ssid[32] = "";                         // Pos 20, 40 bytes
char password[32] = "";                     // Pos 60, 40 bytes
char wifiap[6] = "on";                      // Pos 100, 10 bytes

char mqtt_server[16] = "";                  // Pos 110, 20 bytes
int  mqtt_port = 1883;                      // Pos 130, 10 bytes
char mqtt_user[32] = "";                    // Pos 140, 40 bytes
char mqtt_pass[32] = "";                    // Pos 180, 40 bytes
char mqtt_group[32] = "xblinds/all";        // Pos 220, 40 bytes
char mqtt_client_name[32] = "xblinds";      // Pos 260, 40 bytes

char stepperlastpreset[7] = "close";        // Pos 300, 10 bytes
long stepperhome = 0;                       // Pos 310, 10 bytes
long stepperhalf = 1024;                    // Pos 320, 10 bytes
long stepperopen = 2048;                    // Pos 330, 10 bytes
long stepperfull = 3072;                    // Pos 340, 10 bytes
long overridehalf = 0;                      // Pos 350, 10 bytes
int tiltpos = -1;                           // Pos 360, 10 bytes
char lastmove[7] = "preset";                // Pos 370, 10 bytes
long stepsize = 512;                        // Pos 380, 10 bytes


// Non-EEPROM variables
char xbversion[6] = "1.0";
long steppercurrentpos;
bool boot = true;
char mac[17];
bool allowOTA = true;
char no_status[25];
char tempport[5];
char tempstep[6];
char* tmpname;
char* mqtt_status = "xblinds/status";  // default fall-back value
char* mqtt_previous;
bool mqtt_valid = true;
int buttonState = 0;
bool buttonpressed = false;
char tilt_state[50];
char tilttopic[50];
long physicalpos;
int pctpos;
bool startup = true;
int initialRun = 0;

// This is the web interface, stored in a const - ideally the html would be read into the const at compile-time
// I make changes in the actual html file as html is a lot easier to handle in VSCode, and paste the entire file in here
// Tried using 'const char webpage[] PROGMEM = R"=====(' here but it led to some instability on the ESP I couldn't explain
//
const char *webpage = R"===(
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

WiFiClient espClient;
PubSubClient MQTTclient(espClient);
ESP8266WebServer webserver(80);
AccelStepper stepper = AccelStepper(AccelStepper::FULL4WIRE, motorPin1, motorPin3, motorPin2, motorPin4);
Pinger pinger;
ESP8266HTTPUpdateServer httpUpdater;

//Functions

// Publish retained message (new to v0.7) with selected preset on MQTT
// Added tilt_state in v0.8
//
void sendInfoMQTT(bool spooftilt = false)
{
  char mqttpayload[10];
  char tmppct[4];
  String tmpstr;
  bool retained = true;
  
  if(strcmp(lastmove, "preset") == 0) { 
    if(strcmp(stepperlastpreset, "close") == 0) { 
      strcpy(mqttpayload,"closed");
    } else {
      strcpy(mqttpayload,"open"); 
    }
  } else {
    if(tiltpos == 0) { 
      strcpy(mqttpayload,"closed"); 
    } else { 
      strcpy(mqttpayload,"open");
    }
  }
  
  Serial.print("Sending this to MQTT: ");
  Serial.print(mqttpayload);
  Serial.print(", tilt: ");
  Serial.println(tiltpos);
  
  int presetlen = strlen(mqttpayload);
  MQTTclient.publish(mqtt_status,(byte*)mqttpayload,presetlen,retained);

  tmpstr = String(tiltpos);
  tmpstr.toCharArray(tmppct,4);
  int pctlen = strlen(tmppct);
  MQTTclient.publish(tilt_state,(byte*)tmppct,pctlen,retained);
  
  // spoofing that the tilt value was sent on MQTT so it won't override a preset e.g. on reboots because it's retained
  if(spooftilt == true) { 
    Serial.println("Spooftilt is true"); 
    MQTTclient.publish(tilttopic,(byte*)tmppct,pctlen,retained);
  }
  
}

// Turn on local AP if needed and off if connected to your home wifi
//
void updatewifiap()
{
  if(strcmp(wifiap, "on") == 0) {
    Serial.println("Turning on AP");
    if(strcmp(ssid, "") != 0) {
      WiFi.mode(WIFI_AP_STA);
      Serial.println("WIFI_AP_STA");
    } else {
      WiFi.mode(WIFI_AP);
      Serial.println("WIFI_AP");
    }
    IPAddress local_IP(4,3,2,1);
    IPAddress gateway(4,3,2,1);
    IPAddress subnet(255,255,255,0);
    Serial.print("Setting soft-AP ... ");
    Serial.println(WiFi.softAP(mqtt_client_name, "persienne") ? "Soft AP ready" : "Soft AP failed!");
    delay(500);
    Serial.println(WiFi.softAPConfig(local_IP, gateway, subnet) ? "Soft AP configured" : "Soft AP config failed!");
    Serial.print("Soft AP IP address: ");
    Serial.println(WiFi.softAPIP());
  } 
  if(strcmp(wifiap, "off") == 0) {
    Serial.println("Turning off AP");
    WiFi.mode(WIFI_STA);
    Serial.println("WIFI_STA");
    WiFi.softAPdisconnect(true);
  }
}

// Connect to home wifi
//
void setup_wifi() 
{
  String tmpmac = WiFi.macAddress();
  tmpmac.replace(":","");
  tmpmac = tmpmac.substring(6);
  tmpmac.toLowerCase();
  tmpmac.toCharArray(mac,7);
  
  strcpy(mqtt_client_name, "xblinds-");
  strcpy(mqtt_client_name + 8, mac);
  WiFi.hostname(mqtt_client_name);

  if(strcmp(ssid, "") != 0) {
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);
  
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) 
    {
      delay(500);
      Serial.print(".");
    }
    WiFi.mode(WIFI_STA);
    Serial.println("WIFI_STA");
    strcpy(wifiap, "off");
    Serial.println("WiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Hostname: ");
    Serial.println(mqtt_client_name);
  }
  updatewifiap();

  webserver.on("/", defaultpage);
  webserver.on("/read", webinput);
  webserver.on("/debug", debugpage);
  httpUpdater.setup(&webserver);
  webserver.begin();
  Serial.println("HTTP server started");
}


// Code inspired by Rob Tait's Roomba control
//
void connectMQTT() 
{
  Serial.println("Connecting MQTT...");
  int retries = 0;
  IPAddress test_IP;
  bool validIP = test_IP.fromString(mqtt_server);
  strcpy(tilttopic, mqtt_group);
  strcat(tilttopic, "/tilt");
  strcpy(tilt_state, mqtt_group);
  strcat(tilt_state, "/tilt-state");
  sprintf(mqtt_status, "%s/status", mqtt_group);
  MQTTclient.setServer(mqtt_server, mqtt_port);
  MQTTclient.setKeepAlive(90);

  if(validIP == true) {
    if(pinger.Ping(mqtt_server) == true) {
   
      // Loop until we're reconnected
      while (!MQTTclient.connected()) 
      {
        if(retries < 50)
        {
          // Attempt to connect
          sprintf(no_status, "%s not connected", mqtt_client_name);
    
          if (MQTTclient.connect(mqtt_client_name, mqtt_user, mqtt_pass, mqtt_status, 0, 0, no_status, 0)) 
          {
            // Once connected, publish an announcement...
            if(boot == false)
            {
              MQTTclient.publish(mqtt_status, "Reconnected");
              delay(1000);
              //sendInfoMQTT();
            }
            if(boot == true)
            {
              MQTTclient.publish(mqtt_status, "Rebooted");
              delay(1000);
              //sendInfoMQTT();
            }
          } 
          else 
          {
            retries++;
            // Wait 5 seconds before retrying
            Serial.println("Retrying...");
            delay(5000);
          }
        }
        if(retries >= 3) {
          // Dumping out of loop
          Serial.println("ERROR: MQTT server not reachable, aborting...");
          Serial.print("Address attempted: ");
          Serial.println(mqtt_server);
          mqtt_valid = false;
          break;
        }
      } // while
    } else { 
      // ping MQTT server
      Serial.println("ERROR: MQTT Server doesn't respond to ping!");
      Serial.print("Address attempted: ");
      Serial.println(mqtt_server);
      mqtt_valid = false;
    }
  } else { 
    // MQTT IP address not valid
    Serial.print("ERROR: MQTT Server address not valid: ");
    Serial.println(mqtt_server);
    mqtt_valid = false;
  }
  if(mqtt_valid) {
    MQTTclient.subscribe(mqtt_group);
    MQTTclient.loop();
    MQTTclient.subscribe(tilttopic);
    MQTTclient.loop();
    MQTTclient.setCallback(callback);
  }
  Serial.println("MQTT connected!");
}

// Populate fields on webpage via HTTP header
//
void defaultpage()
{
  sprintf(tempport, "%d", mqtt_port);
  sprintf(tempstep, "%d", stepsize);
  Serial.println("Default");
  webserver.sendHeader("wifihost", mqtt_client_name);
  webserver.sendHeader("wifissid", ssid);
  webserver.sendHeader("wifiap", wifiap);
  webserver.sendHeader("mqttgroup", mqtt_group);
  webserver.sendHeader("mqttserver", mqtt_server);
  webserver.sendHeader("mqttport", tempport);
  webserver.sendHeader("mqttuser", mqtt_user);
  webserver.sendHeader("xbversion", xbversion);
  webserver.sendHeader("stepsize", tempstep);
  webserver.send(200, "text/html", webpage);
}

// Show the most important variables on a webpage http://ipaddress/debug
// v0.7
//
void debugpage()
{
  char tmpstr[500];
  sprintf(tmpstr, "xBlinds v%s<br><br>stepperlastpreset: %s<br>stepperhome: %d<br>stepperhalf: %d<br>stepperopen: %d<br>stepperfull: %d<br>overridehalf: %d<br>steppercurrentpos: %d<br>tiltpos: %d<br>lastmove: %s<br>stepsize: %d", xbversion, stepperlastpreset, stepperhome, stepperhalf, stepperopen, stepperfull, overridehalf, steppercurrentpos, tiltpos, lastmove, stepsize);
  webserver.send(200, "text/html", tmpstr);
}



// Receive and process input from the HTTP subpages. Strings are base64 encoded to allow for special chars etc.
//
void webinput()
{
  bool wifi_update_needed = false;
  bool mqtt_update_needed = false;
  bool eeprom_update_needed = false;
  String message = "";
  char tmpChar[9];

  for (uint8_t i = 0; i < webserver.args(); i++) {
    if (webserver.argName(i) == "wifissid") {
      if(webserver.arg(i).length() > 1) {  // only update if actually changed
        char base64tmp[64];
        webserver.arg(i).toCharArray(base64tmp, webserver.arg(i).length()+1);
        int base64len = strlen(base64tmp);
        base64_decode(ssid, base64tmp, base64len);
        eeprom_update_needed = true;
        wifi_update_needed = true;
      }
    } else if (webserver.argName(i) == "wifipwd") {
      if(webserver.arg(i).length() > 1) {  // only update if actually changed
        char base64tmp[64];
        webserver.arg(i).toCharArray(base64tmp, webserver.arg(i).length()+1);
        int base64len = strlen(base64tmp);
        base64_decode(password, base64tmp, base64len);
        eeprom_update_needed = true;
        wifi_update_needed = true;
      }
    } else if (webserver.argName(i) == "wifiap") {
      char base64tmp[20];
      webserver.arg(i).toCharArray(wifiap, webserver.arg(i).length()+1);
      int base64len = strlen(base64tmp);
      base64_decode(wifiap, base64tmp, base64len);
      eeprom_update_needed = true;
      wifi_update_needed = true;
        
    } else if (webserver.argName(i) == "mqttgroup") {
      char base64tmp[64];
      webserver.arg(i).toCharArray(base64tmp, webserver.arg(i).length()+1);
      int base64len = strlen(base64tmp);
      base64_decode(mqtt_group, base64tmp, base64len);
      eeprom_update_needed = true;
      mqtt_update_needed = true;
      
    } else if (webserver.argName(i) == "mqttserver") {
      char base64tmp[64];
      webserver.arg(i).toCharArray(base64tmp, webserver.arg(i).length()+1);
      int base64len = strlen(base64tmp);
      base64_decode(mqtt_server, base64tmp, base64len);
      eeprom_update_needed = true;
      mqtt_update_needed = true;
      
    } else if (webserver.argName(i) == "mqttport") {
      char base64tmp[16];
      webserver.arg(i).toCharArray(base64tmp, webserver.arg(i).length()+1);
      int base64len = strlen(base64tmp);
      char tmpArg[10];
      base64_decode(tmpArg, base64tmp, base64len);
      mqtt_port = atoi(tmpArg);
      eeprom_update_needed = true;
      mqtt_update_needed = true;
            
    } else if (webserver.argName(i) == "mqttuser") {
      char base64tmp[64];
      webserver.arg(i).toCharArray(base64tmp, webserver.arg(i).length()+1);
      int base64len = strlen(base64tmp);
      base64_decode(mqtt_user, base64tmp, base64len);
      eeprom_update_needed = true;
      mqtt_update_needed = true;
      
    } else if (webserver.argName(i) == "mqttpwd") {
      if(webserver.arg(i).length() > 1) {  // only update if actually changed
        char base64tmp[64];
        webserver.arg(i).toCharArray(base64tmp, webserver.arg(i).length()+1);
        int base64len = strlen(base64tmp);
        base64_decode(mqtt_pass, base64tmp, base64len);
        eeprom_update_needed = true;
        mqtt_update_needed = true;
      }

    } else if (webserver.argName(i) == "stepsize") {
      char base64tmp[16];
      webserver.arg(i).toCharArray(base64tmp, webserver.arg(i).length()+1);
      int base64len = strlen(base64tmp);
      char tmpArg[10];
      base64_decode(tmpArg, base64tmp, base64len);
      stepsize = atoi(tmpArg);
      eeprom_update_needed = true;

    } else if (webserver.argName(i) == "save") {
      // handle the saves, closed, normal (save half as well), full
      if(webserver.arg(i) == "closed")    { savestepper("closed");    }
      if(webserver.arg(i) == "normal")    { savestepper("normal");    }
      if(webserver.arg(i) == "full")      { savestepper("full");      }
      if(webserver.arg(i) == "half")      { savestepper("half");      }
      if(webserver.arg(i) == "resethalf") { savestepper("resethalf"); }
      if(webserver.arg(i) == "reboot")    { ESP.reset();              }
      
    } else if (webserver.argName(i) == "stepper") {
      if(webserver.arg(i) == "cw")      { adjuststepper("cw");  }
      if(webserver.arg(i) == "ccw")     { adjuststepper("ccw"); }
      
    } else if (webserver.argName(i) == "preset") {
      if(webserver.arg(i) == "close") { turnblinds("close"); }
      if(webserver.arg(i) == "open")  { turnblinds("open");  }
      if(webserver.arg(i) == "full")  { turnblinds("full");  }
      if(webserver.arg(i) == "half")  { turnblinds("half");  }
    }
  }
  
  if(eeprom_update_needed == true) {
    updateeeprom();
  }
  if(wifi_update_needed == true) {
    setup_wifi();
  }
  if(mqtt_update_needed == true) {
    mqtt_valid = true;
    Serial.println("MQTT must reconnect...");
    connectMQTT();
  }
}

// +/- buttons are pressed in the web interface, adjust stepper accordingly
//
void adjuststepper(char* tmpdir)
{
  if(strcmp(tmpdir, "cw") == 0) {
    steppercurrentpos = steppercurrentpos + stepsize;
    Serial.print("Attempting to move to position: ");
    Serial.println(steppercurrentpos);
    stepper.moveTo(steppercurrentpos);
    stepper.runToPosition();
  }
  if(strcmp(tmpdir, "ccw") == 0) {
    steppercurrentpos = steppercurrentpos - stepsize;
    Serial.print("Attempting to move to position: ");
    Serial.println(steppercurrentpos);
    stepper.moveTo(steppercurrentpos);
    stepper.runToPosition();
  }
}


// Save preset is pressed in the web interface
//
void savestepper(char* tmppos)
{
  bool stepper_update_needed = false;
  
  if(strcmp(tmppos, "normal") == 0) {
    stepperopen = steppercurrentpos;
    if(stepperopen > 0) {
      stepperhalf = stepperopen / 2;
    } else {
      stepperhalf = 0;
    }
    Serial.println("Save open and half pos");
    Serial.print("Open: ");
    Serial.println(stepperopen);
    Serial.print("Half: ");
    Serial.println(stepperhalf);
    if(enableEEPROM == true) { stepper_update_needed = true; }
  }
  if(strcmp(tmppos, "closed") == 0) {
    steppercurrentpos = 0;
    stepperhome = 0;
    stepperhalf = 1024;
    stepperopen = 2048;
    stepperfull = 3072;
    overridehalf = 0;
    stepper.setCurrentPosition(0);
    stepper.setMaxSpeed(500);
    stepper.setAcceleration(100);
    Serial.println("Save closed pos and reset to defaults");  
    if(enableEEPROM == true) { stepper_update_needed = true; }
  }
  if(strcmp(tmppos, "full") == 0) {
    stepperfull = steppercurrentpos;
    Serial.println("Save full pos");  
    if(enableEEPROM == true) { stepper_update_needed = true; }
  }
  if(strcmp(tmppos, "half") == 0) {
    overridehalf = steppercurrentpos;
    Serial.println("Save half override");  
    if(enableEEPROM == true) { stepper_update_needed = true; }
  }
  if(strcmp(tmppos, "resethalf") == 0) {
    overridehalf = 0;
    Serial.println("Reset half override");  
    if(enableEEPROM == true) { stepper_update_needed = true; }
  }

  if(stepper_update_needed == true) {
    savestepperpresets(); 
  }
}

// Called when tactile button is pressed. Will close blinds if they're open and vice versa.
// If MQTT is used, function will post a retained message to MQTT that will trigger the move (v0.9)
//
void toggleblinds()
{
  char mqttpayload[10];
  bool retained = true;
  
  if (MQTTclient.connected()) {
    MQTTclient.publish(mqtt_status, "Button pressed");
  }

  if(strcmp(stepperlastpreset, "close") != 0) {
    Serial.println("Toggle to close");
    if (MQTTclient.connected()) {
      strcpy(mqttpayload,"close");
      int presetlen = strlen(mqttpayload);
      MQTTclient.publish(mqtt_group,(byte*)mqttpayload,presetlen,retained);
    } else {
      turnblinds("close");
    }
  } else {
    Serial.println("Toogle to open");
    if (MQTTclient.connected()) {
      strcpy(mqttpayload,"open");
      int presetlen = strlen(mqttpayload);
      MQTTclient.publish(mqtt_group,(byte*)mqttpayload,presetlen,retained);
    } else {
      turnblinds("open");
    }
  }
}


// Sending the preset positions to the stepper based on passed keyword and posting status to MQTT
//
void turnblinds(char* tmppos)
{
  stepper.enableOutputs();
  
  if((strcmp(tmppos, "open") == 0) and (stepperopen != steppercurrentpos)) {
    MQTTclient.publish(mqtt_status, "opening");
    strcpy(lastmove, "preset");
    stepper.moveTo(stepperopen);
    Serial.println("Preset: Open");
    stepper.runToPosition();
    strcpy(stepperlastpreset, "open");
    steppercurrentpos = stepperopen;
    Serial.println(steppercurrentpos);
    tiltpos = float(stepperopen) / float(stepperfull) * 100;
    Serial.println(tiltpos);
    if(enableEEPROM == true) { savestepperpresets(); }
    sendInfoMQTT();
  }
  
  if((strcmp(tmppos, "close") == 0) and (stepperhome != steppercurrentpos)) {
    MQTTclient.publish(mqtt_status, "closing");
    strcpy(lastmove, "preset");
    stepper.moveTo(stepperhome);
    Serial.println("Preset: Close");
    stepper.runToPosition();
    strcpy(stepperlastpreset, "close");
    steppercurrentpos = stepperhome;
    Serial.println(steppercurrentpos);
    tiltpos = 0;
    Serial.println(tiltpos);
    if(enableEEPROM == true) { savestepperpresets(); }
    sendInfoMQTT();
  }
  
  if((strcmp(tmppos, "full") == 0) and (stepperfull != steppercurrentpos)) {    
    MQTTclient.publish(mqtt_status, "opening");
    strcpy(lastmove, "preset");
    stepper.moveTo(stepperfull);
    Serial.println("Preset: Full");
    stepper.runToPosition();
    strcpy(stepperlastpreset, "full");
    steppercurrentpos = stepperfull;
    Serial.println(steppercurrentpos);
    tiltpos = 100;
    Serial.println(tiltpos);
    if(enableEEPROM == true) { savestepperpresets(); }
    sendInfoMQTT();
  }
  
  if(strcmp(tmppos, "half") == 0) {
    if((overridehalf != 0) and (overridehalf != steppercurrentpos)) {
      MQTTclient.publish(mqtt_status, "opening");
      strcpy(lastmove, "preset");
      stepper.moveTo(overridehalf);
      Serial.println("Preset: Override half");
      steppercurrentpos = overridehalf;
      tiltpos = float(overridehalf) / float(stepperfull) * 100;
      Serial.println(tiltpos);
      stepper.runToPosition();
      strcpy(stepperlastpreset, "half");
      Serial.println(steppercurrentpos);
      if(enableEEPROM == true) { savestepperpresets(); }
      sendInfoMQTT();
    } 
    if((overridehalf == 0) and (stepperhalf != steppercurrentpos)) {
      MQTTclient.publish(mqtt_status, "opening");
      strcpy(lastmove, "preset");
      stepper.moveTo(stepperhalf);
      Serial.println("Preset: Half");
      steppercurrentpos = stepperhalf;
      tiltpos = float(stepperhalf) / float(stepperfull) * 100;
      Serial.println(tiltpos);
      stepper.runToPosition();
      strcpy(stepperlastpreset, "half");
      Serial.println(steppercurrentpos);
      if(enableEEPROM == true) { savestepperpresets(); }
      sendInfoMQTT();
    }
  }
}


// Sending tilt positions to the stepper based on received tilt percentage received on MQTT
// v0.8 addition
//
void tiltblinds(int tiltpct)
{
  Serial.print("Tilt received: ");
  Serial.println(tiltpct);

  if (tiltpct != tiltpos) {
    stepper.enableOutputs();
    
    if (tiltpct > tiltpos) {
      MQTTclient.publish(mqtt_status, "opening");
    } else {
      MQTTclient.publish(mqtt_status, "closing");
    }

    physicalpos = ((float(stepperfull) / 100) * float(tiltpct));
    
    Serial.print("Running to pos: ");
    Serial.println(physicalpos);
    
    stepper.moveTo(physicalpos);
    stepper.runToPosition();
    steppercurrentpos = physicalpos;
    
    tiltpos = tiltpct;
    strcpy(lastmove, "tilt");

    if(enableEEPROM == true) { savestepperpresets(); }

    sendInfoMQTT();
  } else {
    Serial.println("Ignoring...");
  }
}

// Read all variables from flash memory and dump to terminal, only done on startup
// If xBlinds has never been installed on this ESP before, clear memory
//
void readeeprom()
{
  if(enableEEPROM == true) {
    EEPROM.begin(EEPROM_MEMORY_SIZE);
    delay(100);
    EEPROM.get(10, xbidentifier);
    
    if((strcmp(xbidentifier, "xBlinds") == 0) and (resetEEPROM == false)) {
      Serial.println("Reading stored values from EEPROM...");
      EEPROM.get(20, ssid);
      EEPROM.get(60, password); 
      EEPROM.get(100, wifiap);
      EEPROM.get(110, mqtt_server);
      EEPROM.get(130, mqtt_port);
      EEPROM.get(140, mqtt_user);
      EEPROM.get(180, mqtt_pass);
      EEPROM.get(220, mqtt_group);
      EEPROM.get(260, mqtt_client_name);
      EEPROM.get(300, stepperlastpreset);
      EEPROM.get(310, stepperhome);
      EEPROM.get(320, stepperhalf);
      EEPROM.get(330, stepperopen);
      EEPROM.get(340, stepperfull);
      EEPROM.get(350, overridehalf);
      EEPROM.get(360, tiltpos);
      EEPROM.get(370, lastmove);
      EEPROM.get(380, stepsize);

      if(stepsize == 0) { stepsize = 512; }                   // if never saved

      Serial.print("Identifier: ");
      Serial.println(xbidentifier);
      Serial.print("SSID: ");
      Serial.println(ssid);
      Serial.print("WiFi pw: ");
      Serial.println(password);
      Serial.print("Local AP: ");
      Serial.println(wifiap);
      Serial.print("MQTT server: ");
      Serial.println(mqtt_server);
      Serial.print("MQTT port: ");
      Serial.println(mqtt_port);
      Serial.print("MQTT user: ");
      Serial.println(mqtt_user);
      Serial.print("MQTT pw: ");
      Serial.println(mqtt_pass);
      Serial.print("MQTT topic: ");
      Serial.println(mqtt_group);
      Serial.print("MQTT client: ");
      Serial.println(mqtt_client_name);
      Serial.print("Stepper last preset: ");
      Serial.println(stepperlastpreset);
      Serial.print("Stepper home: ");
      Serial.println(stepperhome);
      Serial.print("Stepper half: ");
      Serial.println(stepperhalf);
      Serial.print("Stepper open: ");
      Serial.println(stepperopen);
      Serial.print("Stepper full: ");
      Serial.println(stepperfull);
      Serial.print("Override half: ");
      Serial.println(overridehalf);
      Serial.print("Tilt pos: ");
      Serial.println(tiltpos);
      Serial.print("Last move: ");
      Serial.println(lastmove);
      Serial.print("Step size: ");
      Serial.println(stepsize);
      
    } else {
      
      // clear EEPROM as we haven't written to it before
      Serial.println("Erasing EEPROM...");
      EEPROM.begin(EEPROM_MEMORY_SIZE);
      for (int i = 0 ; i < EEPROM.length() ; i++) {
        EEPROM.write(i, 0);
      }
      delay(100);
      EEPROM.commit();
      
      Serial.println("EEPROM erased!");
      Serial.println("Saving defaults to EEPROM...");
      updateeeprom();

    }
  }
  
  // Make sure the stepper has the same idea about the world as the ESP when waking up
  if(strcmp(lastmove, "preset") == 0) {
    // Presets were used last
    if(strcmp(stepperlastpreset, "close") == 0) { 
      steppercurrentpos = stepperhome; 
      stepper.setCurrentPosition(steppercurrentpos);
    }
    if(strcmp(stepperlastpreset, "open") == 0)   { 
      steppercurrentpos = stepperopen;
      stepper.setCurrentPosition(steppercurrentpos);
    }
    if(strcmp(stepperlastpreset, "full") == 0)   { 
      steppercurrentpos = stepperfull;
      stepper.setCurrentPosition(steppercurrentpos);
    }
    if(strcmp(stepperlastpreset, "half") == 0)   {
      if(overridehalf != 0) {
        steppercurrentpos = overridehalf;
      } else {
        steppercurrentpos = stepperhalf;
      }
      stepper.setCurrentPosition(steppercurrentpos);
    }
  } else {
    // Tilt was used last
    steppercurrentpos = ((float(stepperfull) / 100) * float(tiltpos));
    stepper.setCurrentPosition(steppercurrentpos);
  }
}

// Save configuration variables to flash memory and read them back to terminal for verification
//
void updateeeprom()
{
  if(enableEEPROM == true) {
    Serial.println("Updating EEPROM...");
    EEPROM.begin(EEPROM_MEMORY_SIZE);

    EEPROM.put(10, "xBlinds");
    delay(100);
    EEPROM.put(20, ssid);
    delay(100);
    EEPROM.put(60, password); 
    delay(100);
    EEPROM.put(100, wifiap);
    delay(100);
    EEPROM.put(110, mqtt_server);
    delay(100);
    EEPROM.put(130, mqtt_port);
    delay(100);
    EEPROM.put(140, mqtt_user);
    delay(100);
    EEPROM.put(180, mqtt_pass);
    delay(100);
    EEPROM.put(220, mqtt_group);
    delay(100);
    EEPROM.put(260, mqtt_client_name);
    delay(100);
    EEPROM.put(380, stepsize);
    delay(100);

    EEPROM.commit();

    Serial.println("Confirming EEPROM...");
    char tmp1[8];
    EEPROM.get(10, tmp1);
    Serial.println(tmp1);

    char tmp2[32];
    EEPROM.get(20, tmp2);
    Serial.println(tmp2);

    char tmp3[32];
    EEPROM.get(60, tmp3);
    Serial.println(tmp3);

    char tmp4[6];
    EEPROM.get(100, tmp4);
    Serial.println(tmp4);

    char tmp4b[16];
    EEPROM.get(110, tmp4b);
    Serial.println(tmp4b);

    int tmp5;
    EEPROM.get(130, tmp5);
    Serial.println(tmp5);

    char tmp6[32];
    EEPROM.get(140, tmp6);
    Serial.println(tmp6);

    char tmp7[32];
    EEPROM.get(180, tmp7);
    Serial.println(tmp7);

    char tmp8[32];
    EEPROM.get(220, tmp8);
    Serial.println(tmp8);

    char tmp9[32];
    EEPROM.get(260, tmp9);
    Serial.println(tmp9);

    long tmp10;
    EEPROM.get(380, tmp10);
    Serial.println(tmp10);

    int tmp11;
    EEPROM.get(390, tmp11);
    Serial.println(tmp11);

  }
}

// Write stepper variables to flash memory
//
void savestepperpresets()
{
    Serial.println("Saving presets...");
    EEPROM.begin(EEPROM_MEMORY_SIZE);
    delay(100);
    EEPROM.put(300, stepperlastpreset);
    delay(100);
    EEPROM.put(310, stepperhome);
    delay(100);
    EEPROM.put(320, stepperhalf);
    delay(100);
    EEPROM.put(330, stepperopen);
    delay(100);
    EEPROM.put(340, stepperfull);
    delay(100);
    EEPROM.put(350, overridehalf);
    delay(100);
    EEPROM.put(360, tiltpos);
    delay(100);
    EEPROM.put(370, lastmove);
    delay(100);
    EEPROM.commit();
}

// React to known topics on MQTT, code inspired by Rob Tait's Roomba control
//
void callback(char* topic, byte* payload, unsigned int length) 
{
  Serial.println("MQTT callback...");
  String newTopic = topic;
  payload[length] = '\0';
  String newPayload = String((char *)payload);
  String listento(mqtt_group);
  String tilt = listento + "/tilt";

  if((startup == false) or ((startup == true) and strcmp(lastmove, "preset") == 0)) {
    if (newTopic == listento) {
      if (newPayload == "close") { turnblinds("close"); }
      if (newPayload == "open")  { turnblinds("open");  }
      if (newPayload == "half")  { turnblinds("half");  }
      if (newPayload == "full")  { turnblinds("full");  }
    } 
  }
  if((startup == false) or ((startup == true) and strcmp(lastmove, "tilt") == 0)) {
    if (newTopic == tilt)        { tiltblinds(newPayload.toInt());  }
  } 
  if(initialRun < 3) { initialRun++; }
}


void setup() 
{
  delay(1000);
  Serial.begin(115200);
  delay(1000);
  Serial.println("Setup starting...");
  
  // Set up input from the optional tactile button
  pinMode(buttonPin, INPUT);

  // restore values from flash memory if enabled (only disabled for testing)
  if(enableEEPROM) { readeeprom(); }
  
  setup_wifi();

  // Set up OTA from Arduino IDE
  ArduinoOTA.setHostname(mqtt_client_name);
  ArduinoOTA.begin();

  // Set the stepper speed parameters, slower run and acceleration = more torque
  stepper.setMaxSpeed(500);       // set the max motor speed
  stepper.setAcceleration(100);   // set the acceleration

  
  if((strcmp(ssid, "") != 0) and (strcmp(mqtt_server, "") != 0) and (mqtt_valid == true)) {
    connectMQTT();
    MQTTclient.loop();
    delay(200);
    MQTTclient.loop();
    delay(200);
  }

  Serial.println("Setup ended!");
}

void loop() 
{
  if(initialRun >= 2) { startup = false; }

  if (!MQTTclient.connected() and (!stepper.run())) 
  {
    Serial.println("Reconnecting MQTT");
    Serial.print("SSID: ");
    Serial.println(ssid);
    Serial.print("MQTT: ");
    Serial.println(mqtt_server);
    Serial.print("MQTT valid: ");
    Serial.println(mqtt_valid);
    
    if((strcmp(ssid, "") != 0) and (strcmp(mqtt_server, "") != 0) and (mqtt_valid == true)) {
      initialRun = 0;
      connectMQTT();
    }
  }

  // Handle the pressing of optional tactile button, including really slow presses
  buttonState = digitalRead(buttonPin);
  if (buttonState == HIGH and buttonpressed == false) {
    buttonpressed = true;    
    toggleblinds();
  } else if(buttonState == LOW) {
    buttonpressed = false;
  }
   
  if (allowOTA == true)
  {
    ArduinoOTA.handle();
  }

  webserver.handleClient();   // HTTP
  MQTTclient.loop();          // MQTT

  // Make sure we turn off the stepper when not in use
  if (!stepper.run()) {
    stepper.disableOutputs();
  }

  
}
