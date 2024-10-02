#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <fstream>
#include <filesystem>

using boost::asio::ip::tcp;

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
    request_.resize(100000);
    socket_.async_read_some(boost::asio::buffer(request_),
                            [this, self](boost::system::error_code ec, std::size_t length)
                            {
                              if (!ec)
                              {
                                handle_request(request_);
                              }
                            });
  }

  void handle_request(const std::string &request)
  {
    std::string response;
    std::string method;
    std::string uri;

    std::istringstream request_stream(request);
    request_stream >> method >> uri;

    if (method == "POST" && uri == "/upload")
    {
      response = handle_upload(request_stream);
    }
    else if (method == "GET" && uri.length() > 1 && uri.find('/') == 0)
    {
      std::string id = uri.substr(1);
      response = handle_get(id);
    }
    else
    {
      response = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
    }

    do_write(response);
  }

  std::string handle_upload(std::istringstream &request_stream)
  {
    std::string line;
    std::string boundary;

    while (std::getline(request_stream, line) && line != "\r")
    {
      if (line.find("Content-Type: multipart/form-data;") != std::string::npos)
      {
        size_t pos = line.find("boundary=");
        if (pos != std::string::npos)
        {
          boundary = "--" + line.substr(pos + 9); // 9 - len "boundary="
        }
      }
    }

    std::ostringstream body_stream;
    while (std::getline(request_stream, line))
    {
      body_stream << line << "\n";
    }

    std::string body = body_stream.str();

    return save_file(body, boundary);
  }

  std::string save_file(const std::string &body, const std::string &boundary)
  {
    size_t start = body.find(boundary);
    size_t end = body.length();

    if (start != std::string::npos && end != std::string::npos)
    {
      start += boundary.length();
      size_t filename_start = body.find("filename=\"", start) + 10; // 10 - длина "filename=\""
      size_t filename_end = body.find("\"", filename_start);
      std::string filename = body.substr(filename_start, filename_end - filename_start);

      // Определяем путь к файлу
      std::filesystem::path storage_path = "./storage/" + filename;

      // Сохраняем файл
      std::ofstream ofs(storage_path, std::ios::binary);
      ofs.write(body.data(), body.length());
      ofs.close();

      return "HTTP/1.1 200 OK\r\n\r\nFile uploaded successfully.";
    }
    else
    {
      return "HTTP/1.1 400 Bad Request\r\n\r\nCould not parse the file.";
    }
  }

  std::string handle_get(const std::string &id)
  {
    return "HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n";
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
