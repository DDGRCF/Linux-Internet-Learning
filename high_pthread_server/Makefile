CURDIR = $(shell pwd)
UTILSDIR = $(CURDIR)/../utils

src = $(wildcard $(CURDIR)/*.cc)
utils_src = $(wildcard $(UTILSDIR)/*.cc)

src += $(utils_src)
obj = $(patsubst %.cc, %.o, $(src))

INCLUDES += -I$(CURDIR)/
INCLUDES += -I$(UTILSDIR)/

dir = $(notdir $(src)) # 去除路径，要不然打印地很长

$(info Src: $(src))
$(info Obj: $(obj))
$(info NotDir: $(dir))

ALL: server

$(obj): %.o:%.cc
	g++ -c $< -o $@ $(INCLUDES)

server: $(obj)
	g++ -g $^ -o $@


clean:
	-rm -rf $(obj) server

.PHONY: clean ALL