/*
ESP8266.CMS v1.4 <https://github.com/lukastautz/ESP8266.CMS>
Copyright (C) 2022 Lukas Tautz

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.

You can use ESP8266.CMS for free in your projects, you can also modify it BUT YOU ARE NOT ALLOWED TO DELETE THIS COMMENT!

siege test results: 5 concurrent users, 30 min, no articles, sitename esp-server:
  35100 requests
  0 failed requests
  260 ms average response time
  60 ms minimal response time
  610 ms maximal response time
*/


#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "config.h"

#define BLUE_LED 1
#define LED_ON digitalWrite(BLUE_LED, LOW)
#define LED_OFF digitalWrite(BLUE_LED, HIGH)
WiFiClient client;
WiFiServer wifiServer(PORT);
bool lBreak = false;
bool space = false;
bool qStart = false;
bool olderPage = false;
String req;
String url;
String query;
String post;
String currentLine;
String pn;
String Articles[MAX_ARTICLES * 2];
String result;
String httpRequest(String url, bool isAdmin);
unsigned short page;
unsigned short acC = 1;
unsigned long rS;
#ifdef STATUS_PAGE
unsigned long reqSBoot = 0;
#endif

unsigned char h2int(char c) {
    if (c >= '0' && c <= '9') {
        return (unsigned char)c - '0';
    }
    if (c >= 'a' && c <= 'f') {
        return (unsigned char)c - 'a' + 10;
    }
    if (c >= 'A' && c <= 'F') {
        return (unsigned char)c - 'A' + 10;
    }
    return 0;
}
String urlDecode(String str) {
    String encodedString;
    char c;
    char code0;
    char code1;
    for (unsigned int i = 0; i < str.length(); i++) {
        c = str.charAt(i);
        if (c == '+') {
            encodedString += ' ';
        } else if (c == '%') {
            i++;
            code0 = str.charAt(i);
            i++;
            code1 = str.charAt(i);
            c = (h2int(code0) << 4) | h2int(code1);
            encodedString += c;
        } else encodedString += c;
    }
    return encodedString;
}
String GET(String key) {
    bool isKey = false;
    String cur;
    String returnV;
    for (unsigned short l = 0; l < query.length(); l++) {
        if (query[l] != '&' && query[l] != '=' && isKey == false) {
            cur += query[l];
        } else if (isKey == true && query[l] == '&') {
            return urlDecode(returnV);
        } else if (isKey == true) {
            returnV += query[l];
        } else if (query[l] == '=') {
            if (cur == key) isKey = true;
            cur = "";
        } else if (query[l] == '&') {
            cur = "";
            isKey = false;
        }
    }
    return urlDecode(returnV);
}
String POST(String key) {
    bool isKey = false;
    String cur;
    String returnV;
    for (unsigned short l = 0; l < post.length(); l++) {
        if (post[l] != '&' && post[l] != '=' && isKey == false) {
            cur += post[l];
        } else if (isKey == true && post[l] == '&') {
            return urlDecode(returnV);
        } else if (isKey == true) {
            returnV += post[l];
        } else if (post[l] == '=') {
            if (cur == key) isKey = true;
            cur = "";
        } else if (post[l] == '&') {
            cur = "";
            isKey = false;
        }
    }
    return urlDecode(returnV);
}
void wifiConnect() {
    unsigned short networkCount = WiFi.scanNetworks();
    for (unsigned short i = 0; i < networkCount; i++) {
        if (WiFi.SSID(i) == WLAN_NAME) {
#ifdef WLAN_PASSWORD
            WiFi.begin(WLAN_NAME, WLAN_PASSWORD);
#else
            WiFi.begin(WLAN_NAME);
#endif
            WiFi.hostname(HOSTNAME);
            return;
        }
    }
}
void setup() {
    pinMode(BLUE_LED, OUTPUT);
    LED_ON;
    wifiConnect();
    while (WiFi.status() != WL_CONNECTED) delay(10);
    wifiServer.begin();
    LED_OFF;
}
void loop() {
    client = wifiServer.available();
    if (client) {
        LED_ON;
        rS = millis() + 2500;
        while (client.connected()) {
            if (rS < millis()) break;
            if (client.available()) {
                char c = client.read();
                if (c == 22) break; // SSL handshake
                if (lBreak == false) {
                    if (c == '\n' || c == '\r') lBreak = true;
                    else req += c;
                }
                if (c == '\n') {
                    if (currentLine.length() == 0) {
                        for (unsigned short u = 0; u < req.length() - 9; u++) {
                            if (req[u] == '?' && qStart == false) qStart = true;
                            else if (space == true && qStart == true) query += req[u];
                            else if (space == true) url += req[u];
                            else if (req[u] == ' ') space = true;
                        }
                        if (url[url.length() - 1] == '/' && url.length() != 1) {
                            url.remove(url.length() - 1);
                        }
                        if (client.remoteIP().toString() == ADMIN_IP && url == "/admin/save") {
                            while (client.connected()) {
                                int c = client.read();
                                if (c < 1) break;
                                post += (char)c;
                                delay(1); // Without this delay, big post data will be shortened
                            }
                            if (post[0] == '{') post = ""; // JSON Post isn't supported yet
							client.print(httpRequest(url, true));
                        } else client.print(httpRequest(url, false));
                        break;
                    } else {
                        currentLine = "";
                    }
                } else if (c != '\r') {
                    currentLine += c;
                }
            }
        }
        client.stop();
        req = "";
        url = "";
        query = "";
        post = "";
        currentLine = "";
        lBreak = false;
        space = false;
        qStart = false;
        LED_OFF;
    }
}

