CC      = gcc
CFLAGS  = -Wall -Wextra -O2 -std=c99
LIBS    = -lcurl
TARGET  = jarvis
SRC     = main.c

.PHONY: all install uninstall clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LIBS)

install: $(TARGET)
	@echo "Installing to /usr/local/bin/$(TARGET)..."
	install -m 755 $(TARGET) /usr/local/bin/$(TARGET)
	@echo "Done! Run: jarvis"

uninstall:
	@echo "Removing /usr/local/bin/$(TARGET)..."
	rm -f /usr/local/bin/$(TARGET)
	@echo "Done."

clean:
	rm -f $(TARGET)
