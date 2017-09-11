1、libevent编译说明：
	linux:
		配置：./configure --with-pic --enable-static
		编译：make
		编译成功后，在源码目录的.lib子目录下能够找到编译好的四个.a文件，也就是最终需要的文件。
		--with-pic是因为网关插件的动态库需要-fPIC选项支持。否则无法加载成功！
	windows：
		进入VS2015 x64本地工具命令提示符，
		在libevent源码目录下执行nmake -f Makefile.nmake
		编译成功后，在源码当前目录下能够找到三个编译好的.lib文件，也就是最终需要的文件。
	
2、zlib编译说明
	linux：
		配置：先执行 export CFLAGS=-fPIC， 然后执行./configure --static
		编译：make
		编译成功后，在源码目录下能够找到编译好的libz.a文件，也就是最终需要的文件。
	windows：
		启动VS2015开发工具，
		打开源码目录下\contrib\vstudio\vc14中的zlibvc.sln工程文件，
		选择工程配置为ReleaseWithoutAsm x64（无汇编支持的X64版本），然后编译工程zlibstat（这个是编译为静态库文件），
		编译成功后，按照VS2015的提示，源码目录下\contrib\vstudio\vc14\x64\ZlibStatReleaseWithoutAsm\zlibstat.lib就是最终需要的文件。
	注意：windows编译和linux编译会相互影响配置文件，每次编译前需要删除目录重新从SVN同步。
	编译需要依赖头文件：zconf.h zlib.h
	
3、openssl编译说明：
	linux:
		配置：在源码目录下执行./config -fPIC
		编译：make
		编译成功后，在源码目录下能够找到编译好的libssl.a和libcrypto.a文件，也就是最终需要的文件。
		编译需要依赖的头文件在源码目录下的include文件夹中；
	windows：
		1、安装ActivPerl 5.24；
		2、启动Visual Studio 2015 x64本地工具命令提示符；
		3、进入openssl源码目录，执行：
			perl Configure VC-WIN64A
			ms\do_win64a	#创建Makefile文件
			nmake -f ms\ntdll.mak  #编译动态库（等待时间较长），可选
			nmake -f ms\ntdll.mak clean  #清除上次OpenSSL动态库的编译
			nmake -f ms\nt.mak   #编译静态库（根据需要，选择是否编译静态库）
			nmake -f ms\nt.mak clean  #清除上次OpenSSL静态库的编译
		编译成功后，在源码目录下的out32目录中的ssleay32.lib和libeay32.lib就是最终需要的文件。
		编译需要依赖的头文件在源码目录下的inc32文件夹中。