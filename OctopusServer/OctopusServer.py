import requests
from bs4 import BeautifulSoup
import csv 
import os
import mido
import re
import json
from flask import Flask, request, render_template, Response, send_file, jsonify
import subprocess

instruments = [
   	"acoustic grand piano",   "bright acoustic piano",  "electric grand piano",  "honky-tonk piano", "rhodes piano",   "chorused piano",
   	"harpsichord",  "clavinet",  "celeste",   "glockenspiel",   "music box",  "vibraphone",
   	"marimba",   "xylophone",  "tubular bells",  "dulcimer",    "hammond organ",   "percussive organ",
   	"rock organ",   "church organ", "reed organ",   "accordion",   "harmonica", "tango accordion",
   	"nylon guitar",  "steel guitar",  "jazz guitar",   "clean guitar",  "muted guitar",   "overdriven guitar",
   	"distortion guitar",   "guitar harmonics",   "acoustic bass",    "fingered electric bass",  "picked electric bass",  "fretless bass",
   	"slap bass 1",  "slap bass 2",  "synth bass 1",  "synth bass 2",  "violin",    "viola",
   	"cello",     "contrabass",  "tremolo strings",   "pizzcato strings",  "orchestral harp",      "timpani",
   	"string ensemble 1",   "string ensemble 2",   "synth strings 1",   "synth strings 1",   "choir aahs",     "voice oohs",
   	"synth voices",    "orchestra hit",   "trumpet",   "trombone",  "tuba",      "muted trumpet",
   	"frenc horn", "brass section",  "syn brass 1",  "synth brass 2",  "soprano sax",  "alto sax",
   	"tenor sax",  "baritone sax",   "oboe",      "english horn",  "bassoon",   "clarinet",
   	"piccolo",   "flute",     "recorder",  "pan flute",  "bottle blow",    "shakuhachi",
   	"whistle",   "ocarina",   "square wave",   "saw wave",   "calliope lead",  "chiffer lead",
   	"charang lead",   "voice lead",   "fifths lead",   "brass lead",  "newage pad",  "warm pad",
   	"polysyn pad",   "choir pad",   "bowed pad",  "metallic pad",  "halo pad",   "sweep pad",
   	"rain",    "soundtrack",  "crystal",   "atmosphere",  "brightness",  "goblins",
   	"echoes",   "sci-fi",  "sitar",     "banjo",     "shamisen",  "koto",
   	"kalimba",   "bagpipes",  "fiddle",    "shanai",   "tinkle bell",  "agogo",
   	"steel drums", "woodblock", "taiko drum",     "melodoc tom",      "synth drum",    "reverse cymbal",
   	"guitar fret noise",   "breath noise",   "seashore",  "bird tweet",    "telephone ring", "helicopter",
   	"applause",  "gunshot"
]

# Define proxies to use.

proxy_list = [
    {
        'http': 'http://35.185.196.38:3128',
        'https': 'http://35.185.196.38:3128' # United States The Dalles
    },

    {
        'http': 'http://185.217.136.67:1337',
        'https': 'http://20.72.218.43:8080' # Microsoft
    }
]
proxies = proxy_list[1]

def save_to_csv(filename, data):
    """Save data to a CSV file."""
    with open(filename, 'a', newline='') as csvfile:
        writer = csv.writer(csvfile)
        writer.writerow(data)

def parse_and_save_title(title, href, filename):
    """Parse the title string and href attribute, and save their parts to a CSV file."""
    parts = title.split(" — ")
    if len(parts) == 2:
        # Extract the 'yyyyy' part from the href attribute
        yyyyy = os.path.basename(href).split('-')[-1]
        print("Save: filename=", filename, "parts=", parts, "code=", yyyyy)
        save_to_csv(filename, parts + [yyyyy])
    else:
        print(f"Title '{title}' does not have the expected format.")

