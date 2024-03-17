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

![](https://github.com/guido57/PlayingOctopusSD/blob/main/docs/PlayingOctopusSDblockdiagram.png)

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

it can be used to delete or update or upload .mp3 and .mid files. The index update is automatic.
 
