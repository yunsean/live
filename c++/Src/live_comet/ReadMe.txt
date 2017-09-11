订阅通道  curl -v -X POST http://localhost:99/sub?token=111 -d[{\"cname\":\"haha\",\"seq\":0}]      （注册源，并且订阅通道）
推送消息  curl -v "http://localhost:99/pub?cname=haha&content=111"
推送JSON  curl -v -X POST http://localhost:99/pub?cname=haha -d{\"anybody\":1,\"anybody2\":\"ImDylan\"}   （将JSON作为消息体推送出去，只能是JSON）
获取状态  curl http://localhost:88/info

拉取数据  curl -o bad.flv http://localhost:99/pull?stub=111                     （stub要自己保证唯一，否则会覆盖之前的）
推送数据  curl -X POST http://localhost:99/push?stub=111 --data-binary @1.flv   （必须确保stub已经在waiting状态）
获取状态  curl http://localhost:99/inspect?stub=111

远程调用  curl "http://localhost:88/call?method=play&timecode=1497892381066&token=xzuu4"   （除开method之外的其他参数以及header中的非常规header将通过json方式原封传递给源，服务器会自动生成不重复的stub）
调用回答  curl -X POST "http://localhost:88/return?stub=1" --data-binary @1.flv （服务器会将非常规header以及content-length content-type等信息应答给调用者）

应用举例：
播放 ffplay "http://localhost:88/call?method=play&timecode=1497892381066&token=xzuu4"
关闭wifi wget -O - "http://localhost:88/call?method=wifi&enable=false&token=dylan"
启动wifi wget -O - "http://localhost:88/call?token=dylan&method=wifi&enable=true"
拉取分片列表 curl "http://localhost:88/call?begin=0&method=list&token=xzuu4"