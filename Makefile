# Compiler settings
CC = gcc
CFLAGS = -Wall -DSQLITE_HAS_CODEC
LDFLAGS_STATIC = /usr/lib/x86_64-linux-gnu/libc.a /usr/lib/x86_64-linux-gnu/libcrypto.a  /usr/lib/x86_64-linux-gnu/libssl.a -static
LDFLAGS_DYNAMIC = -lcrypto -lssl -ldl

# Source files
SOURCES = main.c sqlite3.c libtct.c libstr.c
OBJECTS = $(SOURCES:.c=.o)
EXECUTABLE_STATIC = shvault_static
EXECUTABLE_DYNAMIC = shvault_dynamic

# Installation directory
PREFIX = /usr/local
BINDIR = $(PREFIX)/bin

# Default target builds both static and dynamic versions
all: static dynamic

# Static linking
static: $(SOURCES) $(EXECUTABLE_STATIC)

$(EXECUTABLE_STATIC): $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS_STATIC) -o $@

# Dynamic linking
dynamic: $(SOURCES) $(EXECUTABLE_DYNAMIC)

$(EXECUTABLE_DYNAMIC): $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS_DYNAMIC) -o $@

# General rule for compiling object files
.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

# Install Target
install: all
	@echo "Installing binaries to $(BINDIR)"
	@mkdir -p $(BINDIR)
	@cp -p $(EXECUTABLE_STATIC) $(EXECUTABLE_DYNAMIC) $(BINDIR)
	@echo "Installation completed."

# Clean Target
clean:
	rm -f $(OBJECTS) $(EXECUTABLE_STATIC) $(EXECUTABLE_DYNAMIC)

test: shvault_static
	# Test creating a new database and inserting an entry
	test "$(shell echo -n testvalue | ./shvault_static -v test.db -p testpassword -a testkey; echo $$?)" = "0"
	# Test searching for the inserted entry
	test "$(shell ./shvault_static -v test.db -p testpassword -s testkey)" = "testvalue"
	# Test updating an entry
	test "$(shell echo -n updatedvalue | ./shvault_static -v test.db -p testpassword -r testkey; echo $$?)" = "0"
	# Verify the update
	test "$(shell ./shvault_static -v test.db -p testpassword -s testkey)" = "updatedvalue"
	# Test deleting the entry
	test "$(shell ./shvault_static -v test.db -p testpassword -e testkey; echo $$?)" = "0"
	# Verify deletion
	test "$(shell ./shvault_static -v test.db -p testpassword -s testkey; echo $$?)" = "1"
	# Test rekeying the database
	test "$(shell ./shvault_static -v test.db -p testpassword -k newtestpassword; echo $$?)" = "0"
	# Verify rekey
	test "$(shell ./shvault_static -v test.db -p newtestpassword -l; echo $$?)" = "0"
	# Test inserting a command
	test "$(shell echo -n "date -I" | ./shvault_static -v test.db -p newtestpassword -a testcommand; echo $$?)" = "0"
	# Test executing command
	test "$(shell ./shvault_static -v test.db -p newtestpassword -c testcommand)" = "$(shell date -I)"
	# Clean up
	rm -f test.db
	@echo "ALL TESTS OK"
