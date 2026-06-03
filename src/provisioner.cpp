#ifdef HAS_CAPTIVE_PORTAL

#include "provisioner.h"
#include <WiFi.h>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>

#define PROV_AP_SSID  "ISS-Tracker-Setup"
#define PROV_NVS_NS   "iss_config"
#define PROV_DNS_PORT 53

static DNSServer      dnsServer;
static AsyncWebServer webServer(80);
static bool           shouldReboot = false;

// ── NVS ────────────────────────────────────────────────────────────────────────

bool hasValidConfig() {
    Preferences p;
    p.begin(PROV_NVS_NS, true);
    bool ok = p.getBool("configured", false);
    p.end();
    return ok;
}

// ── Config variable references (defined in main.cpp via config.h) ──────────────

extern String   WIFI_SSID;
extern String   WIFI_PASSWORD;
extern int      satelliteCatalogueNumber;
extern double   OBSERVER_LATITUDE;
extern double   OBSERVER_LONGITUDE;
extern double   OBSERVER_ALTITUDE;
extern int      beepsNotificationBeforeAOSandLOS;
extern bool     notificationAtTCA;
extern bool     display7DigisStyleClock;
extern uint16_t clockDigitsColor;
extern uint16_t TFT_GHOST_SEGMENT_COLOR;
extern bool     DISPLAY_ISS_CREW;
extern int      autoPageChange;
extern int      durationBetweenPageChanges;
extern int      tftRotation;

void loadConfig() {
    Preferences p;
    p.begin(PROV_NVS_NS, true);

    WIFI_SSID                        = p.getString("wifi_ssid",   WIFI_SSID);
    WIFI_PASSWORD                    = p.getString("wifi_pass",   WIFI_PASSWORD);
    OBSERVER_LATITUDE                = p.getDouble("obs_lat",     OBSERVER_LATITUDE);
    OBSERVER_LONGITUDE               = p.getDouble("obs_lon",     OBSERVER_LONGITUDE);
    OBSERVER_ALTITUDE                = p.getDouble("obs_alt",     OBSERVER_ALTITUDE);
    satelliteCatalogueNumber         = p.getUInt  ("sat_num",     satelliteCatalogueNumber);
    beepsNotificationBeforeAOSandLOS = p.getUInt  ("beep_aos",    beepsNotificationBeforeAOSandLOS);
    notificationAtTCA                = p.getBool  ("beep_tca",    notificationAtTCA);
    display7DigisStyleClock          = p.getBool  ("seg_clock",   display7DigisStyleClock);
    DISPLAY_ISS_CREW                 = p.getBool  ("iss_crew",    DISPLAY_ISS_CREW);
    clockDigitsColor                 = p.getUInt  ("clock_color", clockDigitsColor);
    TFT_GHOST_SEGMENT_COLOR          = p.getUInt  ("ghost_color", TFT_GHOST_SEGMENT_COLOR);
    autoPageChange                   = p.getBool  ("auto_page",   autoPageChange);
    durationBetweenPageChanges       = p.getUInt  ("page_dur",    durationBetweenPageChanges);
    tftRotation                      = p.getInt   ("tft_rot",     tftRotation);

    p.end();

    Serial.printf("[loadConfig] WiFi: %s  Sat: %d  Lat: %.4f  Lon: %.4f\n",
        WIFI_SSID.c_str(), satelliteCatalogueNumber,
        OBSERVER_LATITUDE, OBSERVER_LONGITUDE);
}

// ── Color helpers ──────────────────────────────────────────────────────────────

static uint16_t hexToRgb565(const String &hex) {
    String h = hex.startsWith("#") ? hex.substring(1) : hex;
    if (h.length() != 6) return 0;
    uint8_t r = strtol(h.substring(0, 2).c_str(), nullptr, 16);
    uint8_t g = strtol(h.substring(2, 4).c_str(), nullptr, 16);
    uint8_t b = strtol(h.substring(4, 6).c_str(), nullptr, 16);
    return ((uint16_t)(r >> 3) << 11) | ((uint16_t)(g >> 2) << 5) | (b >> 3);
}

// ── HTML pages ─────────────────────────────────────────────────────────────────

