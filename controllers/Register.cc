#include "Register.h"
#include "../models/Users.h"

//TODO: потом выделить в отдельный файл, чтобы везде применять
void Register::errorResponse(const std::string& message, HttpStatusCode code, std::function<void (const HttpResponsePtr &)> &&callback){
        Json::Value res;
        res["error"] = message;
        auto resp = HttpResponse::newHttpJsonResponse(res);
        resp->setContentTypeCode(CT_APPLICATION_JSON);
        resp->setStatusCode(code);
        callback(resp);
}

//TODO: выделить в /lib
bool verify(const std::string& hash, const std::string& password){
    return true;
}

// Add definition of your processing function here
bool Register::check_parse_request(std::shared_ptr<Json::Value> json) {
    return !json || !((*json)["otp"].isString() && (*json)["phone_number"].isString());
}

void Register::verifyopt(const HttpRequestPtr &req, std::function<void (const HttpResponsePtr &)> &&callback){
    using namespace orm; // для ORM
    using namespace drogon_model::postgres; // для Users и Tokens

    auto json_body_ptr = req->jsonObject();
    if (check_parse_request(json_body_ptr)){
        errorResponse("Invalid request payload", k400BadRequest, std::move(callback));
        LOG_INFO << "Invalid request payload in signup\n";
        return;
    }

    Register::Input in {
        .otp = (*json_body_ptr)["otp"].asString(),
        .phone_number = (*json_body_ptr)["phone_number"].asString()
    };

    if (in.otp == "" || in.phone_number == ""){
        errorResponse("Name and phone and password number are required", k400BadRequest, std::move(callback));
        LOG_INFO << "Name and phone number are required in signup\n";
        return;
    }

    auto cache = app().getRedisClient();
    cache->execCommandAsync([callback = callback, in = in](const drogon::nosql::RedisResult &r){
            //походу сюда попадает то, что значит не найден по ключу isNill
            if (r.isNil()) {
                LOG_INFO << "cache get failed\n";
                errorResponse("OTP is not valid or Expiry", k400BadRequest, AdviceCallback(callback));
                return;
            }
            Json::Value user_data;
            Json::Reader reader;
            bool parsingSuccessful = reader.parse(r.asString(), user_data);
            if (!parsingSuccessful){
                LOG_ERROR << "failed get after cache parse json\n";
                errorResponse("Server internal error", k500InternalServerError, AdviceCallback(callback));
                return;
            }

            if (user_data["otp"].asString() != in.otp){
                LOG_INFO << "Otp is not verify\n";
                errorResponse("Otp is not verify", k400BadRequest, AdviceCallback(callback));
                return;
            }

            Users user;
            user.setHashPassword(user_data["hash_password"].asString());
            user.setName(user_data["name"].asString());
            user.setPhoneNumber(in.phone_number);
            auto storage = app().getDbClient();
            Mapper<Users> mp(storage);
            mp.insert(user, [callback = callback](Users insert_user){
                    Json::Value answer;
                    LOG_INFO << "user created by id = " << insert_user.getValueOfId() << "\n";
                    answer["success"] = true;
                    answer["message"] = "User registered successfully";
                    answer["name"] = insert_user.getValueOfName();
                    answer["phone_number"] = insert_user.getValueOfPhoneNumber();
                    auto resp = HttpResponse::newHttpJsonResponse(answer);
                    resp->setContentTypeCode(CT_APPLICATION_JSON);
                    resp->setStatusCode(k201Created);
                    callback(resp);

                    return;
                },
                [callback = callback](const DrogonDbException& err){
                    //как понять, что CONSTRAINT (но я не уверен, что именно Failure это делает)
                    const Failure* constraint = dynamic_cast<const drogon::orm::Failure*>(&err);
                    if (constraint){
                        errorResponse("User with the phone_number already exists", k400BadRequest, AdviceCallback(callback));
                        LOG_INFO << "Failed set user from storage or user is exists\n";
                        return;
                    }
                    errorResponse("Server internal error", k500InternalServerError, AdviceCallback(callback));
                    LOG_ERROR << "Failed set user from storage or user is exists\n";
                    LOG_DEBUG << "Failed set user from storage or user is exists\n" << err.base().what() << "\n";
                    return;
                }
            );
            return;
        },
        [callback = callback](const std::exception &err){
            LOG_ERROR << "cache get failed\n";
            LOG_DEBUG << err.what() << "\n";
            errorResponse("Server internal error", k500InternalServerError, AdviceCallback(callback));
            return;
        },
        "get %s", in.phone_number.c_str()
    );

    return;
}