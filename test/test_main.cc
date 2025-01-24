#define DROGON_TEST_MAIN
#include <drogon/drogon_test.h>
#include <drogon/drogon.h>

//migrate -path=./migrations -database="postgres://postgres:mysecretpassword@localhost:5432/postgres?sslmode=disable" up
DROGON_TEST(LoginAndVerify)
{
    using namespace drogon;
    auto client = HttpClient::newHttpClient("http://localhost:5555");
    auto req = HttpRequest::newHttpRequest();
    req->setPath("/Login");
    req->setMethod(Post);
    req->setContentTypeCode(CT_APPLICATION_JSON);
    Json::Value message;
    message["phone_number"] = "8-999-999-99-99";
    message["password"] = "password";
    message["id_api"] = 1;
    Json::FastWriter writer;
    std::string str = writer.write(message);
    req->setBody(str);
    client->sendRequest(req, [TEST_CTX](ReqResult res, const HttpResponsePtr& resp) {
        REQUIRE(res == ReqResult::Ok);
        REQUIRE(resp != nullptr);
        CHECK(resp->statusCode() == k200OK);
        CHECK(resp->contentType() == CT_APPLICATION_JSON);
        auto json = resp->getJsonObject();
        REQUIRE(json != nullptr);
        REQUIRE((*json)["token"].isString());
        REQUIRE((*json)["id_user"].isInt());
        auto req = HttpRequest::newHttpRequest();
        req->setPath("/Verify");
        req->setMethod(Post);
        req->setContentTypeCode(CT_APPLICATION_JSON);
        Json::Value message;
        message["token"] = (*json)["token"].asString();
        message["id_api"] = 1;
        Json::FastWriter writer;
        std::string str = writer.write(message);
        req->setBody(str);
        auto client = HttpClient::newHttpClient("http://localhost:5555");
        client->sendRequest(req, [TEST_CTX](ReqResult res, const HttpResponsePtr& resp){
            REQUIRE(res == ReqResult::Ok);
            REQUIRE(resp != nullptr);
            CHECK(resp->statusCode() == k200OK);
        });
    });
}



int main(int argc, char** argv) 
{
    using namespace drogon;

    std::promise<void> p1;
    std::future<void> f1 = p1.get_future();

    // Start the main loop on another thread
    std::thread thr([&]() {
        // Queues the promise to be fulfilled after starting the loop
        app().getLoop()->queueInLoop([&p1]() { p1.set_value(); });
        app().run();
    });

    // The future is only satisfied after the event loop started
    f1.get();
    int status = test::run(argc, argv);

    // Ask the event loop to shutdown and wait
    app().getLoop()->queueInLoop([]() { app().quit(); });
    thr.join();
    return status;
}
