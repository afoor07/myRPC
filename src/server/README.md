# myRPC Server Module

Серверная часть системы удаленного вызова команд myRPC.

## Назначение

Программа-демон `myRPC-server` принимает входящие соединения от клиентов, аутентифицирует пользователей по списку допущенных, выполняет переданные bash-команды и возвращает результат в формате JSON.

## Файлы модуля

- `server.c` - основной исходный код сервера
- `config.c` - модуль парсинга конфигурационных файлов
- `protocol.c` - модуль обработки JSON-протокола
- `../include/config.h` - заголовочный файл конфигурации
- `../include/protocol.h` - заголовочный файл протокола

## Сборка

### Индивидуальная сборка сервера

```bash
# Из корня проекта
gcc -o myRPC-server src/server.c src/config.c src/protocol.c -Iinclude -Wall -Wextra

# Или с использованием Makefile
make server
```

### Сборка в составе всего проекта

```bash
make all
# или
make server
```

## Конфигурация

### Основной конфигурационный файл (`/etc/myRPC/myrpc.conf`)

```ini
# Порт для прослушивания
port = 1234

# Тип сокета: stream (TCP) или dgram (UDP)
socket_type = stream
```

### Файл списка пользователей (`/etc/myRPC/users.conf`)

```ini
# Один пользователь на строку
root
astra
username
```

## Режимы запуска

### 1. Как демон (рекомендуется для production)

```bash
# Запуск через systemd
sudo systemctl start myrpc-server

# Проверка статуса
sudo systemctl status myrpc-server

# Остановка
sudo systemctl stop myrpc-server

# Перезапуск
sudo systemctl restart myrpc-server
```

### 2. В интерактивном режиме (для отладки)

```bash
# Запуск в foreground режиме
sudo /usr/local/bin/myRPC-server -f

# С указанием альтернативного конфигурационного файла
sudo myRPC-server -c /path/to/custom.conf -f
```

### 3. Как обычный процесс (без демонизации)

```bash
sudo myRPC-server
```

## Обработка сигналов

Сервер корректно обрабатывает следующие сигналы:

| Сигнал | Действие | Логирование |
|--------|----------|-------------|
| `SIGINT` (Ctrl+C) | Graceful shutdown | Да |
| `SIGTERM` | Graceful shutdown | Да |
| `SIGHUP` | Перезагрузка конфигурации | Да |
| `SIGCHLD` | Очистка зомби-процессов | Да (опционально) |

### Пример отправки сигналов

```bash
# Найти PID сервера
pidof myRPC-server

# Отправить сигнал на перезагрузку конфигурации
sudo kill -HUP $(pidof myRPC-server)

# Остановить сервер
sudo kill -TERM $(pidof myRPC-server)
```

## Логирование

Сервер использует syslog для логирования всех событий.

### Просмотр логов

```bash
# Через syslog
sudo tail -f /var/log/syslog | grep myRPC

# Через journalctl (systemd)
sudo journalctl -u myrpc-server -f

# За последний час
sudo journalctl -u myrpc-server --since "1 hour ago"
```

### Формат сообщений лога

```
May 15 10:43:39 astra myRPC-server[1234]: Server started on port 1234 (TCP)
May 15 10:43:45 astra myRPC-server[1234]: Connection from 192.168.1.10:54321
May 15 10:43:45 astra myRPC-server[1234]: User 'root' authorized
May 15 10:43:45 astra myRPC-server[1234]: Executing command 'whoami' for user 'root'
May 15 10:43:45 astra myRPC-server[1234]: Command executed successfully (code 0)
May 15 10:43:45 astra myRPC-server[1234]: Connection closed
```

## Временные файлы

При выполнении команд сервер создает временные файлы:

- **stdout**: `/tmp/myRPC_XXXXXX.stdout`
- **stderr**: `/tmp/myRPC_XXXXXX.stderr`

Где `XXXXXX` - уникальный идентификатор.

### Особенности временных файлов

- Создаются с правами доступа `600` (только для владельца)
- Автоматически удаляются после отправки ответа клиенту
- Используют безопасную функцию `mkstemp()` для избежания race conditions

## Архитектура

### Обработка подключений

