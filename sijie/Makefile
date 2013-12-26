TARGET = player
CC = g++

CFLAGS += -I. -I/usr/local/include -g -Wall  -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_ISOC9X_SOURCE

LDFLAGS += -lavdevice  -lavfilter -lswresample -lavformat -lavcodec -lavutil -lswscale -lGLU -lGL -lm -lz -lpthread -lX11 -lSDL

SRC=$(wildcard *.cpp)
OBJS=${SRC:%.cpp=%.o}
NAME=${SRC:%.cpp=%}
DEPS=$(SRC:%.cpp=.dep/%.d)

.PHONY: dep  all

all: $(OBJS)
	$(CC) -o $(TARGET) $(OBJS) $(LDFLAGS) 
#	$(STRIP) $(TARGET)


clean:
	rm -rf *.o $(TARGET) .dep

%.o: %.cpp
	${CC} ${CFLAGS} -c $<
	@mkdir -p .dep
	${CC} -MM $(CFLAGS) $*.cpp > .dep/$*.d 


dep: 
	@mkdir -p .dep
	for i in ${NAME} ; do  \
		${CC} -MM $(CFLAGS) "$${i}".cpp > .dep/"$${i}".d ;\
	done

