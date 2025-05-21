#!/bin/bash


usage() {
        echo "Usage: $0 [options]"
        echo ""
        echo "Build icon set."
        echo ""
        echo "Supported options:"
        echo "-s, --setup               Setup all necessery tool"
        echo "-b, --build               Build icon set"
}

setup() {
        brew install libsvg-cairo
        python3 -m venv .venv
        source .venv/bin/activate
        pip3 install --upgrade pip
        pip3 install cairosvg Pillow
}

build() {
        source .venv/bin/activate
        # python3 svg2iconset.py
        python3 png2iconset.py app.png -o app.iconset
        iconutil -c icns app.iconset -o app.icns
}

while [ $# != 0 ] ; do
        option="$1"
        shift

        case "${option}" in
        -b|--build)
                build
                ;;
        -s|--setup)
                setup
                ;;
        -h|--help)
                usage
                exit 0
                ;;
        *)
                echo "Unknown option ${option}"
                exit 1
                ;;
        esac
done