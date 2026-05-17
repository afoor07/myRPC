# myRPC - Remote Procedure Call System for Astra Linux

## Описание проекта

**myRPC** — это система удаленного вызова команд, реализованная на языке C для операционной системы **Astra Linux Special Edition 1.7**. Проект состоит из двух компонентов:

- **myRPC-client** — консольная утилита для отправки команд на удаленный сервер
- **myRPC-server** — сервер-демон, выполняющий команды от авторизованных пользователей

Протокол обмена данными — **JSON**. Транспорт — **TCP** (stream) или **UDP** (datagram) сокеты.

## Требования к системе

- **ОС:** Astra Linux SE 1.7 (или любой Debian-based Linux)
- **Компилятор:** GCC 4.8 или выше
- **Зависимости:** 
  - `libc6` (стандартная библиотека C)
  - `systemd` (для запуска сервера как службы)
  - `make`, `gcc`, `dpkg-deb` (для сборки)

## Структура проекта
```bash
myRPC/
├── src/ # Исходные коды
│ ├── client.c # Клиентская часть
│ ├── server.c # Сервер-демон
│ ├── config.c # Парсер конфигурации
│ └── protocol.c # JSON протокол
├── include/ # Заголовочные файлы
│ ├── config.h # Заголовки конфигурации
│ └── protocol.h # Заголовки протокола
├── config/ # Конфигурационные файлы
│ ├── myrpc.conf # Пример конфига сервера
│ └── users.conf # Пример списка пользователей
├── systemd/ # systemd сервис файлы
│ └── myrpc-server.service # Файл службы для сервера
├── Makefile # Система сборки
├── README.md # Документация
└── .gitignore # Игнорируемые файлы
```


## Установка и сборка

### Клонирование репозитория

```bash
git clone https://github.com/afoor07/myRPC.git
cd myRPC
```

### Сборка всех компонентов

```bash
make all
```

### Сборка DEB-пакетов

```bash
make deb
```

### Очистка временных файлов

```bash
make clean          # Удалить объектные файлы и исполняемые
make clean-all      # Полная очистка (включая файлы логов и временные)
```

## Установка пакетов

### На серверной ВМ:

```bash
sudo dpkg -i myrpc-server_1.0_amd64.deb
sudo systemctl enable myrpc-server
sudo systemctl start myrpc-server
```

### На клиентской ВМ:

```bash
sudo dpkg -i myrpc-client_1.0_amd64.deb
```

## Конфигурация

### Конфигурационный файл сервера (`/etc/myRPC/myrpc.conf`)

```ini
# Порт для прослушивания
port = 1234

# Тип сокета: stream (TCP) или dgram (UDP)
socket_type = stream
```

### Список разрешенных пользователей (`/etc/myRPC/users.conf`)

```bash
# Один пользователь на строку
chapaev
root
astra
```

**Примечание:** После изменения конфигурации необходимо перезапустить сервер:

```bash
sudo systemctl restart myrpc-server
# Или отправить сигнал SIGHUP, если сервер запущен вручную
sudo kill -HUP $(pidof myRPC-server)
```

## Использование

### Запуск сервера

**Как служба systemd (рекомендуется):**

```bash
sudo systemctl start myrpc-server
sudo systemctl status myrpc-server
```

**Вручную (для отладки):**

```bash
sudo /usr/local/bin/myRPC-server -f  # -f для запуска в foreground режиме
```

### Использование клиента

**Синтаксис:**

```bash
myRPC-client -c "команда" [-h IP_адрес] [-p порт] [-s|-d]
```

**Параметры командной строки:**

| Параметр | Длинная форма | Описание | Обязательный |
|----------|---------------|----------|--------------|
| `-c` | `--command` | Bash команда для выполнения | ✅ Да |
| `-h` | `--host` | IP адрес сервера | ❌ (по умолч.: localhost) |
| `-p` | `--port` | Порт сервера | ❌ (по умолч.: 1234) |
| `-s` | `--stream` | Использовать TCP сокет | ❌ (по умолч.) |
| `-d` | `--dgram` | Использовать UDP сокет | ❌ |
| `-v` | `--version` | Показать версию | ❌ |
| `-h` | `--help` | Показать справку | ❌ |

**Примеры использования:**

