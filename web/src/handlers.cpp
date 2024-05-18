#include "handlers.hpp"

#include <fstream>
#include <memory>
#include <optional>
#include <string>

#include "config.hpp"
#include "crow/http_response.h"
#include "crow/mustache.h"
#include "maddy/parser.h"

namespace botweb {
  crow::response get_wiki_page(const std::string &path,
                               const std::optional<Configuration> &cfg) {
    std::shared_ptr<maddy::Parser> parser = std::make_shared<maddy::Parser>();

    std::ifstream contents("docs/" + path + ".md");
    std::string contents_html = parser->Parse(contents);
    contents.close();

    std::ifstream summary("docs/summary.md");
    std::string summary_html = parser->Parse(summary);
    summary.close();

    auto page = crow::mustache::load("wiki_page.html");

    crow::mustache::context ctx;
    ctx["content"] = contents_html;
    ctx["summary"] = summary_html;
    ctx["contact_url"] = (*cfg).contact_url;
    ctx["contact_name"] = (*cfg).contact_name;

    return crow::response(200, page.render(ctx));
  }
}
