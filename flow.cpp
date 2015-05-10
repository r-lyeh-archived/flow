// Flow is a lightweight http downloader
// - rlyeh, zlib/libpng licensed

#include <string.h>
#include <functional>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#if defined(_WIN32)
#   include <winsock2.h>
#   include <wininet.h>
#   pragma comment(lib,"wininet.lib")
#   define $windows(...) __VA_ARGS__
#   define $apple(...)
#   define $posix(...)
#elif defined(__APPLE__)
#   define $windows(...)
#   define $apple(...) __VA_ARGS__
#   define $posix(...)
#else
#   define $windows(...)
#   define $apple(...)
#   define $posix(...) __VA_ARGS__
#endif

#include "flow.hpp"

namespace {
    std::string url_encode(std::string str) {
        struct detail { static bool isOrdinaryChar(char c) {
            char ch = tolower(c);
            if(ch == 'a' || ch == 'b' || ch == 'c' || ch == 'd' || ch == 'e'
                || ch == 'f' || ch == 'g' || ch == 'h' || ch == 'i' || ch == 'j'
                || ch == 'k' || ch == 'l' || ch == 'm' || ch == 'n' || ch == 'o'
                || ch == 'p' || ch == 'q' || ch == 'r' || ch == 's' || ch == 't'
                || ch == 'u' || ch == 'v' || ch == 'w' || ch == 'x' || ch == 'y'
                || ch == 'z' || ch == '0' || ch == '1' || ch == '2' || ch == '3'
                || ch == '4' || ch == '5' || ch == '6' || ch == '7' || ch == '8'
                || ch == '9') {
                return true;
            }
            return false;
        } };
        int len = str.length();
        char* buff = new char[len + 1];
        strcpy(buff,str.c_str());
        std::string ret = "";
        for(int i=0;i<len;i++) {
            if(detail::isOrdinaryChar(buff[i])) {
                ret = ret + buff[i];
            }else if(buff[i] == ' ') {
                ret = ret + "+";
            }else {
                char tmp[6];
                sprintf(tmp,"%%%x",buff[i]);
                ret = ret + tmp;
            }
        }
        delete[] buff;
        return ret;
    }

    std::string url_decode(std::string str) {
        struct detail {
            static void getAsDec(char* hex) {
                char tmp = tolower(hex[0]);
                if(tmp == 'a') {
                    strcpy(hex,"10");
                }else if(tmp == 'b') {
                    strcpy(hex,"11");
                }else if(tmp == 'c') {
                    strcpy(hex,"12");
                }else if(tmp == 'd') {
                    strcpy(hex,"13");
                }else if(tmp == 'e') {
                    strcpy(hex,"14");
                }else if(tmp == 'f') {
                    strcpy(hex,"15");
                }else if(tmp == 'g') {
                    strcpy(hex,"16");
                }
            }
            static int convertToDec(const char* hex) {
                char buff[12];
                sprintf(buff,"%s",hex);
                int ret = 0;
                int len = strlen(buff);
                for(int i=0;i<len;i++) {
                    char tmp[4];
                    tmp[0] = buff[i];
                    tmp[1] = '\0';
                    getAsDec(tmp);
                    int tmp_i = atoi(tmp);
                    int rs = 1;
                    for(int j=i;j<(len-1);j++) {
                        rs *= 16;
                    }
                    ret += (rs * tmp_i);
                }
                return ret;
            }
        };
        int len = str.length();
        char* buff = new char[len + 1];
        strcpy(buff,str.c_str());
        std::string ret = "";
        for(int i=0;i<len;i++) {
            if(buff[i] == '+') {
                ret = ret + " ";
            }else if(buff[i] == '%') {
                char tmp[4];
                char hex[4];
                hex[0] = buff[++i];
                hex[1] = buff[++i];
                hex[2] = '\0';
                //int hex_i = atoi(hex);
                sprintf(tmp,"%c",detail::convertToDec(hex));
                ret = ret + tmp;
            }else {
                ret = ret + buff[i];
            }
        }
        delete[] buff;
        return ret;
    }
}

namespace
{
    enum config {
        verbose = false
    };

    class http {
    private:

            $windows(
                HINTERNET m_hSession;
                HINTERNET m_hRequest;
            )

            // disable copy & assignment
            http( const http &other );
            http &operator =( const http &other );

    public:

