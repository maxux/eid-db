import os
import sys
import time
import datetime
import sqlite3
import json
import base64
from flask import Flask, request, render_template, make_response

app = Flask(__name__, static_url_path='/static')
app.url_map.strict_slashes = False

def format_niss(niss):
    return "%s.%s.%s-%s.%s" % (niss[0:2], niss[2:4], niss[4:6], niss[6:9], niss[9:])

def format_cardid(cardid):
    return "%s-%s-%s" % (cardid[0:3], cardid[3:10], cardid[10:])

@app.route('/', methods=['GET'])
def generate_v2():
    db = sqlite3.connect("/tmp/idcards.sqlite3")

    c = db.cursor()
    c.execute('SELECT niss, cardid, fields, photo, added FROM cards')
    data = c.fetchall()
    db.close()

    cards = []

    for entry in data:
        fields = json.loads(entry[2])
        photo = base64.b64encode(entry[3])

        entry = {
            "niss": format_niss(entry[0]),
            "cardid": format_cardid(entry[1]),
            "data": fields,
            "photo64": photo.decode('utf-8'),
            "updated": entry[4],
        }

        cards.append(entry)

    content = {
        "cards": cards,
    }

    return render_template("index.html", **content)

print("[+] listening")
app.run(host="0.0.0.0", port=7888, debug=True, threaded=True)

