CC      = gcc
CFLAGS  = -Wall -Wextra -Wpedantic -std=c11 -Iinclude
LDFLAGS = -lm

PREFIX  ?= /usr/local
BINDIR  = $(PREFIX)/bin

SRC_DIR   = src
BUILD_DIR = build
BIN_DIR   = bin

SRC = $(wildcard $(SRC_DIR)/*.c)
OBJ = $(SRC:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

TARGET = $(BIN_DIR)/dns

SERVICE_NAME = dns
SYSTEMD_DIR  ?= /etc/systemd/system
SERVICE_SRC  = systemd/$(SERVICE_NAME).service
SERVICE_DST  = $(SYSTEMD_DIR)/$(SERVICE_NAME).service

.PHONY: all clean run test install install-service uninstall-service

all: $(TARGET)

$(TARGET): $(OBJ) | $(BIN_DIR)
	$(CC) $(OBJ) -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

install: $(TARGET)
	mkdir -p $(BINDIR)
	cp $(TARGET) $(BINDIR)/dns

install-service: install
	sudo cp $(SERVICE_SRC) $(SERVICE_DST)
	sudo systemctl daemon-reload
	sudo systemctl enable --now $(SERVICE_NAME).service

uninstall-service:
	- sudo systemctl disable --now $(SERVICE_NAME).service
	- sudo rm -f $(SERVICE_DST)
	sudo systemctl daemon-reload

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR) main.dSYM

test:
	@echo "no tests yet"
