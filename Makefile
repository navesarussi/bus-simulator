CC      := cc
CFLAGS  := -std=c11 -D_POSIX_C_SOURCE=200809L -Wall -Wextra -Wpedantic -Werror -Iinclude
LDFLAGS :=

UNAME := $(shell uname)
LDFLAGS += -pthread
ifeq ($(UNAME), Linux)
  LDFLAGS += -lrt
endif

SRC_DIR := src
TEST_DIR := tests
TOOLS_DIR := tools
BUILD_DIR := build

LIB_SRCS := \
	$(SRC_DIR)/message.c \
	$(SRC_DIR)/scheduler.c \
	$(SRC_DIR)/bus.c \
	$(SRC_DIR)/shm_channel.c

LIB_OBJS := \
	$(BUILD_DIR)/message.o \
	$(BUILD_DIR)/scheduler.o \
	$(BUILD_DIR)/bus.o \
	$(BUILD_DIR)/shm_channel.o

TEST_BINS := \
	$(BUILD_DIR)/test_message \
	$(BUILD_DIR)/test_scheduler \
	$(BUILD_DIR)/test_shm_channel

TOOL_BINS := \
	$(BUILD_DIR)/race_demo \
	$(BUILD_DIR)/bench_shm

.PHONY: all test tools demo-race bench clean

all: test

test: $(TEST_BINS)
	./$(BUILD_DIR)/test_message
	./$(BUILD_DIR)/test_scheduler
	./$(BUILD_DIR)/test_shm_channel

tools: $(TOOL_BINS)

demo-race: $(BUILD_DIR)/race_demo
	./$(BUILD_DIR)/race_demo 200000 both

bench: $(BUILD_DIR)/bench_shm
	./$(BUILD_DIR)/bench_shm 500000

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/message.o: $(SRC_DIR)/message.c include/message.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $(SRC_DIR)/message.c -o $(BUILD_DIR)/message.o

$(BUILD_DIR)/scheduler.o: $(SRC_DIR)/scheduler.c include/scheduler.h include/message.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $(SRC_DIR)/scheduler.c -o $(BUILD_DIR)/scheduler.o

$(BUILD_DIR)/bus.o: $(SRC_DIR)/bus.c include/bus.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $(SRC_DIR)/bus.c -o $(BUILD_DIR)/bus.o

$(BUILD_DIR)/shm_channel.o: $(SRC_DIR)/shm_channel.c include/shm_channel.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $(SRC_DIR)/shm_channel.c -o $(BUILD_DIR)/shm_channel.o

$(BUILD_DIR)/test_message: $(LIB_OBJS) $(TEST_DIR)/test_message.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(TEST_DIR)/test_message.c $(LIB_OBJS) -o $(BUILD_DIR)/test_message $(LDFLAGS)

$(BUILD_DIR)/test_scheduler: $(LIB_OBJS) $(TEST_DIR)/test_scheduler.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(TEST_DIR)/test_scheduler.c $(LIB_OBJS) -o $(BUILD_DIR)/test_scheduler $(LDFLAGS)

$(BUILD_DIR)/test_shm_channel: $(LIB_OBJS) $(TEST_DIR)/test_shm_channel.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(TEST_DIR)/test_shm_channel.c $(LIB_OBJS) -o $(BUILD_DIR)/test_shm_channel $(LDFLAGS)

$(BUILD_DIR)/race_demo: $(LIB_OBJS) $(TOOLS_DIR)/race_demo.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(TOOLS_DIR)/race_demo.c $(LIB_OBJS) -o $(BUILD_DIR)/race_demo $(LDFLAGS)

$(BUILD_DIR)/bench_shm: $(LIB_OBJS) $(TOOLS_DIR)/bench_shm.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(TOOLS_DIR)/bench_shm.c $(LIB_OBJS) -o $(BUILD_DIR)/bench_shm $(LDFLAGS)

clean:
	rm -rf $(BUILD_DIR)
