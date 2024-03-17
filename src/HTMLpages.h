// HTML root page. Redirect to /browse
static const char PAGE_ROOT[] PROGMEM = R"(
{
  "uri": "/",
  "title": "Tubular bells",
  "menu": false,
  "response": false,
  "element": [
    {
      "name": "caption",
      "type": "ACText",
      "value": "<h2>Tubular Bells</h2>"
    }
  ]
}
)";

// HTML Page to list, play, upload and delete files
static const char PAGE_BROWSE[] PROGMEM = R"(
{
  "uri": "/browse",
  "title": "Browse",
  "menu": true,
  "element": [
    {
      "name": "server_url",
      "type": "ACText",
      "value": "server_url",
      "posterior": "br"
    },
    {
      "name": "object",
      "type": "ACElement"
    },
    {
      "name": "play",
      "type": "ACSubmit",
      "value": "PLAY",
      "uri": "/play"
    },
    {
      "name": "selected_codefile",
      "type": "ACInput",
      "value": "010101",
      "posterior": "none"
    },
    {
      "name": "select_track",
      "type": "ACSelect",
      "label": "select a track",
      "option": [
        "-1","0","1","2","3","4","5","6","7","8","9"
      ],
      "selected": 1
   },
   {
      "name": "select_volume",
      "type": "ACSelect",
      "label": "set the audio volume",
      "option": [
        "0","1","2","3","4","5","6","7","8","9","10",
        "11","12","13","14","15","16","17","18","19","20"
      ],
      "selected": 5
   }, 
   {
      "name": "mp3_smf_delay_ms",
      "type": "ACInput",
      "value": "0",
      "label" : "mp3 to mid delay ms"
    },
   {
      "name": "embed_list",
      "type": "ACElement"
   }
  ]
}
)";

// HTML Page to list, upload and delete files in SPIFFS
static const char PAGE_SPIFFS[] PROGMEM = R"(
{
  "uri": "/spiffs",
  "title": "FileSys",
  "menu": true,
  "element": [
    {
      "name": "object",
      "type": "ACElement"
    }, 
    {
      "name": "server_url",
      "label": "Server URL", 
      "type": "ACInput",
      "value": "http://192.168.1.232:5000",
      "posterior": "none"
    },
    {
      "name": "save",
      "type": "ACSubmit",
      "value": "SAVE",
      "uri": "/save_server_url",
      "posterior": "br"
    },
    {
      "name": "delete",
      "type": "ACSubmit",
      "value": "DELETE",
      "uri": "/delete"
    },
    {
      "name": "upload",
      "type": "ACSubmit",
      "value": "UPLOAD FILE:",
      "uri": "/spiffs"
    },
    {
      "name": "upload_file",
      "type": "ACFile",
      "label": "",
      "store": "fs",
      "posterior": "br"
    },
    {
      "name": "radio",
      "type": "ACRadio",
      "value": [
      ],
      "label": "",
      "arrange": "vertical",
      "posterior": "br"
    }, 
    {
      "name": "status",
      "type": "ACText",
      "value": "status"
    }
  ]
}
)";

// HTML Page to delete a file
static const char PAGE_SAVE_SERVER_URL[] PROGMEM = R"(
{
  "uri": "/save_server_url",
  "title": "save_server_url",
  "menu": false,
  "response": false,
  "element": [
    {
      "name": "caption",
      "type": "ACText",
      "value": "<h2>save server_url</h2>"
    }
  ]
}
)";

// HTML Page to config bells' servo motors
static const char PAGE_BELLS[] PROGMEM = R"(
{
  "uri": "/bells",
  "title": "Bells",
  "menu": true,
  "element": [
    {
      "name": "select_bell",
      "type": "ACSelect",
      "label": "Select a bell",
      "option": [
        "0",
        "1",
        "2",
        "3",
        "4",
        "5"
      ],
      "selected": 0
    },
    {
      "name": "pin",
      "type": "ACInput",
      "value": "0",
      "label": "ESP32 GPIO pin"
    },
    {
      "name": "note",
      "type": "ACInput",
      "value": "77",
      "label": "Note"
    },
    {
      "name": "target",
      "type": "ACInput",
      "value": "60",
      "label": "Servo Target Position",
      "posterior": "none"
    },
    {
      "name": "test_bell_target",
      "type": "ACSubmit",
      "value": "TARGET_TEST",
      "uri": "/test_bell_target",
      "posterior": "br"
    },
    {
      "name": "target_time",
      "type": "ACInput",
      "value": "200",
      "label": "target time in msecs"
    },
    {
      "name": "rest",
      "type": "ACInput",
      "value": "60",
      "label": "rest",
      "posterior": "none"
    },
    {
      "name": "test_bell_rest",
      "type": "ACSubmit",
      "value": "REST_TEST",
      "uri": "/test_bell_rest",
      "posterior": "br"
    },
    {
      "name": "rest_time",
      "type": "ACInput",
      "value": "200",
      "label": "rest time in msecs",
      "posterior": "br"
    },
    {
      "name": "test_bell",
      "type": "ACSubmit",
      "value": "BELL_TEST",
      "uri": "/test_bell",
      "posterior": "br"
    },
    {
      "name": "save",
      "type": "ACSubmit",
      "value": "SAVE_BELL",
      "uri": "/save_bell"
    },
    {
      "name": "status",
      "type": "ACText",
      "value": "status"
    },
    {
      "name": "object",
      "type": "ACElement"
    }
  ]
}
)";

// HTML Page to test a bell by the /bells page
static const char TEST_BELL[] PROGMEM = R"(
{
  "uri": "/test_bell",
  "title": "test bell",
  "menu": false,
  "response": false,
  "element": [
  ]
}
)";

// HTML Page to test a bell positioning the mallet to the rest position by the /bells page
static const char TEST_BELL_REST[] PROGMEM = R"(
{
  "uri": "/test_bell_rest",
  "title": "test bell rest",
  "menu": false,
  "response": false,
  "element": [
  ]
}
)";

// HTML Page to test a bell positioning the mallet to the target position by the /bells page
static const char TEST_BELL_TARGET[] PROGMEM = R"(
{
  "uri": "/test_bell_target",
  "title": "test bell target",
  "menu": false,
  "response": false,
  "element": [
  ]
}
)";


// HTML Page to save the bells' configuration by the /bells page
static const char SAVE_BELL[] PROGMEM = R"(
{
  "uri": "/save_bell",
  "title": "save bell",
  "menu": false,
  "response": false,
  "element": [
  ]
}
)";


// HTML Page to play a mid file
static const char PAGE_PLAY[] PROGMEM = R"(
{
  "uri": "/play",
  "title": "play",
  "menu": false,
  "response": false,
  "element": [
    {
      "name": "caption",
      "type": "ACText",
      "value": "<h2>play page</h2>"
    }
  ]
}
)";

// HTML Page to delete a file
static const char PAGE_DELETE[] PROGMEM = R"(
{
  "uri": "/delete",
  "title": "delete",
  "menu": false,
  "response": false,
  "element": [
    {
      "name": "caption",
      "type": "ACText",
      "value": "<h2>delete page</h2>"
    }
  ]
}
)";

// HTML Page to download the configuration of a single bell
static const char PAGE_DOWNLOAD[] PROGMEM = R"(
{
  "uri": "/download",
  "title": "download a single bell configuration",
  "menu": false,
  "response": false,
  "element": [
  ]
}
)";

