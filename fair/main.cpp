#include "httplib.h"
#include "call_lantern.h"

#include <iostream>

using std::cerr, std::endl;

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Bad Args");
    }
    const auto port = std::strtol(argv[1], nullptr, 10);
    char *lantern_binary = argv[2];

    httplib::Server server;
    server.Get("/", [](const httplib::Request &req, httplib::Response &res) {
      res.set_content("fair-1.0", "text/plain");
    });

    server.Get("/run", [=](const httplib::Request &req, httplib::Response &res) {
      try {
          auto call = make_lantern_call(req, lantern_binary);
          auto args = call.freeze();
          for (const char **it = args; *it; it++) cerr << *it << " ";
          cerr << endl;
          auto result = collect_stdout(lantern_binary, const_cast<char *const *>(args));
          res.set_content(result, "text/plain");
      } catch (hsc_error &e) {
          res.status = 500;
          cerr << std::string("HscError: ") + std::to_string(static_cast<int>(e.error)) + " " + e.what() << endl;
          res.set_content(std::to_string(static_cast<int>(e.error)), "text/plain");
      } catch (std::exception &e) {
          res.status = 500;
          fprintf(stderr, "StdError:%s\n", e.what());
      } catch (...) {
          res.status = 500;
          fprintf(stderr, "UnknownErr\n");
      }
    });

    server.listen("0.0.0.0", static_cast<int>(port));
}