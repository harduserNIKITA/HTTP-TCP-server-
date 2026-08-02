# HTTP/TCP-сервер
---
высоконагруженный асинхронный HTTP/TCP сервер, который использует встроенную системную функцию epoll
---
## сборка
1. Создаём папку build и заходим в неё
```
mkdir build && cd build
```
2. Запускаем конфигурацию сборки (CMakeLists.txt лежит в родительской директории относительно build/)
```
cmake .. 
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
## тестирование
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

