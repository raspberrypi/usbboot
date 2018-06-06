FROM debian:jessie

RUN apt-get update &&\
    apt-get upgrade -y &&\
    apt-get install -y\
      gcc\
      libusb-1.0-0-dev\
      make

WORKDIR /src

CMD [ "make" ]
