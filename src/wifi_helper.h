#ifndef WIFI_HELPER_H
#define WIFI_HELPER_H

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include "wifi_config.h"

// WiFi timeout for connection attempts (milliseconds)
#ifndef WIFI_TIMEOUT
#define WIFI_TIMEOUT 5000  // 5 seconds per network
#endif

// ANSI color codes for serial output (only reliable colors)
#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_MAGENTA "\033[35m"

static Preferences preferences;

struct PermitData {
  char permitNumber[20];
  char plateNumber[20];
  char validFrom[30];
  char validTo[30];
  char barcodeValue[20];
  char barcodeLabel[20];
};

// Helper function to safely copy JSON string to char array
void safeJsonCopy(char* dest, size_t destSize, JsonDocument& doc, const char* key) {
  const char* value = doc[key] | "";
  strncpy(dest, value, destSize - 1);
  dest[destSize - 1] = '\0';
}

// Load permit data from flash memory
bool loadPermitData(PermitData* data) {
  preferences.begin("permit", true); // true = read-only
  
  bool hasData = preferences.isKey("permitNum");
  
  if (hasData) {
    preferences.getString("permitNum", data->permitNumber, sizeof(data->permitNumber));
    preferences.getString("plateNum", data->plateNumber, sizeof(data->plateNumber));
    preferences.getString("validFrom", data->validFrom, sizeof(data->validFrom));
    preferences.getString("validTo", data->validTo, sizeof(data->validTo));
    preferences.getString("barcodeVal", data->barcodeValue, sizeof(data->barcodeValue));
    preferences.getString("barcodeLabel", data->barcodeLabel, sizeof(data->barcodeLabel));
    
    preferences.end();
    Serial.print(COLOR_GREEN);
    Serial.print("Loaded permit data from flash. Permit #: ");
    Serial.print(data->permitNumber);
    Serial.print(COLOR_RESET);
    Serial.println();
    return true;
  }
  
  preferences.end();
  Serial.print(COLOR_YELLOW);
  Serial.print("No saved permit data found in flash.");
  Serial.print(COLOR_RESET);
  Serial.println();
  return false;
}

// Save permit data to flash memory
void savePermitData(PermitData* data) {
  preferences.begin("permit", false); // false = read/write
  
  preferences.putString("permitNum", data->permitNumber);
  preferences.putString("plateNum", data->plateNumber);
  preferences.putString("validFrom", data->validFrom);
  preferences.putString("validTo", data->validTo);
  preferences.putString("barcodeVal", data->barcodeValue);
  preferences.putString("barcodeLabel", data->barcodeLabel);
  
  preferences.end();
  Serial.print(COLOR_GREEN);
  Serial.print("Saved permit data to flash. Permit #: ");
  Serial.print(data->permitNumber);
  Serial.print(COLOR_RESET);
  Serial.println();
}

// OPTIMIZED: Scan-first WiFi connection for faster connection
bool connectToWiFi() {
  Serial.println("\n=== WiFi Connection Attempt ===");
  Serial.println("Scanning for available networks...");
  
  // Scan for networks (async=false, show_hidden=false, passive=false, max_ms=200)
  int n = WiFi.scanNetworks(false, false, false, 200);
  
  if (n == 0) {
    Serial.print(COLOR_RED);
    Serial.print("No networks found!");
    Serial.print(COLOR_RESET);
    Serial.println();
    return false;
  }
  
  Serial.print("Found ");
  Serial.print(n);
  Serial.println(" networks");
  
  // Priority order: check which networks are in range
  struct NetworkPriority {
    const char* ssid;
    const char* password;
    const char* name;
    int rssi;  // Signal strength
    bool found;
  };
  
  NetworkPriority networks[] = {
    {WIFI_SSID_1, WIFI_PASS_1, "Vytis_Svecias", -999, false},
    {WIFI_SSID_2, WIFI_PASS_2, "phone", -999, false},
    {WIFI_SSID_3, WIFI_PASS_3, "36Batavia", -999, false}
  };
  
  // Check which of our networks are available
  for (int i = 0; i < n; i++) {
    String ssid = WiFi.SSID(i);
    int rssi = WiFi.RSSI(i);
    
    for (int j = 0; j < 3; j++) {
      if (ssid == networks[j].ssid) {
        networks[j].found = true;
        networks[j].rssi = rssi;
        Serial.print("  Found: ");
        Serial.print(networks[j].name);
        Serial.print(" (RSSI: ");
        Serial.print(rssi);
        Serial.println(" dBm)");
      }
    }
  }
  
  // Try to connect in priority order
  for (int i = 0; i < 3; i++) {
    if (networks[i].found) {
      Serial.print("Connecting to ");
      Serial.print(networks[i].name);
      Serial.println("...");
      
      // Check if password is empty (open network)
      if (strlen(networks[i].password) == 0) {
        WiFi.begin(networks[i].ssid);
      } else {
        WiFi.begin(networks[i].ssid, networks[i].password);
      }
      
      unsigned long startTime = millis();
      while (WiFi.status() != WL_CONNECTED && millis() - startTime < WIFI_TIMEOUT) {
        delay(100);
        Serial.print(".");
      }
      
      if (WiFi.status() == WL_CONNECTED) {
        Serial.println();
        Serial.print(COLOR_GREEN);
        Serial.print("Connected to ");
        Serial.print(networks[i].name);
        Serial.print("!");
        Serial.print(COLOR_RESET);
        Serial.println();
        Serial.print("  IP: ");
        Serial.println(WiFi.localIP());
        Serial.print("  Signal: ");
        Serial.print(networks[i].rssi);
        Serial.println(" dBm");
        return true;
      }
      
      Serial.println();
      Serial.print(COLOR_YELLOW);
      Serial.print("Failed to connect to ");
      Serial.print(networks[i].name);
      Serial.print(COLOR_RESET);
      Serial.println();
      WiFi.disconnect();
    }
  }
  
  Serial.print(COLOR_RED);
  Serial.print("None of your configured networks are in range.");
  Serial.print(COLOR_RESET);
  Serial.println();
  return false;
}

