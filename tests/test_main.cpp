#include <gtest/gtest.h>
#include <atomic>
#include "http_request.h"
#include <thread_pool.h>

TEST(HttpParserTest, ParseValidStartLine){
    std::string rawReq = "GET /about HTTP/1.1\r\n"
                         "Host: localhost\r\n\r\n";
    HttpRequest req = parseHttpRequest(rawReq);

    EXPECT_EQ(req.method, "GET");
    EXPECT_EQ(req.path, "/about");
    EXPECT_EQ(req.version, "HTTP/1.1");
}

TEST(HttpParseTest, ParseValidHeader){
    std::string rawReq = "GET / HTTP/1.1\r\n"
                         "Host: localhost:8080\r\n"
                         "Content-Type: text/html\r\n"
                         "Connection:     keep-alive\r\n\r\n";
    HttpRequest req = parseHttpRequest(rawReq);

    EXPECT_EQ(req.headers["Connection"], "keep-alive");
    EXPECT_EQ(req.headers["Content-Type"], "text/html");
}

TEST(HttpParseTest, ParseEmptyRequest){
    std::string rawReq = "";
    HttpRequest req = parseHttpRequest(rawReq);

    EXPECT_TRUE(req.method.empty());
    EXPECT_TRUE(req.path.empty());
    EXPECT_TRUE(req.version.empty());
    EXPECT_TRUE(req.headers.empty());
}

TEST(ThreadPoolTest, ExecuteTasks){
    std::atomic<int> counter{0};
    int repeat = 100;
    {
        ThreadPool pool(4);
        for (int i = 0; i < repeat; i++){
            pool.enqueue([&counter]{counter++;});
        }
    }

    EXPECT_EQ(counter.load(), repeat);
}
