#!/usr/bin/env python3
import os
import sys
from PIL import Image
import argparse

def create_iconset(source_image_path, output_folder):
    # Überprüfe, ob die Quelldatei existiert
    if not os.path.exists(source_image_path):
        print(f"Die Quelldatei '{source_image_path}' wurde nicht gefunden.")
        sys.exit(1)
        
    # Bild öffnen
    img = Image.open(source_image_path)
    
    # Optionale Warnung, falls das Bild nicht 1024x1024 ist
    if img.size != (1024, 1024):
        print("Warnung: Das Quellbild ist nicht 1024x1024. Das Ergebnis könnte abweichen.")
    
    # Definition der benötigten Icongrößen (macOS iconset)
    sizes = {
        "icon_16x16.png": 16,
        "icon_16x16@2x.png": 32,
        "icon_32x32.png": 32,
        "icon_32x32@2x.png": 64,
        "icon_128x128.png": 128,
        "icon_128x128@2x.png": 256,
        "icon_256x256.png": 256,
        "icon_256x256@2x.png": 512,
        "icon_512x512.png": 512,
        "icon_512x512@2x.png": 1024,
    }
    
    # Erstelle den Ausgabeordner, falls nicht vorhanden
    if not os.path.exists(output_folder):
        os.makedirs(output_folder)
    
    # Erzeuge die Icons in den entsprechenden Größen
    for filename, size in sizes.items():
        resized_img = img.resize((size, size), Image.LANCZOS)
        save_path = os.path.join(output_folder, filename)
        resized_img.save(save_path)
        print(f"Gespeichert: {save_path} ({size}x{size})")
    
    print("Apple Iconset wurde erfolgreich erstellt in:", output_folder)

def main():
    parser = argparse.ArgumentParser(description="Erzeugt aus einer 1024x1024 PNG ein Apple Iconset.")
    parser.add_argument("source", help="Pfad zur 1024x1024 PNG Quelldatei.")
    parser.add_argument("-o", "--output", default="MyIcon.iconset",
                        help="Ausgabeordner für das Iconset (Standard: MyIcon.iconset)")
    args = parser.parse_args()
    
    create_iconset(args.source, args.output)

if __name__ == "__main__":
    main()