// FIXED: Added forceUpdate parameter
// Returns: 0 = error, 1 = updated, 2 = already up to date
int downloadPermitData(PermitData* data, const char* currentPermitNumber, bool forceUpdate = false) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.print(COLOR_RED);
    Serial.print("Not connected to WiFi!");
    Serial.print(COLOR_RESET);
    Serial.println();
    return 0;
  }

  WiFiClientSecure client;
  client.setInsecure();  // Skip certificate validation for simplicity

  HTTPClient http;

  // Add cache-busting parameter for force updates to bypass GitHub CDN cache
  String url = SERVER_URL;
  if (forceUpdate) {
    url += "?t=";
    url += String(millis());  // Add timestamp to make URL unique
    Serial.print(COLOR_MAGENTA);
    Serial.print("Force update - bypassing CDN cache");
    Serial.print(COLOR_RESET);
    Serial.println();
  }

  http.begin(client, url.c_str());
  http.setTimeout(10000);  // 10 second timeout

  Serial.print("Downloading permit data from ");
  Serial.println(url);

  int httpCode = http.GET();
  
  if (httpCode == 200) {
    String payload = http.getString();
    Serial.println("Download successful!");
    
    // Parse JSON
    // ArduinoJson v7 uses JsonDocument with automatic memory management
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);
    
    if (error) {
      Serial.print(COLOR_RED);
      Serial.print("JSON parsing failed: ");
      Serial.print(error.c_str());
      Serial.print(" (Check JSON format)");
      Serial.print(COLOR_RESET);
      Serial.println();
      http.end();
      return 0;
    }
    
    Serial.println("Permit data raw:");
    Serial.print(payload);
    Serial.println();
    
    // Validate that required field exists and is a string
    if (!doc[JSON_PERMIT_NUMBER].is<const char*>()) {
      Serial.print(COLOR_RED);
      Serial.print("JSON missing required field: permit number");
      Serial.print(COLOR_RESET);
      Serial.println();
      http.end();
      return 0;
    }
    
    const char* newPermitNumber = doc[JSON_PERMIT_NUMBER];
    
    // Check if permit number is empty
    if (strlen(newPermitNumber) == 0) {
      Serial.print(COLOR_RED);
      Serial.print("Permit number is empty in JSON response");
      Serial.print(COLOR_RESET);
      Serial.println();
      http.end();
      return 0;
    }
    
    Serial.print(COLOR_MAGENTA);
    Serial.print("Server permit #: ");
    Serial.print(newPermitNumber);
    Serial.print(COLOR_RESET);
    Serial.println();
    Serial.print(COLOR_MAGENTA);
    Serial.print("Current permit #: ");
    Serial.print(currentPermitNumber);
    Serial.print(COLOR_RESET);
    Serial.println();
    
    // FIXED: Compare permit numbers (skip check if forcing update)
    if (!forceUpdate && strcmp(newPermitNumber, currentPermitNumber) == 0) {
      Serial.print(COLOR_YELLOW);
      Serial.print("Permit number matches. No changes needed.");
      Serial.print(COLOR_RESET);
      Serial.println();
      http.end();
      return 2;  // Already up to date
    }
    
    // FIXED: Show appropriate message based on forceUpdate flag
    if (forceUpdate) {
      Serial.print(COLOR_MAGENTA);
      Serial.print("Force update - downloading regardless of permit number");
      Serial.print(COLOR_RESET);
      Serial.println();
    } else {
      Serial.print(COLOR_GREEN);
      Serial.print("New permit detected! Updating...");
      Serial.print(COLOR_RESET);
      Serial.println();
    }
    
    // Extract data from JSON using helper function
    safeJsonCopy(data->permitNumber, sizeof(data->permitNumber), doc, JSON_PERMIT_NUMBER);
    safeJsonCopy(data->plateNumber, sizeof(data->plateNumber), doc, JSON_PLATE_NUMBER);
    safeJsonCopy(data->validFrom, sizeof(data->validFrom), doc, JSON_VALID_FROM);
    safeJsonCopy(data->validTo, sizeof(data->validTo), doc, JSON_VALID_TO);
    safeJsonCopy(data->barcodeValue, sizeof(data->barcodeValue), doc, JSON_BARCODE_VALUE);
    safeJsonCopy(data->barcodeLabel, sizeof(data->barcodeLabel), doc, JSON_BARCODE_LABEL);
    
    http.end();
    Serial.print(COLOR_GREEN);
    Serial.print("Permit data parsed successfully!");
    Serial.print(COLOR_RESET);
    Serial.println();
    
    // Save to flash memory
    savePermitData(data);
    
    return 1;  // Updated
  } else {
    Serial.print(COLOR_RED);
    Serial.print("HTTP request failed: ");
    if (httpCode > 0) {
      Serial.print("HTTP ");
      Serial.print(httpCode);
      if (httpCode == 404) Serial.print(" (File not found)");
      else if (httpCode == 403) Serial.print(" (Access denied)");
      else if (httpCode == 500) Serial.print(" (Server error)");
    } else {
      Serial.print("Network error (");
      Serial.print(httpCode);
      Serial.print(")");
    }
    Serial.print(COLOR_RESET);
    Serial.println();
    http.end();
    return 0;  // Error
  }
}

void disconnectWiFi() {
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  Serial.println("WiFi disconnected to save power.");
}

#endif