def getFilenameUrlFromSong(songname):
    
    # Define a regular expression pattern to match numbers at the end of the string
    pattern = r'\d+$'
    
    # Use the findall method to extract all matching numbers from the string
    numbers = re.findall(pattern, songname)
    
    # If numbers were found, return the last one; otherwise, return None
    if numbers:
        return "https://midifind.com/files/0-0-1-" + numbers[-1] + "-20"
    else:
        return None

# return an array of urls where the words_to_search appear
def search(words_to_search):

    keywords = words_to_search.split(" ")
    # Convert JSON string to Python list of dictionaries
    with open("az.json", 'r') as file:
        data = json.load(file)

    for record in data:
        if all(keyword in record["artist"] or keyword in record["song"] for keyword in keywords):
            yield f"data: {json.dumps(record)}\n\n"

    # Send termination message
    yield "data: END\n\n"    


# return an array of urls where the words_to_search appear
def search_old(words_to_search):
    ndx = 0
    urls = []
    words_to_search = str(words_to_search).replace(" ", "+")
    url = 'https://midifind.com/search?q=' + words_to_search + ";p=1"
    response_search_page = requests.get(url,proxies=proxies)
    # Check if the request was successful (status code 200)
    if response_search_page.status_code == 200:
        # Parse the HTML content using BeautifulSoup
        soup_search_page = BeautifulSoup(response_search_page.text, 'html.parser')
        # Find all elements with class swchItem
        swchItems = soup_search_page.find_all(class_='swchItem')
        # get the last page number    
        if(swchItems == []):
            last_page_number_text = 1    
        else:
            last_page_number_text = swchItems[-2].find_all('span')[0].text
        last_page_number = int(last_page_number_text)
        for page_number in range(1,last_page_number+1):
            songs_items = soup_search_page.find_all(class_='item')
            for song_item in songs_items:
                if song_item.has_attr("href") and song_item.has_attr("title"):
                    title = str(song_item["title"])
                    if title.startswith("Result"):
                        print(song_item["href"])
                        #myurl = getFilenameUrlFromSong("https://midifind.com/files/r/rolling_stones_the/rolling_stones_the_angie/1597-1-0-51729")
                        # download the file and get the tracks' names.
                        song_fn = getFilenameUrlFromSong(song_item["href"])
                        #download(song_fn)
                        #tracks = list_tracks_from_midi_file()
                        #duration, length = get_midi_duration_and_length("file.mid")
                        parts = song_item["href"].split("/")
                        if len(parts) > 5:
                            filecode = parts[5].split("-")[3]
                        else:
                            filecode = 0    
                        urls.append(song_item["href"])
                        artist = parts[3]
                        song = parts[4]
                        if song[0:len(artist)] == artist:
                            song = song[len(artist)+1:]
                        result = {"ndx" : ndx , "artist" : artist,"song":song,"filecode":filecode, "tracks": [] }
                        yield f"data: {json.dumps(result)}\n\n"
                        ndx = ndx + 1
                        # Flushing the response to send the data immediately
                        #yield "retry: 60000\n\n"

            if page_number < last_page_number:        
                url = 'https://midifind.com/search?q=' + words_to_search + ";p=" + str(page_number+1)
                print(url)
                response_search_page = requests.get(url,proxies=proxies)
                if response_search_page.status_code == 200:
                    # Parse the HTML content using BeautifulSoup
                    soup_search_page = BeautifulSoup(response_search_page.text, 'html.parser')

    # Send termination message
    yield "data: END\n\n"    
    
    return urls            


def download(codefile):
    url = "https://midifind.com/files/0-0-1-" + codefile + "-20"
    r = requests.get(url, proxies=proxies, allow_redirects=True)
    open('static/' + codefile + '.mid', 'wb').write(r.content)
   
