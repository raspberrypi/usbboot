FROM debian:jessie

RUN apt-get update &&\
    apt-get upgrade -y &&\
    apt-get install -y\
      build-essential\
      libusb-1.0-0-dev

WORKDIR /src

CMD [ "make" ]
