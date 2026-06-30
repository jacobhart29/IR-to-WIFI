#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <IRremote.hpp>

const char* ssid = "spiderlan";
const char* password = "Aaron1028!";
#define IR_RECEIVE_PIN 2

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

void setup() {
    Serial.begin(115200);

    if (!LittleFS.begin(true)) return;

    if (!LittleFS.exists(jsonPath)) {
        File f = LittleFS.open(jsonPath, "w");
        f.print("[]");
        f.close();
    }

    IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) delay(500);
    Serial.println(WiFi.localIP());

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
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

    server.begin();
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