export PROG	:= tunnel
SRC_DIR		:= ./src
export CC	:= gcc
export CFLAGS	:= -Werror -Wall -Wextra -DDEBUG -g

.PHONY: all
all:
	$(MAKE) -C $(SRC_DIR) all

.PHONY: install
install:
	$(MAKE) -C $(SRC_DIR) install

.PHONY: clean
clean:
	$(MAKE) -C $(SRC_DIR) clean

.PHONY: cleanall
cleanall:
	$(MAKE) -C $(SRC_DIR) cleanall

.PHONY: print_vars
print_vars:
	@echo "===print_vars==="
	@echo "PROG		= $(PROG)"
	@echo "SRC_DIR		= $(SRC_DIR)"
	@echo "CFLAGS		= $(CFLAGS)"
	@echo "LDFLAGS		= $(LDFLAGS)"
	$(MAKE) -C $(SRC_DIR) print_vars

