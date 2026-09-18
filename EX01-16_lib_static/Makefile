TARGET = mycalStatic
OBJS = main.o
LIB_DIR = lib
LIB_NAME = mycal
LIB_TARGET = $(LIB_DIR)/lib$(LIB_NAME).a
LIB_OBJS = $(LIB_DIR)/plus.o $(LIB_DIR)/minus.o

CC = aarch64-linux-gnu-gcc
AR = aarch64-linux-gnu-ar
CFLAGS = -c -I. -I$(LIB_DIR)

all: $(TARGET)1 $(TARGET)2 $(TARGET)3
	@echo "build finished!"

# 방법 1: .a 파일을 직접 오브젝트 파일처럼 링킹
$(TARGET)1 : $(OBJS) $(LIB_TARGET)
	$(CC) -o $@ $^

# 방법 2: -L(디렉토리) 및 -l(라이브러리명) 옵션 사용
# GCC는 의존성 심볼을 왼쪽->오른쪽으로 해석하므로 OBJS 뒤에 -l 옵션이 와야 합니다.
$(TARGET)2 : $(OBJS) $(LIB_TARGET)
	$(CC) -o $@ $(OBJS) -L$(LIB_DIR) -l$(LIB_NAME)

# 방법 3: -static 옵션을 사용하여 정적 링킹 강제
$(TARGET)3 : $(OBJS) $(LIB_TARGET)
	$(CC) -o $@ $(OBJS) -L$(LIB_DIR) -l$(LIB_NAME) -static

$(LIB_TARGET): $(LIB_OBJS)
	$(AR) crv $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -o $@ $< 

clean:
	@rm -f $(TARGET)? $(OBJS) $(LIB_TARGET) $(LIB_OBJS)
	@echo "clean finished!"
