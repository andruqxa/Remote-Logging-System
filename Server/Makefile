# ------------------------------------------------------------------------------
# DIRECTOARE
# ------------------------------------------------------------------------------

SRC_DIR = src
INC_DIR = include
BUILD_DIR = build


# ------------------------------------------------------------------------------
# COMPILATOR SI OPTIUNI
# ------------------------------------------------------------------------------

CC = gcc
CFLAGS = -Wall -Wextra -g -pthread -I$(INC_DIR)
LDFLAGS = -pthread


# ------------------------------------------------------------------------------
# FISIERE
# ------------------------------------------------------------------------------

TARGET = LogServer

SOURCES = $(wildcard $(SRC_DIR)/*.c)
HEADERS = $(wildcard $(INC_DIR)/*.h)
OBJECTS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SOURCES))


# ------------------------------------------------------------------------------
# REGULI
# ------------------------------------------------------------------------------

.PHONY: all clean rebuild help dirs

all: dirs $(TARGET)
	@echo ""
	@echo "=========================================="
	@echo " Compilare completa!"
	@echo " Ruleaza cu: ./$(TARGET)"
	@echo "=========================================="

dirs:
	@mkdir -p $(BUILD_DIR)

$(TARGET): $(OBJECTS)
	@echo "[LINK] Creez executabilul $(TARGET)..."
	@$(CC) $(OBJECTS) -o $@ $(LDFLAGS)
	@echo "[OK] Executabil creat: $(TARGET)"

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c $(HEADERS)
	@echo "[CC] Compilez $<..."
	@$(CC) $(CFLAGS) -c $< -o $@

clean:
	@echo "[CLEAN] Sterg fisierele compilate..."
	@rm -rf $(BUILD_DIR) $(TARGET)
	@echo "[OK] Curatat!"

rebuild: clean all