FROM ubuntu:16.04

# Setup
RUN apt-get update -y
RUN apt-get install -y apt-utils | true
RUN apt-get install -y software-properties-common python-software-properties
RUN apt-get update -y

RUN add-apt-repository -y ppa:ubuntu-toolchain-r/test
RUN apt-add-repository -y ppa:george-edison55/cmake-3.x
RUN add-apt-repository -y ppa:gnome3-team/gnome3
RUN add-apt-repository -y ppa:gnome3-team/gnome3-staging
RUN apt-get update -y

RUN apt-get install -y git gcc g++ cmake cmake-data liblua5.2-dev libglib2.0-dev libxerces-c-dev libpango1.0-dev librsvg2-dev libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev libgstreamer-plugins-good1.0-dev libgtk-3-dev

# Build preparation
RUN mkdir -p /src/
RUN git clone https://github.com/telemidia/ginga.git /src/ginga
RUN mkdir -p /src/ginga/_build
 
# Build
WORKDIR /src/ginga/_build
RUN cmake ../build-cmake -DWITH_CEF=OFF
RUN make -j4

