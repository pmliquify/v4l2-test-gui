import os
import subprocess
import cairosvg

iconset_folder = 'app.iconset'
if not os.path.exists(iconset_folder):
    os.makedirs(iconset_folder)

sizes = {
    'icon_16x16.png': 16,
    'icon_16x16@2x.png': 32,
    'icon_32x32.png': 32,
    'icon_32x32@2x.png': 64,
    'icon_128x128.png': 128,
    'icon_128x128@2x.png': 256,
    'icon_256x256.png': 256,
    'icon_256x256@2x.png': 512,
    'icon_512x512.png': 512,
    'icon_512x512@2x.png': 1024,
}

svg_file = 'icon.svg'
if not os.path.exists(svg_file):
    print("SVG-Datei nicht gefunden!")
    exit(1)

for filename, size in sizes.items():
    output_path = os.path.join(iconset_folder, filename)
    cairosvg.svg2png(url=svg_file, write_to=output_path, output_width=size, output_height=size)
    print(f"Erstellt: {output_path}")

icns_file = 'app.icns'
subprocess.run(['iconutil', '-c', 'icns', iconset_folder, '-o', icns_file])
print(f".icns-Datei erstellt: {icns_file}")
