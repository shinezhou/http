# http

一个可以进行http请求的php扩展，可以在同一个进程中共享http连接。

## install

```sh
phpize
./configure
make && make install
```

然后把http.so加入到php.ini中

## Api

### function http_get($url, $timeout=1)
- $url get请求的url
- $timeout 请求执行的超时时间单位秒

### function http_post($url, $data, $timeout=1)
- $url post请求的url
- $data post参数，一个数组
- $timeout 请求执行的超时时间单位秒

### function http_info()
- 返回最近一次请求的错误信息
