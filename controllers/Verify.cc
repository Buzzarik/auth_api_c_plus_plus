#include "Verify.h"
#include "../models/Tokens.h"
#include "../models/Users.h"
#include "lib.h"

// Add definition of your processing function here
bool Verify::check_parse_request(std::shared_ptr<Json::Value> json){
    return !json || !((*json)["token"].isString() && (*json)["id_api"].isInt64());
}

void Verify::errorResponse(const std::string& message, HttpStatusCode code, std::function<void (const HttpResponsePtr &)> &&callback){
        Json::Value res;
        res["error"] = message;
        auto resp = HttpResponse::newHttpJsonResponse(res);
        resp->setContentTypeCode(CT_APPLICATION_JSON);
        resp->setStatusCode(code);
        callback(resp);
}

void Verify::verify(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback) {
    using namespace orm; // для ORM
    using namespace drogon_model::postgres; // для Users и Tokens

    //TODO: достать token
    auto json_body_ptr = req->jsonObject();
    if (check_parse_request(json_body_ptr)){
        errorResponse("Invalid request payload", k400BadRequest, std::move(callback));
        LOG_INFO << "Invalid request payload in signup\n";
        return;
    }

    Verify::Input in {
        .hash_token = (*json_body_ptr)["token"].asString(),
        .id_api = (*json_body_ptr)["id_api"].asInt64()
    };

    if (in.hash_token == "") {
        errorResponse("token are required", k400BadRequest, std::move(callback));
        LOG_INFO << "token are required\n";
        return;
    }

    try{
        auto token = jwt::decode(in.hash_token);
        auto id_user = std::stoi(token.get_id());
        auto storage = app().getDbClient();
        Mapper<Tokens> mp(storage);
        mp.deleteBy(Criteria(Tokens::Cols::_expiry, CompareOperator::LE, trantor::Date::now()),
            [callback = callback, id_user = id_user, in = in](size_t records){
                LOG_INFO << "Delete " << records << " tokens\n";
                auto storage = app().getDbClient();
                Mapper<Tokens> mp(storage);
                mp.findOne(Criteria(Tokens::Cols::_id_api, CompareOperator::EQ, (int)in.id_api)
                    && Criteria(Tokens::Cols::_id_user, CompareOperator::EQ, id_user),
                    [callback = callback, in = in](Tokens find_token){
                        if (!lib::verify_token(in.hash_token, in.id_api, find_token)){
                            errorResponse("The token is not exists", k400BadRequest, AdviceCallback(callback));
                            LOG_INFO << "The token is not exists. Hash not valid\n";
                            LOG_DEBUG << "hash = " << in.hash_token  << " api = " << in.id_api
                                << "\nst hash = " << find_token.getValueOfHash() << " api = " << find_token.getValueOfIdApi() << "\n";
                            return;
                        }
                        Json::Value answer;
                        LOG_INFO << "token find by user = " << find_token.getValueOfIdUser()
                                << "id_api = " << find_token.getValueOfIdApi() << "\n";
                        answer["success"] = true;
                        answer["message"] = "Token find successfully";
                        auto resp = HttpResponse::newHttpJsonResponse(answer);
                        resp->setContentTypeCode(CT_APPLICATION_JSON);
                        resp->setStatusCode(k200OK);
                        callback(resp); 
                        return;
                    },
                    [callback = callback](const DrogonDbException& err){
                        const UnexpectedRows* unexpectedRows = dynamic_cast<const drogon::orm::UnexpectedRows*>(&err); //чтобы проверить, что бд нормально отработал
                        if (!unexpectedRows) { //произошла серьезная ошибка
                            errorResponse("Server internal error", k500InternalServerError, AdviceCallback(callback));
                            LOG_ERROR << "Failed get token from storage\n";
                            LOG_DEBUG << "Failed get token from storage\n" << err.base().what() << "\n";
                            return;
                        } 
                        errorResponse("The token is not exists", k400BadRequest, AdviceCallback(callback));
                        LOG_INFO << "The token is not exists. Token is not in storage\n";
                        return;
                    }
                );
            },
            [callback = callback](const DrogonDbException& err){
                errorResponse("Server internal error", k500InternalServerError, AdviceCallback(callback));
                LOG_ERROR << "Failed delete token from storage\n";
                LOG_DEBUG << "Failed delete token from storage\n" << err.base().what() << "\n";
                return;
            }
        );
        return;
    }
    catch(const std::exception& e){
        errorResponse("token is not valid", k400BadRequest, std::move(callback));
        LOG_INFO << "token is not Valid. WRONG decode\n";
        return;
    }
}