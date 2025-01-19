#pragma once

#include <drogon/HttpController.h>

using namespace drogon;

class Signup : public drogon::HttpController<Signup>
{
  public:
    struct Input {
      std::string name;
      std::string password;
      std::string phone_number;
    };

    METHOD_LIST_BEGIN
    // use METHOD_ADD to add your custom processing function here;
    METHOD_ADD(Signup::signup, "", Post);
    // METHOD_ADD(Signup::get, "/{2}/{1}", Get); // path is /Signup/{arg2}/{arg1}
    // METHOD_ADD(Signup::your_method_name, "/{1}/{2}/list", Get); // path is /Signup/{arg1}/{arg2}/list
    // ADD_METHOD_TO(Signup::your_method_name, "/absolute/path/{1}/{2}/list", Get); // path is /absolute/path/{arg1}/{arg2}/list

    METHOD_LIST_END

    void signup(const HttpRequestPtr &req, std::function<void (const HttpResponsePtr &)> &&callback) ;
    // your declaration of processing function maybe like this:
    // void get(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, int p1, std::string p2);
    // void your_method_name(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, double p1, int p2) const;
    static bool check_parse_request(std::shared_ptr<Json::Value> json);
    static void errorResponse(const std::string& message, HttpStatusCode code, std::function<void (const HttpResponsePtr &)> &&callback);
};
