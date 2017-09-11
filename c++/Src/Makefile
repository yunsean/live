SUBDIRS =live_common live_logging live_literal live_media live_input live_source_file live_vpf live_demo
# 需要排除的目录
exclude_dirs := include bin Debug Release log live_libevent
# 取得当前子目录深度为1的所有目录名称
dirs := $(shell find . -maxdepth 1 -type d)
dirs := $(basename $(patsubst ./%,%,$(dirs)))
dirs := $(filter-out $(exclude_dirs),$(dirs))
# 避免clean子目录操作同名，加上_clean_前缀
clean_dirs := $(addprefix _clean_,$(SUBDIRS) )

SUBDIRS := $(dirs)
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