```
Клиент → Сокет → Сервер (главный процесс)
                      |
                      ├→ fork() → Дочерний процесс 1 → Выполнение команды
                      ├→ fork() → Дочерний процесс 2 → Выполнение команды
                      └→ fork() → Дочерний процесс N → Выполнение команды
```

### Поток обработки запроса

1. Прием входящего соединения
2. Чтение JSON-запроса
3. Проверка пользователя в `/etc/myRPC/users.conf`
4. Если авторизация пройдена:
   - Создание временных файлов
   - Выполнение команды через `popen()` или `fork()` + `exec()`
   - Сбор stdout и stderr
5. Формирование JSON-ответа
6. Отправка ответа клиенту
7. Очистка временных файлов
8. Закрытие соединения

## Производительность

### Ограничения

- **Максимальное количество одновременных подключений**: Ограничено ОС (обычно 1024)
- **Таймаут на выполнение команды**: Отсутствует (команда может выполняться бесконечно)
- **Размер ответа**: Ограничен размером буфера (обычно 64KB)

### Рекомендации по оптимизации

1. Использовать TCP вместо UDP для больших объемов данных
2. Ограничить время выполнения команд через `timeout`
3. Использовать `ulimit` для ограничения ресурсов дочерних процессов

## Безопасность

### Механизмы защиты

1. **Аутентификация**: Проверка пользователя по списку `/etc/myRPC/users.conf`
2. **Изоляция**: Команды выполняются в дочернем процессе
3. **Очистка окружения**: Перед выполнением команды сбрасываются переменные окружения
4. **Безопасные временные файлы**: Использование `mkstemp()`
5. **Логирование**: Все действия (включая неудачные попытки) логируются

### Рекомендации по усилению

1. Ограничить команды через whitelist в конфигурации
2. Использовать chroot для изоляции выполнения команд
3. Настроить firewall для ограничения доступа к порту сервера
4. Использовать SELinux (Astra Linux Special Edition)

## Установка из DEB-пакета

```bash
# Установка пакета
sudo dpkg -i myrpc-server_1.0_amd64.deb

# Настройка автозапуска
sudo systemctl enable myrpc-server

# Запуск сервера
sudo systemctl start myrpc-server
```

## Systemd сервис файл

Сервер устанавливает файл службы `/etc/systemd/system/myrpc-server.service`:

```ini
[Unit]
Description=myRPC Server Daemon
After=network.target

[Service]
Type=forking
PIDFile=/var/run/myrpc-server.pid
ExecStart=/usr/local/bin/myRPC-server
ExecReload=/bin/kill -HUP $MAINPID
Restart=on-failure

[Install]
WantedBy=multi-user.target
```

## Удаление

```bash
# Остановка и отключение сервиса
sudo systemctl stop myrpc-server
sudo systemctl disable myrpc-server

# Удаление пакета
sudo dpkg -r myrpc-server

# Полное удаление с конфигурацией
sudo dpkg --purge myrpc-server
```

## Отладка

### Проверка работы сервера

```bash
# Проверка, что сервер запущен
ps aux | grep myRPC-server

# Проверка, что порт слушается
sudo netstat -tlnp | grep 1234   # для TCP
sudo netstat -ulnp | grep 1234   # для UDP

# Проверка временных файлов
ls -la /tmp/myRPC_*
```

### Ручное тестирование

```bash
# TCP запрос
echo '{"login":"root","command":"whoami"}' | nc localhost 1234

# UDP запрос
echo '{"login":"root","command":"whoami"}' | nc -u localhost 1234
```

### GDB отладка

```bash
# Запуск в отладчике
sudo gdb --args myRPC-server -f

# Установка точек остановки
(gdb) break server.c:main
(gdb) break handle_connection
(gdb) run
```

## Устранение неполадок

| Проблема | Вероятная причина | Решение |
|----------|------------------|---------|
| Address already in use | Порт занят | Смените порт в конфиге или убейте процесс |
| Permission denied | Недостаточно прав | Запускайте с `sudo` |
| User not authorized | Пользователь не в списке | Добавьте в `/etc/myRPC/users.conf` |
| Cannot create temporary file | Нет прав на `/tmp` | Проверьте права на директорию `/tmp` |

## Лицензия

MIT License

## Автор

Бычков Олег, ККСО-21-24
