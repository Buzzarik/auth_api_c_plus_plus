#include "Signup.h"
#include <string>
#include <fmt/core.h>
#include <unordered_map>
#include "../models/Users.h"
#include "lib.h"

//TODO: потом выделить в отдельный файл, чтобы везде применять
void Signup::errorResponse(const std::string& message, HttpStatusCode code, std::function<void (const HttpResponsePtr &)> &&callback){
        Json::Value res;
        res["error"] = message;
        auto resp = HttpResponse::newHttpJsonResponse(res);
        resp->setContentTypeCode(CT_APPLICATION_JSON);
        resp->setStatusCode(code);
        callback(resp);
}

bool Signup::check_parse_request(std::shared_ptr<Json::Value> json){
    return !json || !((*json)["password"].isString() && (*json)["name"].isString() && (*json)["phone_number"].isString());
}

//ЗАПРОС
//"SET %s %s EX %d", in.phone_number.c_str(), json_str.c_str(), 5 * 60);

// Add definition of your processing function here
void Signup::signup(const HttpRequestPtr &req, std::function<void (const HttpResponsePtr &)> &&callback){
    using namespace orm; // для ORM
    using namespace drogon_model::postgres; // для Users
 
    auto json_body_ptr = req->jsonObject();

    if (check_parse_request(json_body_ptr)){
        errorResponse("Invalid request payload", k400BadRequest, std::move(callback));
        LOG_INFO << "Invalid request payload in signup\n";
        return;
    }

    Signup::Input in {
        .name = (*json_body_ptr)["name"].asString(),
        .password = (*json_body_ptr)["password"].asString(),
        .phone_number = (*json_body_ptr)["phone_number"].asString()
    };

    if (in.name == "" || in.password == "" || in.phone_number == ""){
        errorResponse("Name, phone and password are required", k400BadRequest, std::move(callback));
        LOG_INFO << "Name and phone and password number are required in signup\n";
        return;
    }

    auto storage = app().getDbClient();
    Mapper<Users> mp(storage);
    mp.findOne(orm::Criteria(Users::Cols::_phone_number, CompareOperator::EQ, in.phone_number),
        [callback = callback](Users user){
            //возвращаем ответ, что такой пользователь есть
            errorResponse("User already exists", HttpStatusCode::k409Conflict, AdviceCallback(callback));
            LOG_INFO << "User already exists in signup\n";
            return;
        },
        [callback = callback, in = in](const DrogonDbException& err){ //если количество строк больше одной или равно нулю, возникает исключение UnexpectedRows. Тоже проверить
            // определяем, что это другая ошибка, а не просто нет данных, как нам и надо
            const UnexpectedRows* unexpectedRows = dynamic_cast<const drogon::orm::UnexpectedRows*>(&err);
            if (!unexpectedRows) { //произошла серьезная ошибка
                errorResponse("Server internal error", k500InternalServerError, AdviceCallback(callback));
                LOG_ERROR << "Failed get user from storage\n";
                LOG_DEBUG << "Failed get user from storage\n" << err.base().what() << "\n";
                return;
            }
            try {
                std::string hash = lib::hashing(in.password); //могут вызвать исключения
                std::string otp = lib::generate_otp();

                Json::Value out;
                out["otp"] = otp;
                out["name"] = in.name;
                out["hash_password"] = hash;
                Json::FastWriter fastWriter;
                std::string json_str = fastWriter.write(out); //запись для кеша

                auto cache = app().getRedisClient();
                //сделать запрос со временем 5 мин
                cache->execCommandAsync([callback = callback, otp = otp](const drogon::nosql::RedisResult &r){
                        Json::Value answer;
                        LOG_DEBUG << "otp: " << otp << "\n"; //TODO: отправка например на почту
                        answer["success"] = true;
                        answer["message"] = "OTP sent successfully";
                        auto resp = HttpResponse::newHttpJsonResponse(answer);
                        resp->setContentTypeCode(CT_APPLICATION_JSON);
                        resp->setStatusCode(k200OK);
                        callback(resp);
                        return;
                    },
                    [callback = callback](const std::exception &err){
                        LOG_ERROR << "cache set failed\n";
                        LOG_DEBUG << err.what() << "\n";
                        errorResponse("Server internal error", k500InternalServerError, AdviceCallback(callback));
                    },
                    "SET %s %s EX %d", in.phone_number.c_str(), json_str.c_str(), 5 * 60
                );
                return;
            }
            catch (const std::exception& e){
                errorResponse("Server internal error", k500InternalServerError, AdviceCallback(callback));
                LOG_ERROR << "Failed generate hash or otp\n";
                LOG_DEBUG << "Failed generate hash or otp\n" << e.what() << "\n";
                return;
            }
        }
    );
}