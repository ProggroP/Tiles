// Clay-Konfigurationsseite. Nutzersichtbare Texte englisch.
// messageKey muss Zeichen fuer Zeichen zu package.json passen, die Werte der
// Auswahllisten zu SRC_* und SCHEMES in tiles.c.
var SOURCES = [
  { "label": "Temperature",                          "value": "0" },
  { "label": "Heart rate (active minutes on Round 2)", "value": "1" },
  { "label": "Active minutes",                       "value": "2" },
  { "label": "Battery",                              "value": "3" },
  { "label": "Steps",                                "value": "4" },
  { "label": "Distance",                             "value": "5" },
  { "label": "Sleep",                                "value": "6" },
  { "label": "Week number",                          "value": "7" }
];

function tile(key, label, def) {
  return {
    "type": "select",
    "messageKey": key,
    "label": label,
    "defaultValue": def,
    "options": SOURCES
  };
}

module.exports = [
  {
    "type": "heading",
    "defaultValue": "Tiles"
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Colours"
      },
      {
        "type": "select",
        "messageKey": "SCHEME",
        "label": "Colour scheme",
        "defaultValue": "0",
        "options": [
          { "label": "Classic", "value": "0" },
          { "label": "Ocean",   "value": "1" },
          { "label": "Sunset",  "value": "2" },
          { "label": "Mono",    "value": "3" }
        ]
      }
    ]
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Tiles"
      },
      tile("TILE_TR", "Top right",    "0"),
      tile("TILE_MR", "Middle right", "1"),
      tile("TILE_BL", "Bottom left",  "3"),
      tile("TILE_BR", "Bottom right", "4")
    ]
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Weather"
      },
      {
        "type": "select",
        "messageKey": "TEMP_UNIT",
        "label": "Temperature unit",
        "defaultValue": "auto",
        "options": [
          { "label": "Automatic (phone region)", "value": "auto" },
          { "label": "Celsius",                  "value": "celsius" },
          { "label": "Fahrenheit",               "value": "fahrenheit" }
        ]
      },
      {
        "type": "text",
        "defaultValue": "Weather data by Open-Meteo.com, updated every 30 minutes using your phone's location."
      }
    ]
  },
  {
    "type": "submit",
    "defaultValue": "Save"
  }
];
