AR = $(CROSS_COMPILE)ar
CC = $(CROSS_COMPILE)gcc$(CC_VERSION)

OBJ_DIR = obj
SRC_DIR = src
BIN_DIR = bin
LIB_DIR = lib
TEST_DIR = test/src
EXAMPLES_DIR = examples

LDFLAGS =
ARFLAGS = -r
CCFLAGS = -Wall -Wextra -Werror -Wswitch-default -Wwrite-strings \
	-O3 -Iinclude -Itest/include -fno-stack-protector $(CFLAGS)

CHLOROS_C_SRCS = main.c thread.c
CHLOROS_S_SRCS = context_switch.S
CHLOROS_OBJS = $(CHLOROS_C_SRCS:%.c=$(OBJ_DIR)/%.o) $(CHLOROS_S_SRCS:%.S=$(OBJ_DIR)/%.o)

TEST_SRCS = test.c test_utils.c \
	phase1_tests.c phase2_tests.c phase3_tests.c phase4_tests.c phase5_tests.c \
	phase6_tests.c argument_tests.c join_tests.c

TEST_OBJS = $(TEST_SRCS:%.c=$(OBJ_DIR)/%.o)

EXAMPLES_SRCS = $(wildcard $(EXAMPLES_DIR)/*.c)
EXAMPLES_BINS = $(EXAMPLES_SRCS:$(EXAMPLES_DIR)/%.c=$(BIN_DIR)/examples/%)

LIB_NAME = chloros
LIB = $(LIB_DIR)/lib$(LIB_NAME).a
TEST_BIN = $(BIN_DIR)/test

.PHONY: all clean test submission

vpath % $(SRC_DIR) $(TEST_DIR) $(EXAMPLES_DIR)

$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CCFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: %.S
	@mkdir -p $(@D)
	$(CC) $(CCFLAGS) -c $< -o $@

$(LIB): $(CHLOROS_OBJS)
	@mkdir -p $(@D)
	$(AR) $(ARFLAGS) $@ $^

$(TEST_BIN): $(TEST_OBJS) $(LIB)
	@mkdir -p $(@D)
	$(CC) $(LDFLAGS) -L$(LIB_DIR) -lchloros -o $@ $^

$(BIN_DIR)/examples/%: $(EXAMPLES_DIR)/%.c $(LIB)
	@mkdir -p $(@D)
	$(CC) $(CCFLAGS) $(LDFLAGS) -o $@ $< -L$(LIB_DIR) -lchloros

all: $(LIB)

test: $(TEST_BIN)
	@$(TEST_BIN)

examples: $(EXAMPLES_BINS)

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR) $(LIB_DIR)
