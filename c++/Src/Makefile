SUBDIRS = live_common live_logging live_literal live_media live_input live_output live_codec live_vpf live_network live_rtmp live_rtmp2 
SUBDIRS += live_source_file live_source_onvif live_source_hxht live_source_rtmp live_sink_file live_sink_flv live_codec_nal
SUBDIRS += live_comet live_gateway

# 需要排除的目录
exclude_dirs := include bin Debug Release log live_libevent live_demo
# 取得当前子目录深度为1的所有目录名称
dirs := $(shell find . -maxdepth 1 -type d)
dirs := $(basename $(patsubst ./%,%,$(dirs)))
dirs := $(filter-out $(exclude_dirs),$(dirs))
# 避免clean子目录操作同名，加上_clean_前缀
clean_dirs := $(addprefix _clean_,SUBDIRS )

SUBDIRS := $(SUBDIRS)
all:subdirs
subdirs:
	for n in $(SUBDIRS);\
		do $(MAKE) -C $$n || exit 1 ; \
		done ; 
#	./live_gateway/gateway 

clean:
	for n in $(SUBDIRS); \
		do $(MAKE) -C $$n clean; \
		done
