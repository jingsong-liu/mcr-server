# mcr-server
> Your newborn http server.

## Overview
mcr-server is an original http-server based on tcp.
It is designed for deploying static web pages. Also it's a good project for learner

## Features
*	No library depency(except libc and libpthread)
* Lightweight
*	Service Configurable
* High Responsiveness(kept by thread connections)
*	Support mime types

## build and run
### 1.Get source code
```sh
git clone https://github.com/jeesyn/mcr-server
```

### 2.Compile 
Navigate to the source code directory, and run make:
```sh
make
```
Executable file `mcr-server` will be generate in current directory.

### 3.Config and run
Write server configurations to `your-configure.conf` file, and start mcr-server with option -c. Otherwise `default.conf` will be load.
```sh
./mcr-server [-c your-configure.conf] [-d]
```

## Contributing
Fork the mcr-server repository and create pull request, it will be processed as soon as possible.

## Example
Getting a mirror of [redis](https://redis.io) official site and deploying it by mcr-server.
### 1. Get website mirror
```sh
wget redis.io -m 
```
### 2. Config the downloaded website directory as wwwroot
```
...
wwwroot: realpath-of-redis.io
...
```
Save the config file name as `redis-io.conf`
### 3. Deploy
```sh 
./mcr-server -c redis-io.conf -d
```
### 4. Have fun.
Open browser and go localhost:8080/
You will get [this](http://zz.culti.site:8082/) vision.