String httpRequest(String url, bool isAdmin) {
#ifdef STATUS_PAGE
    if (url == "/status") {
        result = F("HTTP/1.1 200 OK\nContent-Type: text/html; charset=UTF-8\n\n<!DOCTYPE html><html><head><meta charset=\"UTF-8\"><title>" SITE_NAME " - " LANG_STATUS "</title><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"><style>body{font-family:sans-serif;color:#505a62;font-size:1rem;line-height:1.3}hr{border:0;border-top:0.1rem solid #f4f5f6;margin:0.6rem 0}h1{font-weight:normal;letter-spacing:-0.073rem;margin-bottom:.9rem;margin-top:.15rem;line-height:1.2;font-size:2.4rem}</style></head><body><h1>" SITE_NAME " - " LANG_STATUS "</h1>");
        result += String(reqSBoot);
        result += F(" " LANG_REQUESTS "<br>" LANG_ONLINE " ");
        unsigned long hours = millis() / 3600000;
        unsigned short days = hours / 24;
        hours %= 24;
        if (days == 0) result += String(hours) + " " LANG_HOURS;
        else result += String(days) + " " LANG_DAYS " " + String(hours) + " " LANG_HOURS;
        return result + F("</body></html><!-- Generated with ESP8266-CMS -->");
    }
#ifdef FAVICON_ICO
    else if (url == "/favicon.ico") {
        return urlDecode(F("HTTP/1.1 200 OK\nContent-Type: image/x-icon\nCache-Control: public, max-age=2628000\n\n" FAVICON_ICO));
    }
#ifdef CUSTOM_PAGES
    else CUSTOM_PAGES
#endif
#endif
#endif
#ifdef FAVICON_ICO
    if (url == "/favicon.ico") {
        return urlDecode(F("HTTP/1.1 200 OK\nContent-Type: image/x-icon\nCache-Control: public, max-age=2628000\n\n" FAVICON_ICO));
    }
#ifdef CUSTOM_PAGES
    else CUSTOM_PAGES
#endif
#endif

    result = F("HTTP/1.1 200 OK\nContent-Type: text/html; charset=UTF-8\n\n<!DOCTYPE html><html><head><meta charset=\"UTF-8\"><title>" SITE_NAME "</title><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"><style>body{font-family:sans-serif;color:#505a62;font-size:1rem;line-height:1.3}a,a:visited{text-decoration:underline;color:#00f}a:focus,a:hover{text-decoration:none}article{border:1px solid #9da7af;padding:.4rem;margin-bottom:1rem;border-radius:5px;padding-bottom:.5rem}code,blockquote,pre{overflow:auto}pre{background:#f4f5f6;border-radius:0.1rem;font-size:1rem;border-left:0.3rem solid #d1d1d1;font-family:monospace;display:block;padding:0.2rem 0.5rem;white-space:pre}code{background:#f4f5f6;border-radius:0.1rem;font-size:1rem;padding:0.2rem 0.5rem;white-space:normal;font-family:monospace}hr{border:0;border-top:0.1rem solid #f4f5f6;margin:0.6rem 0}blockquote,dl,figure,ol,pre,ul{margin-bottom:0.55rem;margin-top:0.55rem}h1,h2,h3,h4{font-weight:normal;letter-spacing:-0.073rem;margin-bottom:.9rem;margin-top:.15rem;line-height:1.2}article h2:first-child{margin-bottom:0}h1{font-size:2.4rem}h2{font-size:1.7rem}h3{font-size:1.3rem}h4{font-size:1.1rem}img,video,iframe,audio{max-width:100%}</style></head><body><h1>" SITE_NAME "</h1>");
    pn = GET("page");
    if (pn == "") page = 1;
    else page = atoi(pn.c_str());
    if (Articles[(page * DISPLAY_ARTICLES * 2) + 1] != "") olderPage = true;
    else olderPage = false;
    if (isAdmin == true) {
        if (url == "/admin/edit") {
            unsigned short _id = atoi(GET("id").c_str());
            String title = Articles[_id];
            title.replace("\"", "&quot;");
            String text = Articles[_id + 1];
            text.replace("<", "&lt;");
            text.replace(">", "&gt;");
            result += F("<style>input,textarea{background:#fff;font-family:sans-serif;margin-top:6px;margin-bottom:6px;font-size:1rem;padding:7px;outline:0;border:1px solid #ccc;border-radius:4px;-webkit-transition:border-color .15s ease-in-out,box-shadow .15s ease-in-out;-moz-transition:border-color .15s ease-in-out,box-shadow .15s ease-in-out;-ms-transition:border-color .15s ease-in-out,box-shadow .15s ease-in-out;transition:border-color .15s ease-in-out,box-shadow .15s ease-in-out;width: 100%;width:-webkit-calc(100% - 20px);width:-moz-calc(100% - 20px);width:-ms-calc(100% - 20px);width:calc(100% - 20px)}input:focus,input:hover,textarea:focus,textarea:hover{border:1px solid #28a745;outline:0}textarea{height:66vh;font-family:monospace}input:focus,textarea:focus{-webkit-box-shadow:0 0 0 .21rem rgba(40,167,69,.5);box-shadow:0 0 0 .21rem rgba(40,167,69,.5)}input[type=submit]{cursor:pointer}</style><form action=\"/admin/save\" method=\"post\"><input type=\"hidden\" name=\"id\" value=\"");
            result += GET("id") + "\"><input type=\"text\" maxlength=\"200\" name=\"title\" value=\"" + title + "\" placeholder=\"" LANG_TITLE "\"><textarea maxlength=\"4300\" name=\"content\" placeholder=\"" LANG_CONTENT "\">" + text;
            result += F("</textarea><div style=\"text-align:center\"><input type=\"submit\" value=\"" LANG_SAVE "\"></div></form>");
        } else if (url == "/admin/save") {
            unsigned short _id = atoi(POST("id").c_str());
            Articles[_id] = POST("title");
            Articles[_id + 1] = POST("content");
            return F("HTTP/1.1 302 Found\nLocation: /\n\nSaved!");
        } else if (url == "/admin/new") {
            for (short id = acC; id > 0; id = id - 2) {
                Articles[id + 1] = Articles[id - 1];
                Articles[id + 2] = Articles[id];
            }
            if (acC < (MAX_ARTICLES * 2) - 1) acC = acC + 2;
            Articles[0] = LANG_NEW_ARTICLE;
            Articles[1] = "";
            return F("HTTP/1.1 302 Found\nLocation: /admin/edit?id=0\n\nCreated!");
        } else if (url == "/admin/delete") {
            int _id = atoi(GET("id").c_str());
            Articles[_id] = "";
            Articles[_id + 1] = "";
            if (Articles[_id + 2] != "") {
                for (unsigned short id = _id; id < acC; id = id + 2) {
                    if (Articles[id + 2] == "") break;
                    Articles[id] = Articles[id + 2];
                    Articles[id + 2] = "";
                    Articles[id + 1] = Articles[id + 3];
                    Articles[id + 3] = "";
                }
            }
            if (acC != 1) acC = acC - 2;
            return F("HTTP/1.1 302 Found\nLocation: /\n\nDeleted!");
        } else {
            for (unsigned short id = (page - 1) * DISPLAY_ARTICLES * 2; id < page * DISPLAY_ARTICLES * 2; id = id + 2) {
                if (Articles[id] == "") break;
                else result += "<article><h2>" + Articles[id] + "</h2>" + Articles[id + 1] + "<br><a href=\"/admin/edit?id=" + String(id) + "\">" LANG_EDIT "</a> <a href=\"/admin/delete?id=" + String(id) + "\">" LANG_DELETE "</a></article>";
            }
            if (olderPage == true) result += "<hr><div style=\"text-align:center\"><a href=\"/?page=" + String(page + 1) + "\">" LANG_BEFORE "</a></div>";
            result += F("<hr><a href=\"/admin/new\">" LANG_NEW_ARTICLE "</a>");
        }
    } else {
#ifdef STATUS_PAGE
        reqSBoot++;
#endif
        for (unsigned short id = (page - 1) * DISPLAY_ARTICLES * 2; id < page * DISPLAY_ARTICLES * 2; id = id + 2) {
            if (Articles[id] == "") break;
            else result += "<article><h2>" + Articles[id] + "</h2>" + Articles[id + 1] + "</article>";
        }
        if (olderPage == true) result += "<hr><div style=\"text-align:center\"><a href=\"/?page=" + String(page + 1) + "\">" LANG_BEFORE "</a></div>";
    }
    return result + F("</body></html><!-- Generated with ESP8266-CMS -->");
}