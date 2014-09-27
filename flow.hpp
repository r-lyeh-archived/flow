// Flow is a lightweight http downloader
// - rlyeh, BOOST licensed

#pragma once

#include <string>
#include <functional>

namespace flow
{
    struct status {
        std::string url;
        std::string code;
        std::string data;
        bool ok;
    };

    using callback = std::function<void(status &)>;
    
    // sync api
    status download( const std::string &url, const callback &good = callback(), const callback &fail = callback() );

    // async api
    void download_async( const std::string &url, const callback &good = callback(), const callback &fail = callback() );
}
