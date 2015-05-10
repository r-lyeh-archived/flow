flow <a href="https://travis-ci.org/r-lyeh/flow"><img src="https://api.travis-ci.org/r-lyeh/flow.svg?branch=master" align="right" /></a>
====

- Flow is a lightweight network downloader with native fallbacks. Aimed to gamedev.
- Flow is non-blocking friendly. Support for sync/async requests.
- Flow is tiny. Single header and source files.
- Flow is zlib/libpng licensed.

### sample
```c++
#include <cassert>
#include <iostream>
#include <string>

#include "flow.hpp"

int main( int argc, const char **argv ) {
    auto prompt = []() {
        std::string prompt;
        while( prompt.empty() ) {
            std::cout << "url> ";
            std::getline( std::cin, prompt );
        }
        return prompt;
    };

    for( ;; ) {
        std::string url;
        url = prompt();

        // sync test
        flow::status status = flow::download(url);
        std::cout << "sync: "<< status.url << ";ok=" << status.ok << "," << status.data.size() << " bytes;" << status.code << std::endl;

        // async test
        flow::download_async(url, []( flow::status &status ) {
            std::cout << "async: "<< status.url << ";ok=" << status.ok << "," << status.data.size() << " bytes;" << status.code << std::endl;
        } );
    }

        return 0;
}
```

