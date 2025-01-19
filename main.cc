#include <drogon/drogon.h>



int main() {
    //Set HTTP listener address and port
    drogon::app().loadConfigFile("../config.yaml");
    // drogon::app().addListener("0.0.0.0", 5555);
    //Load config file
    //drogon::app().loadConfigFile("../config.json");
    //drogon::app().loadConfigFile("../config.yaml");
    //Run HTTP framework,the method will block in the internal event loop
    
    // drogon::app().registerHandler("/num_users",
    //     [](drogon::HttpRequestPtr req) -> Task<drogon::HttpResponsePtr>)
    //     //          Now returning a response ^^^
    // {
    //     auto sql = app().getDbClient();
    //     try
    //     {
    //         auto result = co_await sql->execSqlCoro("SELECT COUNT(*) FROM users;");
    //         size_t num_users = result[0][0].as<size_t>();
    //         auto resp = drogon::HttpResponse::newHttpResponse();
    //         resp->setBody(std::to_string(num_users));
    //         co_return resp;
    //     }
    //     catch(const DrogonDbException &err)
    //     {
    //         // Exception works as sync interfaces.
    //         auto resp = drogon::HttpResponse::newHttpResponse();
    //         resp->setBody(err.base().what());
    //         co_return resp;
    //     }
    // }

    drogon::app().run();
    
    return 0;
}