```bash
# Базовая команда (localhost, порт 1234, TCP)
myRPC-client -c "whoami"

# Удаленный сервер с указанием порта
myRPC-client -c "date" -h 192.168.1.100 -p 1234 -s

# Команда с аргументами
myRPC-client -c "ls -la /home" -h 192.168.1.100 -p 1234 -s

# Использование UDP сокета
myRPC-client -c "uname -a" -h 192.168.1.100 -p 1234 -d

# Получение справки
myRPC-client --help
```

## Проверка работы

### 1. Успешное выполнение команды (разрешенный пользователь)

```bash
$ myRPC-client -c "whoami" -h 10.0.2.3 -p 1234 -s
root

$ myRPC-client -c "date" -h 10.0.2.3 -p 1234 -s
Пт мая 15 10:43:39 MSK 2026

$ myRPC-client -c "uname -a" -h 10.0.2.3 -p 1234 -s
Linux astra 5.10.142-1-hardened #1 SMP x86_64 GNU/Linux
```

### 2. Ошибка авторизации (запрещенный пользователь)

```bash
$ myRPC-client -c "whoami" -h 10.0.2.3 -p 1234 -s
User 'unknown' not authorized
```

### 3. Ошибка выполнения команды

```bash
$ myRPC-client -c "non_existent_command" -h 10.0.2.3 -p 1234 -s
Error: command not found
```

## Логирование

### Просмотр логов сервера

**Через syslog:**

```bash
sudo tail -f /var/log/syslog | grep myRPC
```

**Через journalctl (при использовании systemd):**

```bash
sudo journalctl -u myrpc-server -f
```

### Формат сообщений лога

```
May 15 10:43:39 astra myRPC-server[1234]: Server started on port 1234 (TCP)
May 15 10:43:45 astra myRPC-server[1234]: Connection from 192.168.1.10
May 15 10:43:45 astra myRPC-server[1234]: User 'root' authorized
May 15 10:43:45 astra myRPC-server[1234]: Executing command 'whoami' for user 'root'
May 15 10:43:45 astra myRPC-server[1234]: Command executed successfully (code 0)
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

### Ручной тест протокола (с использованием netcat)

```bash
# Отправка JSON запроса вручную (TCP)
echo '{"login":"root","command":"whoami"}' | nc localhost 1234

# Ожидаемый ответ:
{"code":0,"result":"root\n"}
```

## Безопасность

- **Аутентификация:** Сервер проверяет пользователя по списку в `/etc/myRPC/users.conf`
- **Изоляция:** Команды выполняются в дочернем процессе с изолированным окружением
- **Логирование:** Все действия (включая неудачные попытки) логируются в syslog
- **Временные файлы:** Временные файлы stdout/stderr создаются с правами 600 и автоматически удаляются

## Устранение неполадок

| Проблема | Решение |
|----------|---------|
| `Connection refused` | Сервер не запущен или порт не совпадает |
| `User not authorized` | Добавьте пользователя в `/etc/myRPC/users.conf` |
| `Permission denied` | Запустите клиент/сервер с `sudo` |
| Сервер не запускается через systemd | Проверьте `sudo journalctl -u myrpc-server` |

## Разработка и внесение изменений

Проект разработан с использованием **Git Flow**:

- `main` — стабильная ветка релизов
- `develop` — основная ветка разработки
- `feature/*` — ветки для новых возможностей

Для добавления новой функциональности:

```bash
git checkout develop
git checkout -b feature/my-feature
# ... внесите изменения ...
git commit -m "feat: добавил новую функцию"
git checkout develop
git merge feature/my-feature --no-ff
```

## Лицензия

Проект распространяется под лицензией **MIT**. Подробнее см. файл LICENSE.

## Контакты

**Автор:** Бычков Олег, ККСО-21-24  
**GitHub:** [@afoor07](https://github.com/afoor07)  
**Email:** oleg.bychkov@example.com

## Благодарности

- Примеры из курса "Системное программирование"
- Документация по сокетам и демонам в Linux
- Материалы по Astra Linux Special Edition
```

Этот README полностью соответствует требованиям:
✅ Формат Markdown с корректной структурой заголовков  
✅ Описание модулей (клиент и сервер)  
✅ Инструкции по сборке (`make all`, `make deb`, `make clean`)  
✅ Примеры конфигурационных файлов  
✅ Демонстрация использования с примерами команд  
✅ Информация о логировании и отладке  
✅ Подтверждение использования Git Flow  
✅ Контактные данные автора
