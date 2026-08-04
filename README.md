# HTTP/TCP-сервер
---
Bысоконагруженный асинхронный HTTP/TCP сервер c использованием системного вызова epoll
---
## Описание
- Все сетевые взаимодействия реализованы с помощью системных вызовов Linux POSIX API 
- Для отслеживания готовности сетевых сокетов к чтению/записи использовался epoll в режиме Edge-Triggered<br>
и с флагом EPOLLONESHOT для гарантии исключения DataRace
- Многопоточность реализована через ThreadPool
---
## Сборка
1. Создаём папку build и заходим в неё
```
mkdir build && cd build
```
2. Запускаем конфигурацию сборки (CMakeLists.txt лежит в родительской директории относительно build/)
```
cmake .. 
```
(Если до этого сборка была осуществлена с помощью -DSANITIZER=thread или -DSANITIZER=address<br>
то надо явно задать тип none, чтобы сборка была в режиме Release и без TSan/Asan)
```
cmake -DSANITIZER=none ..
```
3. Собираем проект
```
make
```
4. Запускаем
```
./server
```
---
## Тестирование
### Unit-Tests(GoogleTest)
В файле CMakeLists.txt через FetchContent скачивается репозиторий GoogleTest v14.0<br>tests/test_main.cpp тестирует модули parseHttpRequest и ThreadPool с использованием библиотеки <gtest/gtest.h>
Запуск
```
./run_tests
```
### Sanitizers
При проведении тестирования можно пользоваться Dynamic Analysis при помощи санитайзеров<br>
Чтобы включить данную опцию при анализе CMakeLists.txt надо передать следующие параметры<br>
Для использования Address Sanitizer (ASan):
```
cmake -DSANITIZER=address ..
```
Для использования Thread Sanitizer (TSan):
```
cmake -DSANITIZER=thread ..
```
### Стресс-тестирование
Нагрузочные тесты проведены с использованием утилиты wrk, которая запускалась со 100 открытыми соединения, 400 и 1000
```
wrk -t$(nproc) -c100 -d10s http://localhost:8080
```
<img width="757" height="568" alt="нагрукза_сервер_без_cout" src="https://github.com/user-attachments/assets/d89fd42e-f4f4-48d8-8e77-53e81556b35a" />
