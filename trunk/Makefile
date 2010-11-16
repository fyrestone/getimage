##工程名字
PROJECT = GetImage
EXECUTABLE = $(PROJECT).exe

##临时文件目录
OBJ_DIR = Obj
DEBUG_DIR = Debug
RELEASE_DIR = Release

##编译器选项
CC = gcc
DEBUG_DEF = -D_DEBUG
RELEASE_DEF = -DNDEBUG
DEBUG_FLAGS = -O0 -g -Wall $(DEBUG_DEF)
RELEASE_FLAGS = -O2 -fno-strict-aliasing -Wall $(RELEASE_DEF) #-fno-strict-aliasing参数绕过strict aliasing优化
EXTENDS_FLAGS = -dumpversion

##中间文件
OBJECTS = GetImage.o Process.o ISOProcess.o IMGProcess.o MapFile.o ColorPrint.o SafeMemory.o
ARG_PARSER_OBJECT = carg_parser.o

##对外接口
.PHONY : all debug release clean cleanall

all : debug release

#针对不同目标对FLAGS变量赋值，必须单独一行。
debug : FLAGS = $(DEBUG_FLAGS)
debug : OBJ_DIR := $(OBJ_DIR)/$(DEBUG_DIR)
debug : create compile

	
#针对不同目标对FLAGS变量赋值，必须单独一行。
release : FLAGS = $(RELEASE_FLAGS)
release : OBJ_DIR := $(OBJ_DIR)/$(RELEASE_DIR)
release : create compile
	
##创建文件夹
create :
	-mkdir $(subst /,\,$(OBJ_DIR))
	
##编译，内部目标
compile : $(OBJECTS) $(ARG_PARSER_OBJECT)
	$(CC) -o $(EXECUTABLE) $(addprefix $(OBJ_DIR)/, $(OBJECTS) $(ARG_PARSER_OBJECT)) $(FLAGS)

##只清除中间文件
clean :
	-rmdir $(OBJ_DIR) /s /q
	
##清除所有
cleanall :
	-rmdir $(OBJ_DIR) /s /q
	-del $(EXECUTABLE)

##所有的中间文件.o生成命令
$(OBJECTS) : 
	$(CC) -o $(OBJ_DIR)/$@ -c $(subst .o,.c,$@) $(FLAGS)
	
$(ARG_PARSER_OBJECT) : #生成ArgParser.o
	$(CC) -o $(OBJ_DIR)/$@ -c ArgParser/$(subst .o,.c,$@) $(FLAGS)

