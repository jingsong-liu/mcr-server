# mcr-server
> Your newborn http server.

## Overview
mcr-server is an original http-server based on tcp.
For now, it is only for learning linux environment programming. Common used programming skills such as socket/io ,tcp/ip and http protocol, multiple threads, multiple processes ... All of them are necessary for this project.
In the future, it will serve for deploying static pages(some interesting web pages crawled from net).

## Features
*	除libc/libpthread以外，它不依赖于的任何库
*	服务器可配置
*	基于线程模型和事件模型
*	支持并发下载资源
*	支持对象存储资源（七牛云）

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
Write server configurations to `Default.conf` file and start mcr-server
```sh
./mcr-server
```

## Contributing
Fork the mcr-server repository and create pull request, it will be processed as soon as possible.
