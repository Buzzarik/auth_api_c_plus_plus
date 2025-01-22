#include "lib.h"
#include <iostream>

namespace lib{
    std::unique_ptr<drogon_model::postgres::Tokens> create_token(const drogon_model::postgres::Users& user, const std::string& secret, int ttl_token){
        try{
            trantor::Date d = trantor::Date::now();

            //TODO: сделать в виде конфига
            d = d.after(ttl_token * 6).roundSecond(); //NOTE: поставил на 60 сек

            
            jwt::date date(std::chrono::seconds(d.secondsSinceEpoch()));

            auto create_at = user.getValueOfCreatedAt();
            auto str = create_at.toCustomFormattedString("%Y.%m.%d %H:%M:%S");
            auto hash_token = jwt::create()
                .set_type("JWT")
                .set_algorithm("hs256")
                .set_payload_claim("phone_number", jwt::claim(user.getValueOfPhoneNumber()))
                .set_payload_claim("name", jwt::claim(user.getValueOfName()))
                .set_payload_claim("created_at", jwt::claim(str))
                .set_id(std::to_string(user.getValueOfId()))
                .set_expires_at(date)
                .sign(jwt::algorithm::hs256{secret});
            
            auto token = std::make_unique<drogon_model::postgres::Tokens>();
            token->setExpiry(d);
            token->setHash(hash_token);
            return token;
        }
        catch(const std::exception& e){
            return {};
        }
    } 

//TODO: потом нормальный генератор создать
    std::string generate_otp(){
        int number = rand() % 10000;
        return std::format("{:0{}}", number, 4);
    }

//TODO: потом нормальный хещ сделать
    std::string hashing(const std::string& password){
        return std::to_string(std::hash<std::string>{}(password));
    }

    bool verify_password(const std::string& hash, const std::string& password){
        return hash == hashing(password);
    }

    bool verify_token(const std::string& hash_token, int64_t id_api, const drogon_model::postgres::Tokens& token){
        return hash_token == token.getValueOfHash() && id_api == token.getValueOfIdApi();
    }
}