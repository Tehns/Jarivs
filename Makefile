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
	@echo "Installing jarvis to /usr/local/bin..."
	install -m 755 $(TARGET) /usr/local/bin/$(TARGET)
	@echo "Done. Run: jarvis"

uninstall:
	rm -f /usr/local/bin/$(TARGET)
	@echo "Uninstalled."

clean:
	rm -f $(TARGET)
