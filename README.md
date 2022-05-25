**Linux下线程通信消息队列实现，支持阻塞、超时等机制**

### 测试

##### 安装xmake

```c
sudo add-apt-repository ppa:xmake-io/xmake
sudo apt update
sudo apt install xmake
```

**查看是否安装成功:**

```c
xmake --version
```

##### 编译

```c
xmake
```

##### 执行

````c
./build/linux/x86_64/debug/linqueue_test
````

