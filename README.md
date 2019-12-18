# LocalChat

该案例在Linux系统上实现了一个本地聊天室。在不同终端上的用户，运行localchatclient。使用to [usr]:[info]命令向[usr]用户发送[info]；使用quit命令退出登录。

该案例是基于B/S架构。用户端通过一条公共有名管道server_fifo向服务端发送请求，服务端通过各个用户的私有管道向用户发送响应。请求包分为3种：登陆包、聊天包、退出包。响应包分为2种，信息包和不在线包。

当用户在终端上运行localchatclient时，会向服务端发送登陆包。服务端维护一个登录用户的链表。当收到登录请求时，会将其加入到该链表当中，并为该用户创建私有管道。

当用户通过to命令向其他用户发送信息时，会将收信方和信息封装进聊天包当中，发送给服务端。当服务端收到聊天包时，会先判断收信方是否在线。如果在线，服务端会通过收信方的私有管道向其发送info响应包。该响应包中封装了发信方和信息。如果没有在线，服务端则会向发信方发送notonline响应包。

当用户使用quit命令退出时，会向服务端发送退出包。服务端收到退出包时，会将其从登录用户链表中删除，并关闭用户管道的写段。用户删除自己的私有管道。

![](https://github.com/supperPants/LocalChat/blob/master/image/%E6%9C%AC%E5%9C%B0%E8%81%8A%E5%A4%A9%E5%AE%A4.png)

## MCP/1.0协议（MyChatProtocol）

请求包格式：

![](https://github.com/supperPants/LocalChat/blob/master/image/%E8%AF%B7%E6%B1%82%E5%8D%8F%E8%AE%AE.png)

    请求类型：
      online:登录包
      chat:聊天包
      offline:退出包

响应包格式：

![](https://github.com/supperPants/LocalChat/blob/master/image/%E5%93%8D%E5%BA%94%E5%8C%85.png)

    响应类型：
      info:信息包
      notonline:不在线包
  
  


