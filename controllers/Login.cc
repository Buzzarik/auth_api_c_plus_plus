#include "Login.h"
#include "../models/Tokens.h"
#include "../models/Users.h"
#include "lib.h"

// Add definition of your processing function here
bool Login::check_parse_request(std::shared_ptr<Json::Value> json) {
    return !json || !((*json)["phone_number"].isString() && (*json)["password"].isString() && (*json)["id_api"].isInt64());
}

void Login::errorResponse(const std::string& message, HttpStatusCode code, std::function<void (const HttpResponsePtr &)> &&callback){
        Json::Value res;
        res["error"] = message;
        auto resp = HttpResponse::newHttpJsonResponse(res);
        resp->setContentTypeCode(CT_APPLICATION_JSON);
        resp->setStatusCode(code);
        callback(resp);
}

void Login::login(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback){
    using namespace orm; // для ORM
    using namespace drogon_model::postgres; // для Users и Tokens

    auto json_body_ptr = req->jsonObject();
    if (check_parse_request(json_body_ptr)){
        errorResponse("Invalid request payload", k400BadRequest, std::move(callback));
        LOG_INFO << "Invalid request payload in signup\n";
        return;
    }

    Login::Input in {
        .password = (*json_body_ptr)["password"].asString(),
        .phone_number = (*json_body_ptr)["phone_number"].asString(),
        .id_api = (*json_body_ptr)["id_api"].asInt64()
    };

    if (in.password == "" || in.phone_number == ""){
        errorResponse("Password and phone and password number are required", k400BadRequest, std::move(callback));
        LOG_INFO << "Password and phone number are required in login\n";
        return;
    }

    auto storage = app().getDbClient();
    Mapper<Users> mp(storage);
    mp.findOne(Criteria(Users::Cols::_phone_number, CompareOperator::EQ, in.phone_number),
        [callback = callback, in = in](Users user){
            std::string hash_password = user.getValueOfHashPassword();
            if (!lib::verify_password(hash_password, in.password)){
                errorResponse("Wrong password or phone", k400BadRequest, AdviceCallback(callback));
                LOG_INFO << "Wrong password\n";
                return; 
            }
            //TODO: добавить в конфиг секретное слово + ttl для токена и его использовать
            auto token_ptr = lib::create_token(user, "secret", 10);
            if (!token_ptr){
                errorResponse("Server internal error", k500InternalServerError, AdviceCallback(callback));
                LOG_ERROR << "Failed created token\n";
                return;
            }
            token_ptr->setIdApi(in.id_api);
            token_ptr->setIdUser(user.getValueOfId());

            //TODO: залить токен в Postgres
            {
            auto storage = app().getDbClient();
            Mapper<Tokens> mp(storage);
            LOG_DEBUG << "TOKEN = " << token_ptr->getValueOfHash() << "\n";
            mp.insert(*token_ptr, 
                [callback = callback](Tokens insert_token){
                  Json::Value answer;
                    LOG_INFO << "token created by user = " << insert_token.getValueOfIdUser()
                            << "id_api = " << insert_token.getValueOfIdApi() << "\n";
                    answer["success"] = true;
                    answer["message"] = "Token created successfully";
                    answer["token"] = insert_token.getValueOfHash();
                    auto resp = HttpResponse::newHttpJsonResponse(answer);
                    resp->setContentTypeCode(CT_APPLICATION_JSON);
                    resp->setStatusCode(k200OK);
                    callback(resp);
                    return;
                },
                [callback = callback, user = user, in = in](const DrogonDbException& err){
                    //как понять, что CONSTRAINT (но я не уверен, что именно Failure это делает)
                    const Failure* constraint = dynamic_cast<const drogon::orm::Failure*>(&err);
                    if (!constraint){
                        errorResponse("Server internal error", k500InternalServerError, AdviceCallback(callback));
                        LOG_ERROR << "Failed set token in Storage\n";
                        LOG_DEBUG << "Failed set token in Storage\n" << err.base().what() << "\n";
                        return;
                    }
                    //делаем запрос на старый токен
                    //BUG: не понятно, как избежать rollback, если не удалось создать запись :(
                    //BUG: может быть такой случай, когда токен удалиться (сработает триггер) и мы не получим старый токен
                    //BUG: по сути ничего страшного, после выдаст ошибку и надо будет заново зайти, но не приятно
                    auto storage = app().getDbClient();
                    Mapper<Tokens> mp(storage);
                    mp.findOne(Criteria(Tokens::Cols::_id_user, CompareOperator::EQ, (int)user.getValueOfId()) 
                                && Criteria(Tokens::Cols::_id_api, CompareOperator::EQ, (int)in.id_api),
                        [callback = callback](Tokens find_token){
                                Json::Value answer;
                                LOG_INFO << "token find by user = " << find_token.getValueOfIdUser()
                                        << "id_api = " << find_token.getValueOfIdApi() << "\n";
                                answer["success"] = true;
                                answer["message"] = "Token find successfully";
                                answer["token"] = find_token.getValueOfHash();
                                auto resp = HttpResponse::newHttpJsonResponse(answer);
                                resp->setContentTypeCode(CT_APPLICATION_JSON);
                                resp->setStatusCode(k200OK);
                                callback(resp); 
                                return;
                        },
                        [callback = callback](const DrogonDbException &err){
                            errorResponse("Server internal error", k500InternalServerError, AdviceCallback(callback));
                            LOG_ERROR << "Failed get token from Storage\n";
                            LOG_DEBUG << "Failed get token from Storage\n" << err.base().what() << "\n";
                            return;
                        }
                    );
                    return;
                }
            );
            return;
            }
        },
        [callback = callback, in = in](const DrogonDbException& err){
            const UnexpectedRows* unexpectedRows = dynamic_cast<const drogon::orm::UnexpectedRows*>(&err); //чтобы проверить, что бд нормально отработал
            if (!unexpectedRows) { //произошла серьезная ошибка
                errorResponse("Server internal error", k500InternalServerError, AdviceCallback(callback));
                LOG_ERROR << "Failed get user from storage\n";
                LOG_DEBUG << "Failed get user from storage\n" << err.base().what() << "\n";
                return;
            }
            errorResponse("Wrong password or phone", k400BadRequest, AdviceCallback(callback));
            LOG_INFO << "Wrong phone\n";
            return; 
        }
    );
}
