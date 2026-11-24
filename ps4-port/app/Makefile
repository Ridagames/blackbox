TITLE := BLACKBOX PS4
VERSION := 0.1.0
TITLE_ID := RGSB00001
CONTENT_ID := IV0000-RGSB00001_00-BLACKBOXPS400001

LIBS := -lc -lkernel -lc++ -lSceUserService -lSceVideoOut -lSceAudioOut -lScePad -lSceSysmodule -lSDL2
TOOLCHAIN := $(OO_PS4_TOOLCHAIN)
PROJDIR := blackbox
INTDIR := $(PROJDIR)/x64/Release
CXXFILES := $(wildcard $(PROJDIR)/*.cpp) $(wildcard $(PROJDIR)/modules/*.cpp)
OBJS := $(patsubst $(PROJDIR)/%.cpp,$(INTDIR)/%.o,$(CXXFILES))
CXXFLAGS := --target=x86_64-pc-freebsd12-elf -fPIC -funwind-tables -O2 -DNDEBUG -c -isysroot $(TOOLCHAIN) -isystem $(TOOLCHAIN)/include -isystem $(TOOLCHAIN)/include/c++/v1 -isystem $(TOOLCHAIN)/include/SDL2
LDFLAGS := -m elf_x86_64 -pie --script $(TOOLCHAIN)/link.x --eh-frame-hdr -L$(TOOLCHAIN)/lib $(LIBS) $(TOOLCHAIN)/lib/crt1.o

ifeq ($(shell uname -s),Darwin)
CXX := /usr/local/opt/llvm/bin/clang++
LD := /usr/local/opt/llvm/bin/ld.lld
CDIR := macos
else
CXX := clang++
LD := ld.lld
CDIR := linux
endif

.PHONY: all clean prepare
all: $(CONTENT_ID).pkg

prepare:
	mkdir -p $(INTDIR) sce_sys/about sce_module data
	test -s data/catalog.json
	cp "$(TOOLCHAIN)/samples/SDL2/sce_sys/about/right.sprx" sce_sys/about/right.sprx
	cp "$(TOOLCHAIN)/src/modules/libc.prx" sce_module/libc.prx
	cp "$(TOOLCHAIN)/src/modules/libSceFios2.prx" sce_module/libSceFios2.prx
	python3 -c "from PIL import Image; im=Image.open('../../assets/blackbox-logo.png').convert('RGB'); im=im.resize((512,512)); im.save('sce_sys/icon0.png',format='PNG',optimize=True)"

sce_sys/icon0.png sce_module/libc.prx sce_module/libSceFios2.prx: | prepare
	@test -s "$@"

$(INTDIR)/%.o: $(PROJDIR)/%.cpp | prepare
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -o $@ $<

sce_sys/param.sfo: Makefile | prepare
	$(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core sfo_new $@
	$(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core sfo_setentry $@ APP_TYPE --type Integer --maxsize 4 --value 1
	$(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core sfo_setentry $@ APP_VER --type Utf8 --maxsize 8 --value '$(VERSION)'
	$(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core sfo_setentry $@ ATTRIBUTE --type Integer --maxsize 4 --value 0
	$(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core sfo_setentry $@ CATEGORY --type Utf8 --maxsize 4 --value 'gd'
	$(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core sfo_setentry $@ CONTENT_ID --type Utf8 --maxsize 48 --value '$(CONTENT_ID)'
	$(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core sfo_setentry $@ DOWNLOAD_DATA_SIZE --type Integer --maxsize 4 --value 0
	$(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core sfo_setentry $@ SYSTEM_VER --type Integer --maxsize 4 --value 0
	$(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core sfo_setentry $@ TITLE --type Utf8 --maxsize 128 --value '$(TITLE)'
	$(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core sfo_setentry $@ TITLE_ID --type Utf8 --maxsize 12 --value '$(TITLE_ID)'
	$(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core sfo_setentry $@ VERSION --type Utf8 --maxsize 8 --value '$(VERSION)'

eboot.bin: $(OBJS)
	$(LD) $(OBJS) -o $(INTDIR)/$(PROJDIR).elf $(LDFLAGS)
	$(TOOLCHAIN)/bin/$(CDIR)/create-fself -in=$(INTDIR)/$(PROJDIR).elf -out=$(INTDIR)/$(PROJDIR).oelf --eboot "eboot.bin" --paid 0x3800000000000011

pkg.gp4: eboot.bin sce_sys/about/right.sprx sce_sys/param.sfo sce_sys/icon0.png sce_module/libc.prx sce_module/libSceFios2.prx data/catalog.json
	$(TOOLCHAIN)/bin/$(CDIR)/create-gp4 -out $@ --content-id=$(CONTENT_ID) --files "$^"
	python3 -c 'import xml.etree.ElementTree as E; p=E.parse("$@"); root=p.getroot().find("rootdir"); files=p.getroot().find("files"); exec("for f in files.findall(\"file\"):\n parts=f.get(\"targ_path\").split(\"/\")[:-1]\n node=root\n for part in parts:\n  child=next((d for d in node.findall(\"dir\") if d.get(\"targ_name\")==part),None)\n  if child is None: child=E.SubElement(node,\"dir\",{\"targ_name\":part})\n  node=child"); p.write("$@",encoding="utf-8",xml_declaration=True)'
	@echo '=== GENERATED GP4 PROJECT ==='
	@sed -n '1,240p' $@
	@echo '=== GP4 INPUT FILES ==='
	@for file in $^; do test -s "$$file" || { echo "MISSING: $$file"; exit 1; }; done

$(CONTENT_ID).pkg: pkg.gp4
	$(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core pkg_build $< .

clean:
	rm -rf $(PROJDIR)/x64 sce_sys sce_module eboot.bin pkg.gp4 $(CONTENT_ID).pkg
