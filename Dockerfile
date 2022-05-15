# syntax=docker/dockerfile:1
FROM ubuntu:22.04
COPY . /repo
WORKDIR /repo
ARG IS_DOCKER=1
RUN bash setup.sh