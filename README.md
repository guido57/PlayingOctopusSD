# PlayingOctopus
 
## Overview 

I got the idea watching the traditional wind-chimes and guessing how they could be played automatically.
Here you'll find all the software necessary to playing Octopus, while the hardware project is here: https://hackaday.io/project/195138-playing-octopus

![](https://github.com/guido57/PlayingOctopus/blob/main/docs/Octopus.png)

The main distinctive capabilities of this building are:
1) It can play 1 selectable track of any MIDI file by the 6 tubular bells
2) the whole tune (all the MIDI tracks) is played by the loudspeaker
3) the software, by a web interface, allows to:
   * search the MIDI file from thousands of songs
   * select the track to be played by the octopus
   * tune the mallet movements (target, rest, speed)

## Software Architecture

![](https://github.com/guido57/PlayingOctopus/blob/main/docs/BlockDiagram.png)

## Octopus web server

The only way to use the Playing Octopus is connecting to its web server.

It is based on the [Autoconnect library by Hieromon](https://github.com/Hieromon/AutoConnect)
Here you are the most distinctive pages.

### Browse

![](https://github.com/guido57/PlayingOctopus/blob/main/docs/octopus-browse.png)

* SEARCH the Internet for all the mid files accomplishing the query. The words must be separated by a +   e.g. parker+charlie

  The search can be slow!

* Select a song. Then the song can be listened (on the client browser) by the embedded audio player

* PLAY run the Octopus to play the song (mp3) on its loudspeaker and the selected track bye the tubular bells

* mp3 to mid delay ms is for synchronizing the mp3 to the tubular bells

### Bells

![](https://github.com/guido57/PlayingOctopus/blob/main/docs/octopus-bells.png)

The servo motors moving the mallets must be carefully tuned. For each of the six bells the following parameters must be tuned.

* ESP32 GPIO pin: the hardware pin where the servo is connected to
* Note: the note played by that bell (optional)
* Servo Target Position: the position (in degrees) that the mallet must reach to hit the bell
* target time in msecs: the time to stay on the target position
* rest (idle): the position (in degrees) that the mallet must reach after hitting ther bell 
* rest time in msecs: the time to stay on the rest position
* BELL_TEST: to test the parameters set
* SAVE_TEST: to save the parameters

### FileSys

![](https://github.com/guido57/PlayingOctopus/blob/main/docs/octopus-filesys.png)

it can be used to set the Octopus Server URL. After running the python flask server on a shell, the program outputs its URL:
```
* Serving Flask app 'OctopusServer'
 * Debug mode: on
WARNING: This is a development server. Do not use it in a production deployment. Use a production WSGI server instead.
 * Running on all addresses (0.0.0.0)
 * Running on http://127.0.0.1:5000
 * Running on http://192.168.1.232:5000
Press CTRL+C to quit
```
You have to copy that URL (e.g. http://192.168.1.232:5000) to the FileSys page and save it.

 
## Python Flask Server

ESP32 is very powerful but its storage memory (flash) is very limited while we need to play mp3 (1 - 4 MBytes) along with midi files, therefore we need an external storage server. See the folder /static where a few mp3 and mid file are already available.

### Hardware and libraries

You can run this python app on a Raspberry PI 3B, 4 or 5 or on any Linux (Ubuntu 22.04 tested). 

You need:
* Python 3
* fluidsynth
```
sudo apt install fluidsynth
```

* flask
```
pip install Flask
```
  
### Server code

The pages served by the flask python server are directly called and managed by the client (ESP32). Anyway they can be called for testing purposes.

They are:

* /events?query=<word_to_search_separated_by_a_+>   e.g. /search?q=rolling+stones
  
  return a list of songs accomplishing the query

  
* /tracks?index=<codefile>  e.g. /tracks?index=63118
  
  return the tracks for that filecode (song id)


* /get_mp3?index=<codefile>  e.g. /get_mp3?index=63118
  
  convert the mid file (e.g. 63118.mid) to an mp3 file (e.g. 63118.mp3) and store it to the server folder "static"


* /output.mp3?query=<codefile>  e.g. /output.mp3?query=63118
  
  stream that mp3 file (e.g. 63118.mp3) to the client 


* /output.mid?query=<codefile>  e.g. /output.mid?query=63118

  stream that mid file (e.g. 63118.mid) to the client 



