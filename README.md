# Lane detection in C++
yes I know, cliche. this is just exploratory/for learning purposes/bored

## To run:
1. Install [docker](https://docs.docker.com/engine/install/)

2. Clone repo

3. Build:
```
$ docker build -t lane_detection .
```

4. Run:
```
$ xhost +local:docker
$ docker run --rm -e DISPLAY=$DISPLAY -v /tmp/.X11-unix:/tmp/.X11-unix lane_detection
```
