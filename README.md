# ELFMemoryLoader

**`Living off the Land Attack` in Linux。**

**Linux场景下的核心载荷不落地攻击。**



Loader get elf data from remote server, then use file descriptor to run elf in memory。

加载器从远程服务器上获取到elf的内容，并通过文件描述符将内存中的elf运行起来。



## Principle Introduction

To launch an attack, you should run a tcp server first. The TCP server is used to provide ELF data after requested，and of course client need to pass authorization before downloading elf data 。

After that，you may want to modify IP 、port、auth-password even package size as appropriate。Once compiled，you can run client part in victim device。

In fact, the client run the target elf in the following steps:

step 1. Pass the authentication.
step 2. Request the stage1 packet.
step 3. Request the stage2 packets（elf data）.
step 4. Create anonymous file in memory.
step 5. Fill the anonymous file content with the remote elf data.
step 6. run the anonymous file with the file descriptor.



在开始实施攻击前，你需要先搭建一个特别的tcp服务器。这个服务器用来向客户端返回elf数据，客户端获得数据前要先鉴权。

做好服务器端的准备工作后，可以改改客户端源码中的IP、端口、密码等信息。编译好工具后直接传到目标设备上运行即可。

实际上，客户端内存加载elf的思想如下：

step 1. 通过服务器的认证。
step 2. 从服务器上请求stage1的数据。该数据其实描述的是elf的大小。
step 3. 从服务器上请求stage2的数据，这部分数据其实就是elf数据。
step 4. 在内存中运行匿名文件。
step 5. 填充匿名文件，把得到的elf数据填进来。
step 6. 通过匿名文件的句柄把文件从内存中跑起来。



目前没有实现对请求数据的加密，但是这其实很简单，自己在收发模块前添加简单的加解密函数即可。

## Compile

使用gcc进行常规编译。

```bash
# client
gcc elfloader.c -o loader
# server
gcc server.c -lpthread -o server
```



## Usage

### server

```shell
./server -p [password] -f [target_elf] -v
```

### client

```shell
./loader
```



## Sample

Run the server.

![image-20210918160524072](https://raw.githubusercontent.com/aplyc1a/blogs_picture/master/image-20210918160524072.png)

Run the client.

![image-20210918160531833](https://raw.githubusercontent.com/aplyc1a/blogs_picture/master/image-20210918160531833.png)

The Server shows the connect event.

![image-20210918160537904](https://raw.githubusercontent.com/aplyc1a/blogs_picture/master/image-20210918160537904.png)
