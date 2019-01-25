#make:
#	iconv klen_copy.c -f utf8 -t koi8-r > klen_copy1.c
#	gcc klen_copy1.c -o copy -lcurses -lmenuw -lpanelw -lncursesw -rdynamic -ldl -static && ./copy ./ ~/work/ 123456

#flash:
#	gcc klen_copy.c -o copy -lmenuw -lpanelw -lncursesw -rdynamic -ldl -lgpm -static && ./copy ./ ../ 1
#mcbc:
#	iconv klen_copy.c -f utf8 -t koi8-r > klen_copy1.c
#	gcc klen_copy1.c libs/standart.c libs/mkiolib.c -lssl -o copy -lmenu -lpanel -lncurses -rdynamic -ldl -lgpm && ./copy ./ ../ 123456

CC = gcc
LINK = $(CC)

SRC = ./src
BUILD = ./bin

CFLAGS      = -g -O2

INCS = -I$(SRC) 

LDLIBS = -lmenuw -lpanelw -lncursesw -rdynamic -lcrypto -lssl

DEPS = $(SRC)/standart.h \
	$(SRC)/mkiolib.h

OBJS = $(BUILD)/klen_copy.o \
	$(BUILD)/standart.o \
	$(BUILD)/mkiolib.o 

BINS = $(BUILD)/klen_copy

BINS2 = klen_copy 

all: prebuild \
	$(BINS)


$(BUILD)/klen_copy: \
	$(BUILD)/klen_copy.o \
	$(OBJS)
	$(LINK) $(CFLAGS) -o $(BUILD)/klen_copy $(OBJS) $(LDLIBS)


$(BUILD)/klen_copy.o: $(DEPS) \
	$(SRC)/klen_copy.c
	$(CC) -c $(CFLAGS) $(INCS) -o $(BUILD)/klen_copy.o $(SRC)/klen_copy.c
	
$(BUILD)/standart.o: $(DEPS) \
	$(SRC)/standart.c
	$(CC) -c $(CFLAGS) $(INCS) -o $(BUILD)/standart.o $(SRC)/standart.c

$(BUILD)/mkiolib.o: $(DEPS) \
	$(SRC)/mkiolib.c
	$(CC) -c $(CFLAGS) $(INCS) -o $(BUILD)/mkiolib.o $(SRC)/mkiolib.c

clean:
	rm -rf $(BUILD)

prebuild:
	test -d $(BUILD) || mkdir -p $(BUILD)
