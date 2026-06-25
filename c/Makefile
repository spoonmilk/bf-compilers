CC      = gcc
CFLAGS  = -Wall -Wextra -O2
DBGCFLAGS  = -Wall -Wextra -O0
TARGET  = bf_compile
SRCS    = compile.c lexer.c

.PHONY: build compile clean

build: $(TARGET)

$(TARGET): $(SRCS) lexer.h
	$(CC) $(CFLAGS) -o $@ $(SRCS)

debug: $(TARGET)

$(TARGET): $(SRCS) lexer.h
	$(CC) -g $(DBGCFLAGS) -o $@ $(SRCS)


compile: $(TARGET)
ifndef BF
	$(error No BF: make compile BF=<input.bf> OUT=<output.wasm>)
endif
ifndef OUT
	$(error No OUT: make compile BF=<input.bf> OUT=<output.wasm>)
endif
	$(eval WAT := $(basename $(OUT)).wat)
	./$(TARGET) $(BF) $(WAT)
	wat2wasm $(WAT) -o ./out/$(OUT)
	rm -f $(WAT)

clean:
	rm -f $(TARGET) *.wat *.wasm
