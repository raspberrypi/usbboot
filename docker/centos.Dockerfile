FROM centos:centos7

RUN yum upgrade -y && \
    yum install -y\
      gcc\
      libusbx-devel\
      make

WORKDIR /src

CMD [ "make" ]
