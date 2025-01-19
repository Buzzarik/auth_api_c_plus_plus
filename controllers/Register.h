#pragma once

#include <drogon/HttpController.h>

using namespace drogon;

class Register : public drogon::HttpController<Register>
{
  public:
    struct Input {
        std::string otp;
        std::string phone_number;
    };

    METHOD_LIST_BEGIN
    // use METHOD_ADD to add your custom processing function here;
    METHOD_ADD(Register::verifyopt, "", Post); // path is /Register/{arg2}/{arg1}
    // METHOD_ADD(Register::your_method_name, "/{1}/{2}/list", Get); // path is /Register/{arg1}/{arg2}/list
    // ADD_METHOD_TO(Register::your_method_name, "/absolute/path/{1}/{2}/list", Get); // path is /absolute/path/{arg1}/{arg2}/list

    METHOD_LIST_END
    // your declaration of processing function maybe like this:
    // void get(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, int p1, std::string p2);
    // void your_method_name(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, double p1, int p2) const;
    void verifyopt(const HttpRequestPtr &req, std::function<void (const HttpResponsePtr &)> &&callback);
    static bool check_parse_request(std::shared_ptr<Json::Value> json);
    static void errorResponse(const std::string& message, HttpStatusCode code, std::function<void (const HttpResponsePtr &)> &&callback);
};
