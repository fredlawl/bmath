FROM debian:bookworm-slim

RUN apt-get update && \
  apt-get -y install \
  devscripts \
  g++ \
  libghc-iconv-dev \
  libreadline-dev \
  meson \
  pkg-config

WORKDIR /work/build

COPY . /work/build/

