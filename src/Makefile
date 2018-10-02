PROJECT_NAME     := con-check

SRC_CFILES += \
  trace_log.c \

SRC_CPPFILES += \
  main.cpp \

INC_FOLDERS += \
  ./ \

LIB_FILES += \
  oping \
  confuse \

CFLAGS += -O0
CFLAGS += -g3
CFLAGS += -Wall
CFLAGS += -c
CFLAGS += -fmessage-length=0

INST_PATH = /usr/local/bin/

OBJ_FILES = $(SRC_CFILES:.c=.o) $(SRC_CPPFILES:.cpp=.o)

default: all

clean:
	rm -f $(PROJECT_NAME)
	rm -f $(OBJ_FILES)

all: build

install: $(PROJECT_NAME)
	cp -f $(PROJECT_NAME) $(INST_PATH)
	mkdir -p /etc/con-check/
	cp -f con-check.conf /etc/con-check/
	cp -f offline.sh /etc/con-check/
	cp -f online.sh /etc/con-check/

uninstall:
	rm -f $(INST_PATH)$(PROJECT_NAME)
	rm -Rf /etc/con-check

build: $(PROJECT_NAME)

 
$(PROJECT_NAME): $(OBJ_FILES)
	gcc $(OBJ_FILES) -o $@ $(addprefix -l, $(LIB_FILES))


%.o:%.c
	gcc $(CFLAGS) $(CFLAGS) $(addprefix -I, $(INC_FOLDERS)) $< -o $@

%.o:%.cpp
	g++ $(CFLAGS) $(CFLAGS) $(addprefix -I, $(INC_FOLDERS)) $< -o $@