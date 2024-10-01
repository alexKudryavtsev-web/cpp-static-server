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
UUID: 2e50e3e1-8f8c-48e1-82ad-3ab0e14f8c8b
```

Ура! У нас есть полноценный конфиг для сервера!


### День 2: накатим архитектуру и запустим HTTP-сервер

1) Архитектура.

Перед тем как создавать HTTP-сервер стоит набросать архитектуру приложения. Ограничимся классической архитектурой. Напомню, что классическая архитектура - это же ее отсутствие в проекте. Данный подход обоснован тем, что профита от продвинутой архитектуры не будет в таком маленьком приложении. Наоборот, это усложнит для понимания проект разработчикам.

Останется просто разложить код по папкам. Я предлагаю вариант:

```tree
/src - исходные файлы
/include - заголовки
main.cpp - точка входа
```

Теперь нам придется поправить Dockerfile:

```docker
COPY main.cpp .
COPY src/ .
COPY include/ .
```

Как видно, я добавил еще два вызова COPY. Все содержимое src и include попадет в корневую папку проекта. Почему именно в корневую? Благодаря этому мы сможем обеспечить такой приятный импорт:

```cpp
#include "server.h"
```

За место:

```cpp
#include "include/server.h"
```

Тут все зависит от предпочтения. Однако если захочется использовать мой вариант, то надо будет прописать новые значения в includePath вашего редактора.

2) HTTP-сервер.

Введем сущности: Server - это прослойка над средствами boost, Server обрабатывает подключения пользователей и выдаем им Session - сессии, которые будут непосредственно обрабатывать запросы.

```cpp
class Server
{
public:
  Server(short port) : acceptor_(io_service_, tcp::endpoint(tcp::v4(), port))
  {
    do_accept();
  }

  void run()
  {
    io_service_.run();
  }

private:
  void do_accept()
  {
    acceptor_.async_accept(
        [this](boost::system::error_code ec, tcp::socket socket)
        {
          if (!ec)
          {
            std::make_shared<Session>(std::move(socket))->start();
          }
          do_accept();
        });
  }

  boost::asio::io_service io_service_;
  tcp::acceptor acceptor_;
};
```

Как видно, Server просто запускает boost-ский сервер и во время подключения раздает сессию. Session я предоставлю в укореченном виде:

```cpp
class Session : public std::enable_shared_from_this<Session>
{
public:
  Session(tcp::socket socket) : socket_(std::move(socket)) {}

  void start()
  {
    do_read();
  }

private:
  tcp::socket socket_;
  std::string request_;

 void do_read()
  {
    auto self(shared_from_this());
    request_.resize(1024);
    socket_.async_read_some(boost::asio::buffer(request_),
                            [this, self](boost::system::error_code ec, std::size_t length)
                            {
                              if (!ec)
                              {
                                handle_request(request_);
                              }
                            });
  }

  void do_write(const std::string &response)
  {
    auto self(shared_from_this());
    boost::asio::async_write(socket_, boost::asio::buffer(response),
                             [this, self](boost::system::error_code ec, std::size_t /*length*/)
                             {
                               if (!ec)
                               {
                                 socket_.close();
                               }
                             });
  }
};
```

Здесь опущенна реализация метода handle_request. Для тестирования сервера был создан простенький обработчик. Детальнее про роутинг и бизнес-логику будет завтра.

Сейчас же стоит обратить внимание на внутреннюю реализацию. Методом do_read будем считывать запрос пользователя, а методом do_write - отвечать ему. В обоих вариантах все это работает асинхронно.

Теперь в main.cpp достаточно всего лишь создать экземпляр Server и вызвать у него run:

```cpp
#include "server.h"
#include <iostream>

int main()
{
  try
  {
    std::cout << "Start app on 8080" << std::endl;
    Server s(8080);
    s.run();
  }
  catch (std::exception &e)
  {
    std::cerr << "Exception: " << e.what() << std::endl;
  }
  return 0;
}
```

Для запуска проекта сейчас необходимо указать, как будут маппиться порт:

```shell
docker run -p 8080:8080 cppapp
```

Ура! Теперь у нас есть почти весь boilerplate-код для проекта, а именно работающий HTTP-сервер!