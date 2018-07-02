# live
基于libevent的媒体转发服务器

live_gateway为流媒体转发服务器主程序，采用插件化方式支持rtmp以及国内平安城市最大供应商（接触过的人都知道）的直播流拉取
live_comet为一个http转发请求服务器，实现从内网到内网的请求，可以用于一些调试场合，比如在你的android设备主机上植入comet设备端，当调试时可以通过comet直接访问android的的shell命令或者文件拉取

live_gateway采用插件结构，支持网关输入（source）、解编码(codec)、输出(sink)三个插件类型
通过编写相应的插件，让网关具有特定的功能。
已包含如下插件：
live_source_file  文件输入（支持对录制的原始码流重新放入到网关中作为输入使用，一般测试场景下会用到）
live_source_hxht  HXHT插入插件（平安城市一个供应商）
live_source_onvif Onvif输入插件（支持Onvif摄像头输入）
live_source_rtmp  Rtmp输入插件（一般可作为移动终端rtmp推流输入使用）
live_codec_nal    AVC/AAC帧数据重定向编码插件
live_sink_file    点播输出插件（直接将本地文件通过http输出，与网关本身无关）
live_sink_flv     Flv输出插件（将avc/aac编码为http/flv输出，可使用flash播放器或者ijkplayer移动播放播放网关流）
live_sink_raw     原始码流输出插件（将网关从源输入的信号原样输出，该格式的文件可供live_source_file再次使用，一般用于网关信号录制使用）

如有任何问题，欢迎联系yunsean@163.com
