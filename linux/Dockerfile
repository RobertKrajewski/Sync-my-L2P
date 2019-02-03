FROM ubuntu:16.04

RUN apt-get update && apt-get install -y qt5-default qt5-qmake libssl-dev git build-essential qttools5-dev-tools
RUN mkdir /work
WORKDIR /work
COPY . /work
RUN mkdir /res
RUN qmake DESTDIR=/res -o syncmyl2p.mk
RUN make -f syncmyl2p.mk
RUN linux/create_appimage.sh