static const char SETUP_PAGE[] PROGMEM = R"HTMLPAGE(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8"/>
  <meta name="viewport" content="width=device-width, initial-scale=1"/>
  <title>ISS Tracker Setup</title>
  <style>
    :root { --accent:#1a73e8; }
    *     { box-sizing:border-box; margin:0; padding:0; }
    body  { font-family:sans-serif; background:#f0f4f8; padding:16px; color:#222; }
    h1    { color:var(--accent); font-size:1.3rem; margin-bottom:4px; }
    .sub  { color:#666; font-size:.85rem; margin-bottom:16px; }
    .card { background:#fff; border-radius:10px; padding:16px; margin-bottom:14px;
            box-shadow:0 1px 4px rgba(0,0,0,.1); }
    .card h2 { font-size:.95rem; color:var(--accent); margin-bottom:10px;
               border-bottom:1px solid #eee; padding-bottom:6px; }
    label { display:block; font-size:.8rem; color:#555; margin-top:10px; margin-bottom:3px; }
    input[type=text], input[type=password], input[type=number], select {
      width:100%; padding:9px 10px; border:1px solid #ccc; border-radius:6px;
      font-size:.95rem; background:#fafafa;
    }
    .row   { display:flex; gap:10px; }
    .row>div { flex:1; min-width:0; }
    .chk   { display:flex; align-items:center; gap:8px; margin-top:10px; }
    .chk label { margin:0; color:#333; font-size:.9rem; }
    input[type=checkbox] { width:18px; height:18px; accent-color:var(--accent); flex-shrink:0; }
    input[type=color]    { width:100%; height:38px; padding:2px 4px;
                           border:1px solid #ccc; border-radius:6px; cursor:pointer; }
    .pw   { position:relative; }
    .pw input { padding-right:60px; }
    .pw button { position:absolute; right:8px; top:50%; transform:translateY(-50%);
                 background:none; border:none; color:var(--accent); font-size:.8rem; cursor:pointer; }
    .hint { font-size:.75rem; color:#999; margin-top:4px; }
    .save { display:block; width:100%; padding:14px; background:var(--accent); color:#fff;
            border:none; border-radius:8px; font-size:1.1rem; font-weight:bold; cursor:pointer;
            margin-top:4px; }
    .save:active { opacity:.8; }
  </style>
</head>
<body>
  <h1>ISS Tracker &mdash; Setup</h1>
  <p class="sub">Fill in your details and tap Save. The device will reboot and connect automatically.</p>

  <form method="POST" action="/save">

    <div class="card">
      <h2>WiFi</h2>
      <label>Network name (SSID)</label>
      <input type="text" name="wifi_ssid" placeholder="Your WiFi network" required/>
      <label>Password</label>
      <div class="pw">
        <input type="password" name="wifi_pass" id="pw" placeholder="Your WiFi password"/>
        <button type="button" onclick="t()">Show</button>
      </div>
    </div>

    <div class="card">
      <h2>Your Location</h2>
      <div class="row">
        <div>
          <label>Latitude</label>
          <input type="number" name="obs_lat" value="46.4666" step="0.0001" min="-90"  max="90"  required/>
        </div>
        <div>
          <label>Longitude</label>
          <input type="number" name="obs_lon" value="6.8615"  step="0.0001" min="-180" max="180" required/>
        </div>
      </div>
      <label>Altitude (metres)</label>
      <input type="number" name="obs_alt" value="500" min="0" max="9000"/>
      <p class="hint">Tip: right-click your location on Google Maps to copy coordinates.</p>
    </div>

    <div class="card">
      <h2>Satellite</h2>
      <label>Select satellite to track</label>
      <select name="sat_num">
        <optgroup label="Space Station">
          <option value="25544" selected>ISS &mdash; International Space Station</option>
        </optgroup>
        <optgroup label="Weather">
          <option value="25338">NOAA 15</option>
          <option value="28654">NOAA 18</option>
          <option value="33591">NOAA 19</option>
          <option value="29155">METOP-A</option>
          <option value="38771">METOP-B</option>
          <option value="43689">METOP-C</option>
          <option value="37849">Suomi NPP</option>
          <option value="40940">Fengyun 3D</option>
          <option value="41866">GOES 16</option>
          <option value="43226">GOES 17</option>
          <option value="46984">Sentinel-6</option>
        </optgroup>
        <optgroup label="HAM Radio">
          <option value="43017">AO-91 (AMSAT Fox-1B)</option>
          <option value="43137">AO-92 (AMSAT Fox-1D)</option>
          <option value="7530">OSCAR 7 (AO-7)</option>
          <option value="43770">CAS-4A (LeoSat-2)</option>
          <option value="43771">CAS-4B (LeoSat-3)</option>
          <option value="39444">FUNcube-1 (AO-73)</option>
          <option value="27607">SO-50 (SaudiSat-1C)</option>
          <option value="40908">LilacSat-2 (CAS-3H)</option>
          <option value="24278">FO-29 (JAS-2)</option>
          <option value="39445">EO-79 (Nayif-1)</option>
          <option value="41168">AO-85 (AMSAT Fox-1A)</option>
          <option value="43678">CAS-6 (XW-4)</option>
          <option value="43743">Diwata-2</option>
          <option value="28650">HAMSAT (VO-52)</option>
          <option value="43786">Tevel-1</option>
          <option value="43803">Tevel-2</option>
          <option value="44832">QO-100 (Es&apos;hail-2, GEO)</option>
        </optgroup>
        <optgroup label="Other">
          <option value="20580">Hubble Space Telescope</option>
        </optgroup>
      </select>
    </div>

    <div class="card">
      <h2>Notifications</h2>
      <label>Beep before AOS &amp; LOS (seconds &mdash; 0 = off)</label>
      <input type="number" name="beep_aos" value="15" min="0" max="300"/>
      <div class="chk">
        <input type="checkbox" name="beep_tca" id="beep_tca" checked/>
        <label for="beep_tca">Beep at TCA (closest approach)</label>
      </div>
    </div>

    <div class="card">
      <h2>Display</h2>
      <div class="chk">
        <input type="checkbox" name="seg_clock" id="seg_clock" checked/>
        <label for="seg_clock">7-segment style clock</label>
      </div>
      <div class="chk">
        <input type="checkbox" name="iss_crew" id="iss_crew" checked/>
        <label for="iss_crew">Show ISS crew names</label>
      </div>
      <div class="row" style="margin-top:12px;">
        <div>
          <label>Clock digits color</label>
          <input type="color" name="clock_color" value="#F8D400"/>
        </div>
        <div>
          <label>Ghost segment color</label>
          <input type="color" name="ghost_color" value="#484848"/>
        </div>
      </div>
    </div>

    <div class="card">
      <h2>Behaviour</h2>
      <div class="chk">
        <input type="checkbox" name="auto_page" id="auto_page"/>
        <label for="auto_page">Auto page change</label>
      </div>
      <label>Page change interval (seconds)</label>
      <input type="number" name="page_dur" value="6" min="1" max="60"/>
    </div>

    <button type="submit" class="save">Save &amp; Reboot</button>
  </form>

  <script>
    function t() {
      var e = document.getElementById('pw');
      var b = e.nextElementSibling;
      e.type = e.type === 'password' ? 'text' : 'password';
      b.textContent = e.type === 'password' ? 'Show' : 'Hide';
    }
  </script>
</body>
</html>
)HTMLPAGE";

static const char REBOOT_PAGE[] PROGMEM = R"HTMLPAGE(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8"/>
  <meta name="viewport" content="width=device-width, initial-scale=1"/>
  <title>ISS Tracker Setup</title>
  <style>
    body { font-family:sans-serif; max-width:480px; margin:40px auto; padding:0 20px; color:#222; }
    h1   { color:#2e7d32; }
    p    { color:#555; margin-top:8px; }
  </style>
</head>
<body>
  <h1>&#10003; Saved!</h1>
  <p>Device is rebooting and will connect to your WiFi network.</p>
  <p>You can close this page.</p>
</body>
</html>
)HTMLPAGE";

// ── TFT display ────────────────────────────────────────────────────────────────

static void showSetupScreen(TFT_eSPI &tft) {
    tft.fillScreen(TFT_BLACK);

    // Header bar
    uint16_t accent = tft.color565(0x1a, 0x73, 0xe8);
    tft.fillRect(0, 0, tft.width(), 44, accent);
    tft.setTextFont(4);
    tft.setTextColor(TFT_WHITE, accent);
    String hdr = "ISS TRACKER  -  SETUP MODE";
    tft.setCursor((tft.width() - tft.textWidth(hdr)) / 2, 10);
    tft.print(hdr);

    // Helper: center a string at y
    auto cprint = [&](const String &s, uint16_t col, int16_t y, uint8_t font) {
        tft.setTextFont(font);
        tft.setTextColor(col, TFT_BLACK);
        tft.setCursor((tft.width() - tft.textWidth(s)) / 2, y);
        tft.print(s);
    };

    // Step 1
    cprint("1.  Connect your phone or PC to WiFi:", TFT_WHITE, 62,  2);
    cprint(PROV_AP_SSID,                           TFT_CYAN,  84,  4);

    // Step 2
    cprint("2.  Open your browser and go to:",     TFT_WHITE, 132, 2);
    cprint("192.168.4.1",                          TFT_CYAN,  154, 4);

    // Hints
    cprint("(browser may open automatically)",     TFT_LIGHTGREY, 212, 2);
    cprint("Device will reboot when done.",        TFT_LIGHTGREY, 232, 2);
}

// ── Captive portal ─────────────────────────────────────────────────────────────

void startProvisioner(TFT_eSPI &tft) {
    showSetupScreen(tft);

    WiFi.mode(WIFI_AP);
    WiFi.softAP(PROV_AP_SSID);
    delay(200);

    dnsServer.start(PROV_DNS_PORT, "*", WiFi.softAPIP());

    webServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/html", SETUP_PAGE);
    });

    webServer.on("/save", HTTP_POST, [](AsyncWebServerRequest *request) {
        Preferences p;
        p.begin(PROV_NVS_NS, false);

        auto str = [&](const char *key, const char *def) -> String {
            return request->hasParam(key, true) ? request->getParam(key, true)->value() : def;
        };
        auto num = [&](const char *key, int def) -> int {
            return request->hasParam(key, true) ? request->getParam(key, true)->value().toInt() : def;
        };
        auto has = [&](const char *key) -> bool {
            return request->hasParam(key, true);
        };

        p.putString("wifi_ssid",   str("wifi_ssid",   ""));
        p.putString("wifi_pass",   str("wifi_pass",   ""));
        p.putDouble("obs_lat",     str("obs_lat",     "0").toDouble());
        p.putDouble("obs_lon",     str("obs_lon",     "0").toDouble());
        p.putDouble("obs_alt",     str("obs_alt",     "0").toDouble());
        p.putUInt  ("sat_num",     num("sat_num",     25544));
        p.putUInt  ("beep_aos",    num("beep_aos",    15));
        p.putBool  ("beep_tca",    has("beep_tca"));
        p.putBool  ("seg_clock",   has("seg_clock"));
        p.putBool  ("iss_crew",    has("iss_crew"));
        p.putUInt  ("clock_color", hexToRgb565(str("clock_color", "#F8D400")));
        p.putUInt  ("ghost_color", hexToRgb565(str("ghost_color", "#484848")));
        p.putBool  ("auto_page",   has("auto_page"));
        p.putUInt  ("page_dur",    num("page_dur",    6) * 1000);
        p.putBool  ("configured",  true);

        p.end();

        Serial.println("[provisioner] config saved:");
        Serial.printf("  ssid=%s lat=%.4f lon=%.4f sat=%d\n",
            str("wifi_ssid","").c_str(),
            str("obs_lat","0").toDouble(),
            str("obs_lon","0").toDouble(),
            num("sat_num", 25544));

        request->send(200, "text/html", REBOOT_PAGE);
        shouldReboot = true;
    });

    // Captive portal detection redirects (Android, iOS, Windows)
    auto redir = [](AsyncWebServerRequest *r) { r->redirect("http://192.168.4.1/"); };
    webServer.on("/generate_204",        HTTP_GET, redir);
    webServer.on("/hotspot-detect.html", HTTP_GET, redir);
    webServer.on("/ncsi.txt",            HTTP_GET, redir);
    webServer.on("/connecttest.txt",     HTTP_GET, redir);
    webServer.onNotFound([](AsyncWebServerRequest *r) { r->redirect("http://192.168.4.1/"); });

    webServer.begin();

    Serial.printf("[provisioner] AP: %s  IP: %s\n", PROV_AP_SSID, WiFi.softAPIP().toString().c_str());

    while (!shouldReboot) {
        dnsServer.processNextRequest();
        delay(10);
    }

    delay(500);
    ESP.restart();
}

#endif // HAS_CAPTIVE_PORTAL
