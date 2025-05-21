FROM ubuntu:22.04

RUN apt-get update && \
    DEBIAN_FRONTEND=noninteractive TZ=Etc/UTC apt-get -y install tzdata && \
    apt-get -y install sudo wget bc xxd usbutils nano cpio iputils-ping liblz4-tool dosfstools flex bison libssl-dev libxml2-utils sshpass udev abootimg nfs-kernel-server iproute2 && \
    apt-get clean

RUN sudo apt update && \
    sudo apt install -y build-essential cmake && \
    sudo apt install -y libopencv-dev && \
    sudo apt install -y qt6-base-dev && \
    sudo apt install -y libglx-dev libgl1-mesa-dev && \
    sudo apt install -y libxcb-xkb-dev && \
    sudo apt install -y libxkbcommon-dev libxkbfile-dev && \
    sudo apt install -y libvulkan-dev && \
    sudo apt install -y git


RUN useradd -m bazovc && echo "bazovc:bazovc!" | chpasswd && adduser bazovc sudo

#RUN sudo -H pip install pyyaml

#RUN sudo -H pip3 install pyyaml

USER bazovc

WORKDIR /src