        std::map<std::string,std::string> vars;
        std::string host, path, response, error;

        http() {
            $windows(
                m_hSession = 0;
                m_hRequest = 0;
            )
        }

        ~http() {
            $windows(
                if( m_hSession != NULL ) InternetCloseHandle(m_hSession);
                if( m_hRequest != NULL ) InternetCloseHandle(m_hRequest);
            )
        }

        bool connect();
        bool send();
    };

    bool http::connect() {
        std::string form_action = host + path;

        if( vars.size() ) {
            form_action += "?";
            for( std::map<std::string,std::string>::iterator it = vars.begin(); it != vars.end(); ++it )
                form_action += url_encode(it->first) + "=" + url_encode(it->second) + "&";
        }

        $windows(
            m_hSession = InternetOpenA("request 1",
                                    PRE_CONFIG_INTERNET_ACCESS,
                                    NULL,
                                    INTERNET_INVALID_PORT_NUMBER,
                                    0);
            if( m_hSession == NULL)
            {
                std::stringstream ss;
                ss << GetLastError();
                error = std::string("@InternetOpen()") + ss.str();
                return false;
            }

            m_hRequest = InternetOpenUrlA(m_hSession,
                                    form_action.c_str(),
                                    NULL,
                                    0,
                                    INTERNET_FLAG_RELOAD,
                                    0);
            if( m_hRequest == NULL )
            {
                std::stringstream ss;
                ss << GetLastError();
                error = std::string("@InternetOpenUrl()") + ss.str();
                return false;
            }
        )

        return error = std::string(), true;
    }

    bool http::send() {
        response = std::string();

        $windows(
            if( error = "No request made to server", m_hRequest == NULL )
                return false;

            DWORD lBytesRead = 0, bufsz = 512 * 1024 ;
            std::vector<char> buff( bufsz );
            bool ok;
            while( ( ok = (InternetReadFile(m_hRequest, &buff[0], bufsz, &lBytesRead) != FALSE) ) && lBytesRead > 0 ) {
                if( lBytesRead > bufsz ) {
                    return error = "Buffer overflow", false;
                } else {
                    response += std::string( &buff[0], lBytesRead );
                }
            }

            if( ok && !lBytesRead ) {
                return true;
            }
        )

        $apple(
        )

        return error = "no idea :)", false;
    }

    bool downloader( const std::string &url_, std::string &code, std::string &data ) {
        try {
#           define $(x) do { if( verbose ) std::cout << #x << ":" << x << std::endl; } while(0)

            std::string url = url_;
            code = std::string("undefined error");
            data = std::string();

            if( url.size() < 8 )
                return false;

            int idx = url.find_first_of("/");
            if( idx == std::string::npos ) url = "http://" + url_;

            $(url);
            $(idx);

            std::string protocol = url.substr(0, idx + 2), domain = url.substr(idx + 2);

            $(protocol);
            $(domain);

            http req;

            req.host = domain.substr( 0, domain.find_first_of("/") );
            req.path = req.host != domain ? domain.substr( domain.find_first_of("/") ) : std::string("/");

#           ifdef _WIN32
                //
                req.host = protocol + req.host;
#           endif

            $(req.path);
            $(req.host);

            if( verbose ) {
                std::cout << "[" << protocol << "] - connecting to " << req.host << "..." << std::endl;
            }

            if( req.connect() ) {
                if( verbose ) {
                    std::cout << "[" << protocol << "] - sending request " << req.path << std::endl;
                }

                if( req.send() ) {
                    if( req.response.size() ) {
                        data = req.response;
                        code = req.error;
                        return true;
                    }
                }
            }

            return false;
        }
        catch(std::exception &e) {
            code = e.what();
            return false;
        }
        catch(...) {
            code = std::string("exception thrown");
            return false;
        }
    }
}


namespace flow {

    status download( const std::string &url, const callback &good, const callback &fail ) {
        status st { url };
        st.ok = downloader(st.url, st.code, st.data);
        if( st.ok ) {
            if( good ) good( st );
        } else {
            st.data.clear();
            if( fail ) fail( st );
        }
        return st;
    }

    void download_async( const std::string &url, const callback &good, const callback &fail ) {
        std::thread( [=] {
            download( url, good, fail );
        } ).detach();
    }
}


#undef $windows
#undef $posix
#undef $apple
