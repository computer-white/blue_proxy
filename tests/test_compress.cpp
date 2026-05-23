#include <iostream>
#include "http/httpParser.h"

int main()
{
    std::string raw = "Hello World, this is a test for gzip compression check";
    auto req = std::make_shared<blue::http::HttpRequestParser>();
    std::string zipped = req->compress(raw);
    std::cout << "zipped size: " << zipped.size() << std::endl;
    std::string unzipped = req->decompress(zipped);
    std::cout << "unzipped size: " << unzipped.size() << std::endl;
    std::cout << "match: " << (raw == unzipped) << std::endl;
    return 0;
}