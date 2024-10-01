### День 1: подключаем boost и Docker

В качестве курсача я буду писать микросервис по работе со статикой: фото, видео и т.п. У него будет крайне простой функционал: получать статику и раздавать ее. Такие сервисы требуют высокой производительности, поэтому их пишут на C++.

Ванильного C++ нам не хватит, потому что в нем нет много необходимого функционала. Так что, у приложения будет одна зависимость - библиотека boost, которая любезно предоставит средства для асинхронной обработки запросов и ряд небольших полезных фич.

Сегодня я займусь настройкой среды и контейнеризацией. Сразу скажу, что разработка будет вестись на MacOS (но большинство советов будет актуально и для Linux-пользователей) и не будет использоваться Visual Studio.

1) Установим boost локально:

```bash
brew install boost
```

Я делаю это, чтобы редактор мог мне делать подсказки и только. В моем случае, надо просто зайти в настройки VS Code и в настройках оболчки указать в includePath значение /opt/homebrew/include/**.

Будем считать, что все удобства разработки нам обеспечены 😇.

2) Docker:

Теперь самое важное. Нам надо обвернуть приложение в докер. Я предлагаю такой конфиг:

```docker
FROM ubuntu:latest

WORKDIR /app

RUN apt-get upgrade
RUN apt-get update && apt-get install -y \
    g++ \
    libcpprest-dev \
    libboost-all-dev \
    libssl-dev \
    cmake

COPY main.cpp .

RUN g++ -o main main.cpp -lcpprest -lboost_system -lboost_thread -lboost_chrono -lboost_random -lssl -lcrypto

EXPOSE 8080

CMD ["./main"]
```

В дальнейшем для запуска приложения достаточно будет:

```bash
docker build -t cppstaticserver .
docker run cppstaticserver
```

И все чудесным образом запуститься!

Так вот, для старта давайте запустим такую программу, которая при мощи boost генерирует uuid:

```cpp
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <iostream>

int main() {
  // Инициализация генератора UUID
  boost::uuids::random_generator generator;

  // Генерация UUID
  boost::uuids::uuid uuid = generator();

  // Вывод UUID в стандартный поток вывода
  std::cout << "UUID: " << uuid << std::endl;

  return 0;
}
```

Введем две команды выше и получим что-то типа такого:

```bash
alexander@MacBook-Pro-Alexander CppStaticServer % docker run mycppapp
UUID: 2e50e3e1-8f8c-48e1-82ad-3ab0e14f8c8b```

Ура! У нас есть полноценный конфиг для сервера!
```