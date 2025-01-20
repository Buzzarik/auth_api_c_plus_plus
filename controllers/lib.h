#pragma once

#include <jwt-cpp/jwt.h>
#include <string>
#include "../models/Tokens.h"
#include "../models/Users.h"
#include <memory>
#include <sstream>
#include <iomanip>
#include <unordered_map>

namespace lib{
    std::chrono::system_clock::time_point parseDateTime(const std::string& dateTimeString);

    std::unique_ptr<drogon_model::postgres::Tokens> create_token(const drogon_model::postgres::Users& user, const std::string& secret, int ttl_token);

    bool verify_token(const std::string& hash_token, int64_t id_api, const drogon_model::postgres::Tokens& token);

    std::string generate_otp();

    std::string hashing(const std::string& password);  

    bool verify_password(const std::string& hash, const std::string& password); 
}