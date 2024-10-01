#include <boost/asio.hpp>
#include <boost/asio.hpp>

#include <string>

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
  void do_read()
  {
    auto self = shared_from_this();
    socket_.async_read_some(
        boost::asio::buffer(data_, max_length),
        [this, self](boost::system::error_code ec, std::size_t length)
        {
          if (!ec)
          {
            std::string response = "HTTP/1.1 200 OK\r\n"
                                   "Content-Type: text/plain\r\n\r\n"
                                   "Hello from Boost Asio!";
            boost::asio::async_write(socket_, boost::asio::buffer(response),
                                     [this, self](boost::system::error_code ec, std::size_t /*length*/) {

                                     });
          }
        });
  }

  tcp::socket socket_;
  enum
  {
    max_length = 1024
  };
  char data_[max_length];
};

class server
{
public:
  server(short port) : io_service_(), acceptor_(io_service_, tcp::endpoint(tcp::v4(), port))
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
