#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <IRremote.hpp>

const char* ssid = "spiderlan"; 
const char* password = "Aaron1028!";
#define IR_RECEIVE_PIN 2
#define IR_TRANSMIT_PIN 4

AsyncWebServer server(80);
const char* jsonPath = "/ir.json";

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>IR Signal Recorder</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; background-color: #f4f6f9; color: #333; text-align: center; }
        .container { max-width: 900px; margin: auto; background: white; padding: 20px; border-radius: 10px; box-shadow: 0 4px 6px rgba(0,0,0,0.1); }
        h1 { color: #007bff; }
        .btn { display: inline-block; padding: 8px 15px; margin: 5px; font-size: 14px; border: none; border-radius: 4px; cursor: pointer; text-decoration: none; }
        .btn-blue { background-color: #007bff; color: white; }
        .btn-red { background-color: #dc3545; color: white; }
        .btn-green { background-color: #28a745; color: white; }
        table { width: 100%; border-collapse: collapse; margin-top: 20px; }
        th, td { padding: 12px; border: 1px solid #ddd; text-align: left; }
        th { background-color: #f2f2f2; }
        tr:nth-child(even) { background-color: #fafafa; }
        .name-tag { font-weight: bold; color: #495057; background: #e9ecef; padding: 3px 8px; border-radius: 3px; }
    </style>
</head>
<body>
    <div class="container">
        <h1>IR Signal Recorder</h1>
        <a href="/json" target="_blank" class="btn btn-blue">View Raw JSON</a>
        <button onclick="clearLogs()" class="btn btn-red">Clear All</button>

        <table>
            <thead>
                <tr><th>Name</th><th>Protocol</th><th>Command</th><th>Address</th><th>Actions</th></tr>
            </thead>
            <tbody id="logTable"><tr><td colspan="5">Loading...</td></tr></tbody>
        </table>
    </div>

<script>
let isEditing = false;

function loadData() {
    if (isEditing) return;

    fetch('/json')
        .then(r => r.json())
        .then(data => {
            let tbody = document.getElementById('logTable');
            tbody.innerHTML = '';

            if (!data || data.length === 0) {
                tbody.innerHTML = '<tr><td colspan="5">No codes saved.</td></tr>';
                return;
            }

            for (let i = data.length - 1; i >= 0; i--) {
                let row = `<tr>
                    <td>${data[i].name ? `<span class="name-tag">${data[i].name}</span>` : '<i>Unnamed</i>'}</td>
                    <td>${data[i].protocol}</td>
                    <td>${data[i].command}</td>
                    <td>${data[i].address}</td>
                    <td>
                        <button onclick="renameLog(${data[i].timestamp})" class="btn btn-green">Name</button>
                        <button onclick="deleteLog(${data[i].timestamp})" class="btn btn-red">Delete</button>
                    </td>
                </tr>`;
                tbody.innerHTML += row;
            }
        });
}

function renameLog(ts) {
    isEditing = true;
    let name = prompt("Enter a name:");
    if (name) {
        fetch(`/rename?timestamp=${ts}&name=${encodeURIComponent(name)}`, { method: 'POST' })
            .then(() => { isEditing = false; loadData(); });
    } else {
        isEditing = false;
    }
}

function deleteLog(ts) {
    if (confirm("Delete this specific log?")) {
        fetch(`/delete?timestamp=${ts}`, { method: 'POST' })
            .then(() => loadData());
    }
}

function clearLogs() {
    if (confirm("Delete all logs?")) {
        fetch('/clear', { method: 'POST' })
            .then(() => loadData());
    }
}

setInterval(loadData, 2000);
window.onload = loadData;
</script>
</body>
</html>
)rawliteral";

void saveIRToJSON(String protocol, String command, String address) {
    JsonDocument doc;

    if (LittleFS.exists(jsonPath)) {
        File file = LittleFS.open(jsonPath, "r");
        DeserializationError err = deserializeJson(doc, file);
        file.close();

        if (err || !doc.is<JsonArray>()) {
            doc.clear();
            doc.to<JsonArray>();
        }
    } else {
        doc.to<JsonArray>();
    }

    JsonArray array = doc.as<JsonArray>();

    JsonObject obj = array.add<JsonObject>();
    obj["timestamp"] = millis();
    obj["name"] = "";
    obj["protocol"] = protocol;
    obj["command"] = command;
    obj["address"] = address;

    File file = LittleFS.open(jsonPath, "w");
    serializeJson(doc, file);
    file.close();
}

void readJSON() {
    File f = LittleFS.open(jsonPath, "r");
    if (!f) {
        Serial.println("Failed to open ir.json. (run receiver first and grab signals)");
        return;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, f);
    f.close();

    if (err) {
        Serial.print("Failed to parse JSON: ");
        Serial.println(err.f_str());
        return;
    }

    JsonArray array = doc.as<JsonArray>();

    for(JsonObject item: array) {
        Serial.print("Name: ");
        Serial.println(item["name"].as<const char*>());
        Serial.print("Protocol: ");
        Serial.println(item["protocol"].as<const char*>());
        Serial.print("Command: ");
        Serial.println(item["command"].as<const char*>());
        Serial.print("Address: ");
        Serial.println(item["address"].as<const char*>());
    }
}

void sendIR(String commandHex, String addressHex, String protocol) {
    unsigned long command = strtoul(commandHex.c_str(), NULL, 16);
    unsigned long address = strtoul(addressHex.c_str(), NULL, 16);

    protocol.trim();
    protocol.toUpperCase();

    if (protocol == "NEC") {
        IrSender.sendNEC(address, command, 0); // i only tried NEC before so i am not sure if the other protocols work, but they should.
    } else if (protocol == "Sony") {
        IrSender.sendSony(address, command, 12); // i also heard that Sony protocol is 12 bits, but you might need to adjust this based on your specific device.
    } else if (protocol == "RC5") {
        IrSender.sendRC5(address, command, 0);
    } else if (protocol == "Panasonic") {
        IrSender.sendPanasonic(address, command, 0);
    } else if (protocol == "SAMSUNG") {
        IrSender.sendSAMSUNG(address, command);
    } else {
        Serial.println("Unsupported protocol: " + protocol);
    }
}

void sendByName(String name) {
    File f = LittleFS.open(jsonPath, "r");
    if (!f) {
        Serial.println("Failed to open ir.json.");
        return;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, f);
    f.close();

    if (err) {
        Serial.print("Failed to parse JSON: ");
        Serial.println(err.f_str());
        return;
    }

    JsonArray array = doc.as<JsonArray>();

    for(JsonObject item: array) {
        if (item["name"] == name) {
            String protocol = item["protocol"].as<String>();
            String commandHex = item["command"].as<String>();
            String addressHex = item["address"].as<String>();
            sendIR(commandHex, addressHex, protocol);
            return;
        }
    }

    Serial.println("No IR signal found with the name: " + name);
}

void setup() {
    Serial.begin(115200);

    if (!LittleFS.begin(true)) return;

    if (!LittleFS.exists(jsonPath)) {
        File f = LittleFS.open(jsonPath, "w");
        f.print("[]");
        f.close();
    }

    IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);
    IrSender.begin(IR_TRANSMIT_PIN);

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) delay(500);
    Serial.println(WiFi.localIP());

    server.on("/record", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", index_html);
    });

    server.on("/json", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(LittleFS, jsonPath, "application/json");
    });

    server.on("/rename", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (request->hasParam("timestamp") && request->hasParam("name")) {
            String ts = request->getParam("timestamp")->value();
            String name = request->getParam("name")->value();

            JsonDocument doc;
            File f = LittleFS.open(jsonPath, "r");
            deserializeJson(doc, f);
            f.close();

            for (JsonObject item : doc.as<JsonArray>()) {
                if (String(item["timestamp"].as<unsigned long>()) == ts)
                    item["name"] = name;
            }

            File wf = LittleFS.open(jsonPath, "w");
            serializeJson(doc, wf);
            wf.close();

            request->send(200);
        }
    });

    server.on("/delete", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (request->hasParam("timestamp")) {
            String ts = request->getParam("timestamp")->value();

            JsonDocument doc;
            File f = LittleFS.open(jsonPath, "r");
            deserializeJson(doc, f);
            f.close();

            JsonDocument newDoc;
            JsonArray newArr = newDoc.to<JsonArray>();

            for (JsonObject item : doc.as<JsonArray>()) {
                if (String(item["timestamp"].as<unsigned long>()) != ts)
                    newArr.add(item);
            }

            File wf = LittleFS.open(jsonPath, "w");
            serializeJson(newDoc, wf);
            wf.close();

            request->send(200);
        }
    });

    server.on("/clear", HTTP_POST, [](AsyncWebServerRequest *request) {
        LittleFS.remove(jsonPath);
        request->send(200);
    });

    server.on("/ir/status", HTTP_GET, [](AsyncWebServerRequest *request) {
        String status = (IrReceiver.decode()) ? "Receiving" : "Idle";
        request->send(200, "text/plain", "Receiving IR signals: " + status);
    });

    server.on("/ir/send", HTTP_ANY, [](AsyncWebServerRequest *request) {

    String name;

    if (request->hasParam("name", true)) {
        name = request->getParam("name", true)->value();  
    }
    else if (request->hasParam("name")) {
        name = request->getParam("name")->value();        
    }

    if (name.length() > 0) {
        sendByName(name);
        request->send(200, "text/plain", "Sent IR signal for: " + name);
    } else {
        request->send(400, "text/plain", "Missing 'name' parameter");
    }
    });

    server.on("/ir/list", HTTP_GET, [](AsyncWebServerRequest *request) {
        JsonDocument doc;
        File f = LittleFS.open(jsonPath, "r");
        deserializeJson(doc, f);
        f.close();

        String jsonResponse;
        serializeJson(doc, jsonResponse);
        request->send(200, "application/json", jsonResponse);
    });

    server.onNotFound([](AsyncWebServerRequest *request) {
        request->send(404, "text/plain", "Not Found");
    });

    server.begin();

    // readJSON(); // just for debugging.
}

void loop() {
    if (IrReceiver.decode()) {
        if (IrReceiver.decodedIRData.protocol != UNKNOWN) {
            saveIRToJSON(
                String(IrReceiver.getProtocolString()),
                "0x" + String(IrReceiver.decodedIRData.command, HEX),
                "0x" + String(IrReceiver.decodedIRData.address, HEX)
            );
        }
        IrReceiver.resume();
    }
}
