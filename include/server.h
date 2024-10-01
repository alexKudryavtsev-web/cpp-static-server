#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <fstream>

using boost::asio::ip::tcp;

class session : public std::enable_shared_from_this<session>
{
public:
  session(tcp::socket socket) : socket_(std::move(socket)) {}

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

  void handle_request(const std::string &request)
  {
    std::string response;
    std::string method;
    std::string uri;

    std::istringstream request_stream(request);
    request_stream >> method >> uri;

    if (method == "POST" && uri == "/upload")
    {
      handle_upload(request_stream);
      response = "HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n";
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

  void handle_upload(std::istringstream &request_stream)
  {
    std::cout << "Uploaded" << std::endl;
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

class server
{
public:
  server(short port) : acceptor_(io_service_, tcp::endpoint(tcp::v4(), port))
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
            std::make_shared<session>(std::move(socket))->start();
          }
          do_accept();
        });
  }

  boost::asio::io_service io_service_;
  tcp::acceptor acceptor_;
};