def GetAllInitials():
    for char in range(ord('a'), ord('z') + 1):
    #for char in range(ord('a'), ord('a') + 1):

        # URL of the website you want to scrape
        # url = 'https://midifind.com/files/en/a?page38'
        url = 'https://midifind.com/files/en/' + chr(char)
        # Send an HTTP request to the website and get the HTML content
        response_first_char_page = requests.get(url,proxies=proxies)
        # Check if the request was successful (status code 200)
        if response_first_char_page.status_code == 200:
            # Parse the HTML content using BeautifulSoup
            soup_first_char_page = BeautifulSoup(response_first_char_page.text, 'html.parser')
            # Find all elements with class swchItem
            swchItems = soup_first_char_page.find_all(class_='swchItem')
            # get the last page number    
            last_page_number_text = swchItems[-2].find_all('span')[0].text
            last_page_number = int(last_page_number_text)
            for page_number in range(1,last_page_number+1):
                url_char_page_number = url = 'https://midifind.com/files/en/' + chr(char) + '?page' + str(page_number)
                print(url_char_page_number)

app = Flask(__name__)


@app.route('/length')
def length():
    codefile = request.args.get('index')
    filename = "static/" + codefile + ".mid"
    if os.path.exists(filename) == False:
            download(codefile) 
    # Get MIDI duration
    mid = mido.MidiFile(filename)
    total_ticks = sum(msg.time for msg in mid)
    ticks_per_beat = mid.ticks_per_beat
    total_seconds = mido.tick2second(total_ticks, ticks_per_beat, mido.bpm2tempo(120))

    # Get file length in bytes
    file_length = os.path.getsize(filename)

    result = [total_seconds, file_length]
    
    response = jsonify(result)
    response.headers.add("Access-Control-Allow-Origin", "*")
    return response



@app.route('/tracks')
def tracks():
    codefile = request.args.get('index')
    tr = []
    try:
        fn = "static/" + codefile + ".mid"
        if os.path.exists(fn) == False:
            download(codefile) 
    
        mid = mido.MidiFile(fn)
        for i, track in enumerate(mid.tracks):
            for msg in track:
                if msg.type == "program_change":
                    tr.append({'track': i, 'track_name' : track.name, 'program': msg.program, 'instrument':instruments[msg.program]})
    except Exception as e:
        print("Error:", e)

    response = jsonify(tr)
    response.headers.add("Access-Control-Allow-Origin", "*")
    return response

@app.route('/search')
def index():
    return render_template('index.html')

@app.route('/events')
def events():
    query = request.args.get('query')
    if not query:
        return "Query parameter required", 400
    response = Response(search(query), content_type='text/event-stream')
    response.headers.add("Access-Control-Allow-Origin", "*")
    return response

@app.route('/get_mp3')
def get_mp3():
    # Get the codefile index from the request parameters
    codefile = request.args.get('index')

    fn  = "static/" + codefile + ".mp3"
    # Get the midi file correponding to the codefile index
    if os.path.exists(fn) == False:
        download( codefile)
        # Convert MIDI to MP3 using a command-line tool like FluidSynth
        command = "fluidsynth -l -T raw -F - 1mgm.sf2 static/" + codefile + ".mid | lame -b 256 -r - " + fn
        subprocess.run(command,shell=True, check=True)

    # Send the MP3 file as a response
    #return send_file("output.mp3", as_attachment=True)
    response = Response('created static/' + codefile + ".mp3", content_type='text')
    response.headers.add("Access-Control-Allow-Origin", "*")
    return response
    #    else:
    #        return 'MP3 file not found', 404
    # else:
    #    return 'Invalid index', 400

@app.route('/output.mp3')
def stream_mp3():
    # Path to your MP3 file
    query = request.args.get('query')
    if query is None:
        mp3_path = 'static/63114.mp3'
    else:
        mp3_path = 'static/' + query + '.mp3'

    if os.path.isfile(mp3_path):
        return send_file(mp3_path)
    else:
        return "" 

@app.route('/output.mid')
def stream_midi():
    # Path to your mid file
    query = request.args.get('query')
    if query is None:
        mid_path = 'static/63114.mp3'
    else:
        mid_path = 'static/' + query + '.mid'

    if os.path.isfile(mid_path):
        response = send_file(mid_path)
        response.headers.add("Access-Control-Allow-Origin", "*")
        return response
    else: 
        return "" 


if __name__ == '__main__':
    app.run(host="0.0.0.0", debug=True)
