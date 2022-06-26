#include <charconv>
#include <sstream>

#include "fs/epoll/file.h"
#include "fs/io_uring/file.h"
#include "io/buffer_reader.h"
#include "io/write.h"
#include "net/epoll/listener.h"
#include "net/io_uring/listener.h"
#include "runtime/runtime.h"

namespace net = xyco::net::uring;
namespace fs = xyco::fs::uring;
namespace io = xyco::io::uring;

class RequestLine {
 public:
  RequestLine(const std::string &line) {
    auto stream = std::stringstream(line);
    std::string method;
    stream >> method >> url_ >> version_;
    if (url_.ends_with('/')) {
      url_.append("index.html");
    }
    url_.insert(url_.begin(), '.');
    if (method == "GET") {
      method_ = Method::Get;
    }
    if (method == "HEAD") {
      method_ = Method::Head;
    }
    if (method == "POST") {
      method_ = Method::Post;
    }
    if (method == "PUT") {
      method_ = Method::Put;
    }
    if (method == "DELETE") {
      method_ = Method::Delete;
    }
    if (method == "CONNECT") {
      method_ = Method::Connect;
    }
    if (method == "OPTIONS") {
      method_ = Method::Options;
    }
    if (method == "TRACE") {
      method_ = Method::Trace;
    }
    if (method == "PATCH") {
      method_ = Method::Patch;
    }
  }

  [[nodiscard]] auto to_string() const -> std::string {
    return std::to_string(std::__to_underlying(method_)) + " " + url_ + " " +
           version_;
  }

  enum class Method {
    Get,
    Head,
    Post,
    Put,
    Delete,
    Connect,
    Options,
    Trace,
    Patch
  };

  Method method_;
  std::string url_;
  std::string version_;
};

class Request {
 public:
  RequestLine request_line_{""};
  std::unordered_map<std::string, std::string> headers_;
  std::string body_;
};

class StatusLine {
 public:
  [[nodiscard]] auto to_string() const -> std::string {
    return version_ + " " + std::to_string(code_) + " " + reason_ + "\r\n";
  }

  std::string version_;
  int code_{};
  std::string reason_;
};

class Server {
 public:
  Server(std::unique_ptr<xyco::runtime::Runtime> runtime, const char *ip_addr,
         uint16_t port)
      : runtime_(std::move(runtime)) {
    auto init_server = [=](const char *ip_addr,
                           uint16_t port) -> xyco::runtime::Future<void> {
      auto tcp_socket = net::TcpSocket::new_v4().unwrap();
      tcp_socket.set_reuseaddr(true).unwrap();
      (co_await tcp_socket.bind(xyco::net::SocketAddr::new_v4(ip_addr, port)))
          .unwrap();
      auto listener = (co_await tcp_socket.listen(LISTEN_BACKLOG)).unwrap();

      while (true) {
        auto server_stream = (co_await listener.accept()).unwrap().first;
        runtime_->spawn(receive_request(std::move(server_stream)));
      }
    };
    runtime_->spawn(init_server(ip_addr, port));
  }

 private:
  auto receive_request(net::TcpStream server_stream)
      -> xyco::runtime::Future<void> {
    auto reader =
        xyco::io::BufferReader<net::TcpStream, std::string>(server_stream);
    Request request;
    auto line =
        (co_await xyco::io::BufferReadExt::read_line<decltype(reader),
                                                     std::string>(reader))
            .unwrap();
    if (line.length() == 0) {
      (co_await server_stream.shutdown(xyco::io::Shutdown::All)).unwrap();
      co_return;
    }
    request.request_line_ = RequestLine(line.substr(0, line.length() - 2));

    while (true) {
      line = (co_await xyco::io::BufferReadExt::read_line<decltype(reader),
                                                          std::string>(reader))
                 .unwrap();
      if (line == "\r\n") {
        break;
      }
      request.headers_.emplace(
          line.substr(0, line.find(':')),
          line.substr(line.find(':') + 2, line.length() - 2));
    }
    std::string headers;
    for (auto &header : request.headers_) {
      headers += header.first + ": " + header.second + "\r\n";
    }

    int content_length = 0;
    if (request.headers_.find("Content-Length") != request.headers_.end()) {
      auto content_length_str = request.headers_.find("Content-Length")->second;
      std::from_chars(content_length_str.begin().base(),
                      content_length_str.end().base(), content_length);
    }
    request.body_.resize(content_length);
    auto begin = request.body_.begin();
    auto end = request.body_.end();
    while (begin != end) {
      begin += static_cast<decltype(begin)::difference_type>(
          (co_await server_stream.read(begin, end)).unwrap());
    }
    runtime_->spawn(execute_request(request, std::move(server_stream)));
  }

