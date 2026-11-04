# Schickt Farbvorlagen aus src/pkjs/custom-clay.js per AppMessage an den
# laufenden Emulator und macht je einen Screenshot (fuer Store und Pruefung).
# Laeuft in WSL: python3 tools/theme_shots.py <plattform> <ausgabeordner> <vorlage>...
import json, re, subprocess, sys, time

plat, outdir, names = sys.argv[1], sys.argv[2], sys.argv[3:]
src = open('src/pkjs/custom-clay.js').read()
keys = re.search(r"COLOR_KEYS = \[(.*?)\]", src, re.S).group(1)
keys = re.findall(r"'(\w+)'", keys)
templates = {m.group(1): re.findall(r"'([0-9A-F]{6})'", m.group(2))
             for m in re.finditer(r"^\s+(\w+):\s+\[(.*?)\]", src, re.M)}
ids = json.load(open('build/js/message_keys.json'))

def pebble(*args):
    subprocess.run(['pebble'] + list(args) + ['--emulator', plat],
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

for name in names:
    ints = ['%d=%d' % (ids[k], int(v, 16)) for k, v in zip(keys, templates[name])]
    pebble('send-app-message', '--int', *ints)
    # emu-set-time haelt nach einer pypkjs-Verbindung nicht: erst warten,
    # bis die Nachricht durch ist, dann die Zeit direkt vor dem Bild setzen
    time.sleep(3)
    pebble('emu-set-time', '10:09:30')
    time.sleep(2)
    pebble('screenshot', '--no-open', '%s/%s_%s.png' % (outdir, plat, name))
    print(plat, name)
