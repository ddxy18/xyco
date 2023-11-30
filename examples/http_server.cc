#include <charconv>
#include <coroutine>
#include <sstream>
#include <unordered_map>

#include "xyco/utils/logger.h"

import xyco.runtime;
import xyco.task;
import xyco.io;
import xyco.net;
import xyco.fs;

// NOLINTNEXTLINE(bugprone-exception-escape)
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
    return std::to_string(std::to_underlying(method_)) + " " + url_ + " " +
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

// NOLINTNEXTLINE(bugprone-exception-escape)
class Request {
 public:
  RequestLine request_line_{""};
  std::unordered_map<std::string, std::string> headers_;
  std::string body_;
};

// NOLINTNEXTLINE(bugprone-exception-escape)
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
  Server(std::unique_ptr<xyco::runtime::Runtime> runtime, uint16_t port)
      : runtime_(std::move(runtime)), port_(port) {}

  auto run() -> void { runtime_->block_on(init_server()); }

 private:
  auto init_server() -> xyco::runtime::Future<void> {
    auto tcp_socket = *xyco::net::TcpSocket::new_v4();
    *tcp_socket.set_reuseaddr(true);
    *co_await tcp_socket.bind(xyco::net::SocketAddr::new_v4({}, port_));
    auto listener = *co_await tcp_socket.listen(LISTEN_BACKLOG);

    while (true) {
      auto server_stream = std::move((co_await listener.accept())->first);
      runtime_->spawn(receive_request(std::move(server_stream)));
    }
  }

  auto receive_request(xyco::net::TcpStream server_stream)
      -> xyco::runtime::Future<void> {
    auto reader = xyco::io::BufferReader<xyco::net::TcpStream, std::string>(
        &server_stream);
    Request request;
    auto line =
        *co_await xyco::io::BufferReadExt::read_line<decltype(reader),
                                                     std::string>(reader);
    if (line.length() == 0) {
      *co_await server_stream.shutdown(xyco::io::Shutdown::All);
      co_return;
    }
    request.request_line_ = RequestLine(line.substr(0, line.length() - 2));

    while (true) {
      line = *co_await xyco::io::BufferReadExt::read_line<decltype(reader),
                                                          std::string>(reader);
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
          *co_await server_stream.read(begin, end));
    }
    runtime_->spawn(execute_request(request, std::move(server_stream)));
  }

  static auto execute_request(Request request,
                              xyco::net::TcpStream server_stream)
      -> xyco::runtime::Future<void> {
    constexpr int SUCCESS_CODE = 200;
    constexpr int NOT_FOUND_CODE = 404;
    constexpr int SERVER_ERROR_CODE = 500;

    StatusLine status_line;
    status_line.version_ = request.request_line_.version_;
    if (request.request_line_.method_ == RequestLine::Method::Get) {
      auto open_file_result =
          (co_await xyco::fs::File::open(request.request_line_.url_));
      if (!open_file_result) {
        status_line.code_ = NOT_FOUND_CODE;
        status_line.reason_ = "ERROR";
      } else {
        status_line.code_ = SUCCESS_CODE;
        status_line.reason_ = "OK";
      }
      *co_await xyco::io::WriteExt::write_all(server_stream,
                                              status_line.to_string());

      if (!open_file_result) {
        std::string body = "<P>Your browser sent a bad request.\r\n";
        *co_await xyco::io::WriteExt::write_all(
            server_stream,
            "Content-Length: " + std::to_string(body.size()) + "\r\n");
        *co_await xyco::io::WriteExt::write_all(server_stream, "\r\n");
        *co_await xyco::io::WriteExt::write_all(server_stream, body);
        co_return;
      }
      auto file = *std::move(open_file_result);
      *co_await xyco::io::WriteExt::write_all(
          server_stream,
          "Content-Length: " + std::to_string(*co_await file.size()) + "\r\n");
      *co_await xyco::io::WriteExt::write_all(server_stream, "\r\n");
      auto reader = xyco::io::BufferReader<xyco::fs::File, std::string>(&file);
      std::string line = "a";
      while (!line.empty()) {
        line =
            *co_await xyco::io::BufferReadExt::read_line<decltype(reader),
                                                         std::string>(reader);
        *co_await xyco::io::WriteExt::write_all(server_stream, line);
      }
    } else {
      status_line.code_ = SERVER_ERROR_CODE;
      status_line.reason_ = "Internal Server Error";
      *co_await xyco::io::WriteExt::write_all(server_stream,
                                              status_line.to_string());

      std::string body = "<P>Unsupported method.\r\n";
      *co_await xyco::io::WriteExt::write_all(
          server_stream,
          "Content-Length: " + std::to_string(body.size()) + "\r\n");
      *co_await xyco::io::WriteExt::write_all(server_stream, "\r\n");
      *co_await xyco::io::WriteExt::write_all(server_stream, body);
    }
  }

  std::unique_ptr<xyco::runtime::Runtime> runtime_;
  int port_;

  static constexpr int LISTEN_BACKLOG = 5000;
};

// NOLINTNEXTLINE(bugprone-exception-escape)
auto main() -> int {
  constexpr uint16_t port = 8080;

  auto server = Server(*xyco::runtime::Builder::new_multi_thread()
                            .worker_threads(1)
                            .registry<xyco::task::BlockingRegistry>(1)
                            .registry<xyco::io::IoRegistry>(4)
                            .build(),
                       port);
  server.run();
}