  static auto execute_request(Request request, net::TcpStream server_stream)
      -> xyco::runtime::Future<void> {
    constexpr int SUCCESS_CODE = 200;
    constexpr int NOT_FOUND_CODE = 404;
    constexpr int SERVER_ERROR_CODE = 500;

    StatusLine status_line;
    status_line.version_ = request.request_line_.version_;
    if (request.request_line_.method_ == RequestLine::Method::Get) {
      auto open_file_result =
          (co_await fs::File::open(request.request_line_.url_));
      if (open_file_result.is_err()) {
        status_line.code_ = NOT_FOUND_CODE;
        status_line.reason_ = "ERROR";
      } else {
        status_line.code_ = SUCCESS_CODE;
        status_line.reason_ = "OK";
      }
      (co_await xyco::io::WriteExt::write_all(server_stream,
                                              status_line.to_string()))
          .unwrap();

      if (open_file_result.is_err()) {
        std::string body = "<P>Your browser sent a bad request.\r\n";
        (co_await xyco::io::WriteExt::write_all(
             server_stream,
             "Content-Length: " + std::to_string(body.size()) + "\r\n"))
            .unwrap();
        (co_await xyco::io::WriteExt::write_all(server_stream, "\r\n"))
            .unwrap();
        (co_await xyco::io::WriteExt::write_all(server_stream, body)).unwrap();
        co_return;
      }
      auto file = open_file_result.unwrap();
      (co_await xyco::io::WriteExt::write_all(
           server_stream, "Content-Length: " +
                              std::to_string((co_await file.size()).unwrap()) +
                              "\r\n"))
          .unwrap();
      (co_await xyco::io::WriteExt::write_all(server_stream, "\r\n")).unwrap();
      auto reader = xyco::io::BufferReader<fs::File, std::string>(file);
      std::string line = "a";
      while (!line.empty()) {
        line =
            (co_await xyco::io::BufferReadExt::read_line<decltype(reader),
                                                         std::string>(reader))
                .unwrap();
        (co_await xyco::io::WriteExt::write_all(server_stream, line)).unwrap();
      }
    } else {
      status_line.code_ = SERVER_ERROR_CODE;
      status_line.reason_ = "Internal Server Error";
      (co_await xyco::io::WriteExt::write_all(server_stream,
                                              status_line.to_string()))
          .unwrap();

      std::string body = "<P>Unsupported method.\r\n";
      (co_await xyco::io::WriteExt::write_all(
           server_stream,
           "Content-Length: " + std::to_string(body.size()) + "\r\n"))
          .unwrap();
      (co_await xyco::io::WriteExt::write_all(server_stream, "\r\n")).unwrap();
      (co_await xyco::io::WriteExt::write_all(server_stream, body)).unwrap();
    }
  }

  std::unique_ptr<xyco::runtime::Runtime> runtime_;

  static constexpr int LISTEN_BACKLOG = 5000;
};

auto main() -> int {
  const char *ip_addr = "127.0.0.1";
  constexpr uint16_t port = 8080;

  auto server = Server(xyco::runtime::Builder::new_multi_thread()
                           .worker_threads(1)
                           .max_blocking_threads(1)
                           .registry<io::IoRegistry>(4)
                           .build()
                           .unwrap(),
                       ip_addr, port);

  while (true) {
    std::this_thread::sleep_for(std::chrono::seconds(3));
  }
}