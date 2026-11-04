# Rendert die Wetter-Icons aus der Weather-Icons-Schrift (Erik Flowers, SIL OFL 1.1)
# als kantengeglaettete Sprite-Leiste mit vier Alpha-Stufen.
#
# Die Uhr faerbt die Leiste ueber die Palette ein (gbitmap_get_palette), deshalb
# kommt nur Schwarz in vier Deckungsstufen vor -- bitmapgen legt daraus eine
# 2-Bit-Palette an.
#
# Aufruf: python tools/make_icons.py <weathericons.ttf>
import sys
from PIL import Image, ImageDraw, ImageFilter, ImageFont

# Reihenfolge = Zustandscode, muss zu COND_* in main.c und index.js passen
GLYPHS = [
    0xf00d,  # 0  klar, Tag            wi-day-sunny
    0xf02e,  # 1  klar, Nacht          wi-night-clear
    0xf002,  # 2  heiter, Tag          wi-day-cloudy
    0xf086,  # 3  heiter, Nacht        wi-night-alt-cloudy
    0xf013,  # 4  bewoelkt             wi-cloudy
    0xf014,  # 5  Nebel                wi-fog
    0xf01c,  # 6  Niesel               wi-sprinkle
    0xf019,  # 7  Regen                wi-rain
    0xf01a,  # 8  Schauer              wi-showers
    0xf01b,  # 9  Schnee               wi-snow
    0xf0b5,  # 10 Schneeregen/Eisregen wi-sleet
    0xf01e,  # 11 Gewitter             wi-thunderstorm
]

# Kantenlaenge je Icon in Pixeln
# ~round = gabbro, ~rect = emery (Resource-Tags der SDK)
SIZES = {'round': 46, 'rect': 40}

# Strichverstaerkung in Zielpixeln: die Schrift ist recht duenn, das Zifferblatt
# mit Bitham Black und dicken Zeigern dagegen kraeftig
BOLDEN = 0.6
MAX_GROW = 1.3

def render(font_path, cell, out):
    # gross rendern und eng beschneiden; alle Icons etwa mit demselben Faktor
    # verkleinern, damit die Strichstaerke gleich bleibt -- kleine Motive wie
    # der Mond duerfen bis MAX_GROW groesser werden
    big = ImageFont.truetype(font_path, 400)
    crops = []
    for cp in GLYPHS:
        img = Image.new('L', (700, 700), 0)
        ImageDraw.Draw(img).text((100, 100), chr(cp), font=big, fill=255)
        crops.append(img.crop(img.getbbox()))
    scale = (cell - 2) / max(max(c.size) for c in crops)
    grow = int(BOLDEN / scale) | 1          # ungerade Filtergroesse
    sheet = Image.new('L', (cell * len(GLYPHS), cell), 0)
    for i, img in enumerate(crops):
        pad = grow
        padded = Image.new('L', (img.size[0] + 2 * pad, img.size[1] + 2 * pad), 0)
        padded.paste(img, (pad, pad))
        img = padded.filter(ImageFilter.MaxFilter(grow))
        f = min(scale * MAX_GROW, cell / max(img.size))
        w, h = max(1, round(img.size[0] * f)), max(1, round(img.size[1] * f))
        img = img.resize((w, h), Image.LANCZOS)
        sheet.paste(img, (i * cell + (cell - w) // 2, (cell - h) // 2))
    # auf vier Stufen 0/85/170/255 runden
    quant = sheet.point(lambda v: min(3, (v + 42) // 85) * 85)
    rgba = Image.new('RGBA', sheet.size, (0, 0, 0, 0))
    rgba.putalpha(quant)
    rgba.save(out)

if __name__ == '__main__':
    for plat, cell in SIZES.items():
        render(sys.argv[1], cell, 'resources/images/weather~%s.png' % plat)
        print(plat, cell)
