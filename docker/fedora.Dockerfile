FROM fedora:28

RUN dnf --refresh upgrade -y && \
    dnf install -y\
      gcc\
      libusbx-devel\
      make

WORKDIR /src

CMD [ "make" ]
