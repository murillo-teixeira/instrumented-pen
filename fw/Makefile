BOARD := esp32:esp32:XIAO_ESP32C3
PORT := COM8
SKETCH := fw.ino
BUILD_DIR := build

.PHONY: all compile upload clea

all: compile upload clean serial

compile:
	arduino-cli compile --fqbn $(BOARD) --output-dir $(BUILD_DIR) $(SKETCH)

upload: compile
	arduino-cli upload -p $(PORT) --fqbn $(BOARD) --input-dir $(BUILD_DIR)

serial:
	arduino-cli monitor -p $(PORT)

clean:
	del /Q /F $(BUILD_DIR)
	rmdir /Q /S $(BUILD_DIR)
