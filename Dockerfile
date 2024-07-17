FROM ubuntu:jammy

ENV DEBIAN_FRONTEND=noninteractive

RUN apt update && apt install -y \
    build-essential cmake libopencv-dev gdb x11-apps

RUN apt install -y pip && pip install opencv-python

COPY main.cpp /main.cpp
COPY test.py /test.py
COPY CMakeLists.txt /CMakeLists.txt
COPY assets/ /assets/

RUN mkdir build && cd build \
    && cmake .. && make

# Clean up
RUN apt remove -y build-essential cmake

CMD [ "./build/main" ]