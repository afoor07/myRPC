# Компилятор и флаги
CC = gcc
CFLAGS = -Wall -Wextra -O2 -D_GNU_SOURCE
LDFLAGS = -ljson-c

# Директории
SRCDIR = src
INCDIR = include
OBJDIR = obj
BINDIR = .

# Цели
TARGETS = myRPC-client myRPC-server
CLIENT_TARGET = myRPC-client
SERVER_TARGET = myRPC-server

# Исходные файлы
CLIENT_SRCS = $(SRCDIR)/client.c $(SRCDIR)/protocol.c
SERVER_SRCS = $(SRCDIR)/server.c $(SRCDIR)/config.c $(SRCDIR)/protocol.c

# Объектные файлы
CLIENT_OBJS = $(CLIENT_SRCS:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
SERVER_OBJS = $(SERVER_SRCS:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

# Правило по умолчанию
all: $(TARGETS)

# Сборка клиента
$(CLIENT_TARGET): $(CLIENT_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Сборка сервера
$(SERVER_TARGET): $(SERVER_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Компиляция объектных файлов
$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -I$(INCDIR) -c $< -o $@

# Создание директории для объектных файлов
$(OBJDIR):
	mkdir -p $(OBJDIR)

# Сборка deb-пакетов
deb: $(TARGETS)
	# Создание структуры для пакета клиента
	mkdir -p debian/client/usr/local/bin
	mkdir -p debian/client/DEBIAN
	cp $(CLIENT_TARGET) debian/client/usr/local/bin/
	
	# Создание control файла для клиента
	echo "Package: myrpc-client" > debian/client/DEBIAN/control
	echo "Version: 1.0" >> debian/client/DEBIAN/control
	echo "Section: utils" >> debian/client/DEBIAN/control
	echo "Priority: optional" >> debian/client/DEBIAN/control
	echo "Architecture: amd64" >> debian/client/DEBIAN/control
	echo "Depends: libc6 (>= 2.15), libjson-c3 (>= 0.11)" >> debian/client/DEBIAN/control
	echo "Maintainer: Bychkov <oleg07032007@gmail.com>" >> debian/client/DEBIAN/control
	echo "Description: myRPC client for remote command execution" >> debian/client/DEBIAN/control
	echo " Console utility to execute commands on myRPC server" >> debian/client/DEBIAN/control
	
	# Сборка пакета клиента
	dpkg-deb --build debian/client myrpc-client_1.0_amd64.deb
	
	# Создание структуры для пакета сервера
	mkdir -p debian/server/usr/local/bin
	mkdir -p debian/server/etc/myRPC
	mkdir -p debian/server/DEBIAN
	cp $(SERVER_TARGET) debian/server/usr/local/bin/
	[ -f config/myrpc.conf ] && cp config/myrpc.conf debian/server/etc/myRPC/ || true
	[ -f config/users.conf ] && cp config/users.conf debian/server/etc/myRPC/ || true
	
	# Создание control файла для сервера
	echo "Package: myrpc-server" > debian/server/DEBIAN/control
	echo "Version: 1.0" >> debian/server/DEBIAN/control
	echo "Section: net" >> debian/server/DEBIAN/control
	echo "Priority: optional" >> debian/server/DEBIAN/control
	echo "Architecture: amd64" >> debian/server/DEBIAN/control
	echo "Depends: libc6 (>= 2.15), libjson-c3 (>= 0.11)" >> debian/server/DEBIAN/control
	echo "Maintainer: Bychkov <oleg07032007@gmail.com>" >> debian/server/DEBIAN/control
	echo "Description: myRPC server daemon for remote command execution" >> debian/server/DEBIAN/control
	echo " Server daemon that executes commands from authorized users" >> debian/server/DEBIAN/control
	
	# Сборка пакета сервера
	dpkg-deb --build debian/server myrpc-server_1.0_amd64.deb

# Очистка
clean:
	rm -rf $(OBJDIR)
	rm -f $(TARGETS)
	rm -f *.deb
	rm -rf debian/client debian/server

# Установка
install: all
	sudo cp $(TARGETS) /usr/local/bin/
	sudo mkdir -p /etc/myRPC
	sudo cp config/*.conf /etc/myRPC/ 2>/dev/null || true

# Запуск тестов
test: all
	@echo "=== Running tests ==="
	@echo "1. Check if files exist:"
	ls -la myRPC-client myRPC-server
	@echo "2. Run client help:"
	./myRPC-client --help || true

# Очистка и пересборка
rebuild: clean all

.PHONY: all clean deb install test rebuild
