
// ****************************************************************
// Sketch Esp8266 Filesystem Manager spezifisch sortiert Modular(Tab)
// created: Jens Fleischer, 2020-06-08
// last mod: Jens Fleischer, 2020-12-19
// For more information visit: https://fipsok.de
// ****************************************************************
// Hardware: Esp8266
// Software: Esp8266 Arduino Core 2.7.0 - 3.0.2
// Geprüft: von 1MB bis 2MB Flash
// Getestet auf: Nodemcu
/******************************************************************
  Copyright (c) 2020 Jens Fleischer. All rights reserved.

  This file is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  This file is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.
*******************************************************************/
// Diese Version von LittleFS sollte als Tab eingebunden werden.
// #include <LittleFS.h> #include <ESP8266WebServer.h> müssen im Haupttab aufgerufen werden
// Die Funktionalität des ESP8266 Webservers ist erforderlich.
// "server.onNotFound()" darf nicht im Setup des ESP8266 Webserver stehen.
// Die Funktion "setupFS();" muss im Setup aufgerufen werden.
/**************************************************************************************/

#include <list>
#include <tuple>

const char WARNING[] PROGMEM = R"(<h2>Der Sketch wurde mit "FS:none" kompilliert!)";
const char HELPER[] PROGMEM = R"(<form method="POST" action="/upload" enctype="multipart/form-data">
<input type="file" name="[]" multiple><button>Upload</button></form>Lade die fs.html hoch.)";

void setupFS() {                                                                       // Funktionsaufruf "setupFS();" muss im Setup eingebunden werden
  LittleFS.begin();
  server.on("/format", formatFS);
  server.on("/upload", HTTP_POST, sendResponce, handleUpload);
  server.onNotFound([]() {
    if (!handleFile(server.urlDecode(server.uri())))
      server.send(404, "text/plain", "FileNotFound");
  });
}

bool handleList() {                                                                    // Senden aller Daten an den Client
  FSInfo fs_info;  LittleFS.info(fs_info);                                             // Füllt FSInfo Struktur mit Informationen über das Dateisystem
  Dir dir = LittleFS.openDir("/");
  using namespace std;
  using records = tuple<String, String, int>;
  list<records> dirList;
  while (dir.next()) {                                                                 // Ordner und Dateien zur Liste hinzufügen
    if (dir.isDirectory()) {
      uint8_t ran {0};
      Dir fold = LittleFS.openDir(dir.fileName());
      while (fold.next())  {
        ran++;
        dirList.emplace_back(dir.fileName(), fold.fileName(), fold.fileSize());
      }
      if (!ran) dirList.emplace_back(dir.fileName(), "", 0);
    }
    else {
      dirList.emplace_back("", dir.fileName(), dir.fileSize());
    }
  }
  dirList.sort([](const records & f, const records & l) {                              // Dateien sortieren
    if (server.arg(0) == "1") {
      return get<2>(f) > get<2>(l);
    } else {
      for (uint8_t i = 0; i < 31; i++) {
        if (tolower(get<1>(f)[i]) < tolower(get<1>(l)[i])) return true;
        else if (tolower(get<1>(f)[i]) > tolower(get<1>(l)[i])) return false;
      }
      return false;
    }
  });
  dirList.sort([](const records & f, const records & l) {                              // Ordner sortieren
    if (get<0>(f)[0] != 0x00 || get<0>(l)[0] != 0x00) {
      for (uint8_t i = 0; i < 31; i++) {
          if (tolower(get<0>(f)[i]) < tolower(get<0>(l)[i])) return true;
          else if (tolower(get<0>(f)[i]) > tolower(get<0>(l)[i])) return false;
      }
    }
    return false;
  });
  String temp = "[";
  for (auto& t : dirList) {
    if (temp != "[") temp += ',';
    temp += "{\"folder\":\"" + get<0>(t) + "\",\"name\":\"" + get<1>(t) + "\",\"size\":\"" + formatBytes(get<2>(t)) + "\"}";
  }
  temp += ",{\"usedBytes\":\"" + formatBytes(fs_info.usedBytes) +                      // Berechnet den verwendeten Speicherplatz
          "\",\"totalBytes\":\"" + formatBytes(fs_info.totalBytes) +                   // Zeigt die Größe des Speichers
          "\",\"freeBytes\":\"" + (fs_info.totalBytes - fs_info.usedBytes) + "\"}]";   // Berechnet den freien Speicherplatz
  server.send(200, "application/json", temp);
  return true;
}

void deleteRecursive(const String &path) {
  if (LittleFS.remove(path)) {
    LittleFS.open(path.substring(0, path.lastIndexOf('/')) + "/", "w");
    return;
  }
  Dir dir = LittleFS.openDir(path);
  while (dir.next()) {
    deleteRecursive(path + '/' + dir.fileName());
  }
  LittleFS.rmdir(path);
}

bool handleFile(String &&path) {
  if (server.hasArg("new")) {
    String folderName {server.arg("new")};
    for (auto& c : {34, 37, 38, 47, 58, 59, 92}) for (auto& e : folderName) if (e == c) e = 95;    // Ersetzen der nicht erlaubten Zeichen
    LittleFS.mkdir(folderName);
  }
  if (server.hasArg("sort")) return handleList();
  if (server.hasArg("delete")) {
    deleteRecursive(server.arg("delete"));
    sendResponce();
    return true;
  }
  if (!LittleFS.exists("fs.html")) server.send(200, "text/html", LittleFS.begin() ? HELPER : WARNING);     // ermöglicht das hochladen der fs.html
  if (path.endsWith("/")) path += "index.html";
  if (path == "/spiffs.html") sendResponce(); // Vorrübergehend für den Admin Tab
  return LittleFS.exists(path) ? ({File f = LittleFS.open(path, "r"); server.streamFile(f, mime::getContentType(path)); f.close(); true;}) : false;
}

void handleUpload() {                                                                  // Dateien ins Filesystem schreiben
  static File fsUploadFile;
  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    if (upload.filename.length() > 31) {  // Dateinamen kürzen
      upload.filename = upload.filename.substring(upload.filename.length() - 31, upload.filename.length());
    }
    printf(PSTR("handleFileUpload Name: /%s\n"), upload.filename.c_str());
    fsUploadFile = LittleFS.open(server.arg(0) + "/" + server.urlDecode(upload.filename), "w");
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    printf(PSTR("handleFileUpload Data: %u\n"), upload.currentSize);
    fsUploadFile.write(upload.buf, upload.currentSize);
  } else if (upload.status == UPLOAD_FILE_END) {
    printf(PSTR("handleFileUpload Size: %u\n"), upload.totalSize);
    fsUploadFile.close();
  }
}

void formatFS() {                                                                      // Formatiert das Filesystem
  LittleFS.format();
  sendResponce();
}

void sendResponce() {
  server.sendHeader("Location", "fs.html");
  server.send(303, "message/http");
}

const String formatBytes(size_t const& bytes) {                                        // lesbare Anzeige der Speichergrößen
  return bytes < 1024 ? static_cast<String>(bytes) + " Byte" : bytes < 1048576 ? static_cast<String>(bytes / 1024.0) + " KB" : static_cast<String>(bytes / 1048576.0) + " MB";
}
