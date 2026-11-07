// PebbleKit JS: Clay-Einstellungen plus Temperatur von Open-Meteo.
// Die Uhr fragt alle 30 Minuten per REQUEST_WEATHER an (nur wenn eine Kachel
// das Wetter zeigt); beim Start und nach dem Speichern wird von hier aus
// selbst abgefragt.

var Clay = require('@rebble/clay');
var clayConfig = require('./config');

var clay = new Clay(clayConfig);

var WEATHER_URL = 'https://api.open-meteo.com/v1/forecast';
var REQUEST_TIMEOUT_MS = 15000;
var SEND_DELAY_MS = 1500;   // nach Clays eigener Nachricht, sonst "busy"

function getStoredSettings() {
    try {
        return JSON.parse(localStorage.getItem('clay-settings') || '{}');
    } catch (e) {
        return {};
    }
}

// Keine Schnittstelle zur Einheiten-Einstellung der Pebble-App, also die
// Region des Telefons auswerten -- wer damit falsch liegt, stellt es um.
var FAHRENHEIT_REGIONS = /-(US|BS|BZ|KY|FM|MH|PW|LR)\b/i;

function useFahrenheit() {
    var unit = getStoredSettings().TEMP_UNIT;
    if (unit === 'celsius')    return false;
    if (unit === 'fahrenheit') return true;
    var locale = (navigator && navigator.language) || '';
    return FAHRENHEIT_REGIONS.test(locale);
}

function fetchWeather(lat, lon) {
    var url = WEATHER_URL
            + '?latitude='  + lat.toFixed(4)
            + '&longitude=' + lon.toFixed(4)
            + '&current=temperature_2m'
            + '&temperature_unit=' + (useFahrenheit() ? 'fahrenheit' : 'celsius');

    var request = new XMLHttpRequest();
    request.open('GET', url, true);
    request.timeout = REQUEST_TIMEOUT_MS;
    request.onload = function() {
        if (request.status !== 200) {
            console.log('weather request failed: HTTP ' + request.status);
            return;
        }
        try {
            var current = (JSON.parse(request.responseText) || {}).current;
            if (!current || current.temperature_2m === undefined) return;
            Pebble.sendAppMessage({ 'TEMP': Math.round(current.temperature_2m) },
                null, function() { console.log('weather: send failed'); });
        } catch (e) {
            console.log('weather not readable: ' + e);
        }
    };
    request.onerror   = function() { console.log('weather: network error'); };
    request.ontimeout = function() { console.log('weather: timeout'); };
    request.send();
}

function refreshWeather() {
    navigator.geolocation.getCurrentPosition(
        function(pos) { fetchWeather(pos.coords.latitude, pos.coords.longitude); },
        function(err) { console.log('location unavailable: ' + err.message); },
        { enableHighAccuracy: false, maximumAge: 1800000, timeout: 15000 }
    );
}

Pebble.addEventListener('ready', function() {
    refreshWeather();
});

Pebble.addEventListener('appmessage', function(e) {
    if (e.payload && e.payload.REQUEST_WEATHER !== undefined) refreshWeather();
});

// Feuert nach Clays Handler; die Einheit kann sich geaendert haben
Pebble.addEventListener('webviewclosed', function(e) {
    if (e && e.response && e.response !== 'CANCELLED') {
        setTimeout(refreshWeather, SEND_DELAY_MS);
    }
});
