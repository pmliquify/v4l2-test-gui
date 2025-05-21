set -e

docker build -t v4l2_test_gui ./docker
echo "USER: $USER"
docker run --privileged --network host -v /dev/bus/usb:/dev/bus/usb -v /dev:/dev -v /media/$USER:/media/nvidia:slave -v $PWD:/src -it v4l2_test_gui /bin/bash
