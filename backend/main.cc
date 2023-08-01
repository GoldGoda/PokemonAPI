#include <drogon/drogon.h>

using namespace drogon;


void ADD_CORS_SUPPORT()
{
    static constexpr const char* ORIGIN_ALLOWED = "http://localhost:5500";
    
    app().registerPostRoutingAdvice([]( const HttpRequestPtr &req, AdviceCallback &&callback, AdviceChainCallback &&acb)
    {
        if (req->method() == drogon::HttpMethod::Options)
        {
            auto &origin = req->getHeader("Origin");
            auto response = drogon::HttpResponse::newHttpResponse();
            
            response->addHeader("Access-Control-Allow-Origin", "http://localhost:5500");
            response->addHeader("Access-Control-Allow-Methods", "GET,OPTIONS,PUT,POST,DELETE");
            response->addHeader("Access-Control-Allow-Headers", "*");
            response->addHeader("Access-Control-Allow-Credentials","true");  
            callback(response);
        }
        else { acb(); }
    }); 

    app().registerPreSendingAdvice( [](const HttpRequestPtr &req, const HttpResponsePtr & response)
    {
        if (req->method() != drogon::HttpMethod::Options)
        {
            response->addHeader("Access-Control-Allow-Origin", ORIGIN_ALLOWED);
            response->addHeader("Access-Control-Allow-Methods", "GET,OPTIONS,PUT,POST,DELETE");
            response->addHeader("Access-Control-Allow-Headers", "*");
            response->addHeader("Access-Control-Allow-Credentials", "true");
        }
    });

}

int main() {
    //Set HTTP listener address and port
    ADD_CORS_SUPPORT();

    app().registerHandler("/", 
    [](const HttpRequestPtr& request, std::function<void (const HttpResponsePtr&)> &&callback)
    {
        auto client = HttpClient::newHttpClient("https://pokeapi.co");
        auto req = HttpRequest::newHttpRequest();
        auto pokemonRangeInitIterator = request->parameters().find("offset");
        auto pokemonRangeInit = (pokemonRangeInitIterator != request->parameters().end() ?
                                        pokemonRangeInitIterator->second : "0" );
        std::string path = "/api/v2/pokemon/?offset=";
        path += pokemonRangeInit;
        path += "&limit=20";
        req->setPath(path);
        req->setMethod(drogon::Get);

        client->sendRequest( req, [&, callback, session = request->session() ](ReqResult result, const HttpResponsePtr &response){
            
            //En caso de que la request no se pueda enviar
            if (result != ReqResult::Ok)
            {
                std::cout
                    << "error while sending request to server! result: "
                    << result << std::endl;
                    response->setStatusCode(k401Unauthorized);
                return;
            }

            auto jsonPtr = response->getJsonObject();
            if(jsonPtr){
                auto json = (*jsonPtr);
                Json::Value responseJson;
                auto pokemonList  = json["results"];
                auto pokemonCount = json["count"].as<std::string>();
                
                responseJson["count"] = pokemonCount;
                Json::Value subJson;
                int tmpCounter {0};
                for(const auto& pokemon : pokemonList){
                    subJson[tmpCounter++]["name"] = pokemon["name"];
                }
                responseJson["pokemons"] = subJson; 
                callback( HttpResponse::newHttpJsonResponse(responseJson) );
            }
            else{
                response->setStatusCode(k401Unauthorized);
                callback(response);
            }
        });
    });


    app().registerHandler("/pokemon/{1}", 
    [](const HttpRequestPtr& request, std::function<void (const HttpResponsePtr&)> &&callback, std::string&& pokemonId)
    {
        auto response = HttpResponse::newHttpResponse();
    
        if( !request->session()->find("isLogged") ){
            response->setStatusCode(k401Unauthorized);
            callback(response);
            return;
        }

        auto client = HttpClient::newHttpClient("https://pokeapi.co");
        auto req = HttpRequest::newHttpRequest();
        std::string path = "/api/v2/pokemon/";
        path += pokemonId;
        req->setPath(path);
        req->setMethod(drogon::Get);

        client->sendRequest( req, [&, callback, session = request->session() ](ReqResult result, const HttpResponsePtr &cResponse){
            
            //En caso de que la request no se pueda enviar
            if (result != ReqResult::Ok)
            {
                std::cout
                    << "error while sending request to server! result: "
                    << result << std::endl;
                return;
            }

            auto jsonPtr = cResponse->getJsonObject();
            if(jsonPtr){
                auto json = (*jsonPtr);
                Json::Value responseJson;

                responseJson["name"]  = json["name"].as<std::string>();
                responseJson["img"]   = json["sprites"]["front_default"].as<std::string>();
                auto pokemonAbilities  = json["abilities"];
                int tmpCounter {0};
                Json::Value childJson;
                for(const auto& pokemon : pokemonAbilities){
                    childJson[tmpCounter++]["name"] = pokemon["ability"]["name"];
                }
                responseJson["abilities"] = childJson;
                callback( HttpResponse::newHttpJsonResponse(responseJson) );
            }
            else{
                response->setStatusCode(k401Unauthorized);
                callback(response);
            }
        });
    }, {Get, Options});


    app().registerHandler("/login", 
    [](const HttpRequestPtr& request, std::function<void (const HttpResponsePtr&)> &&callback)
    {
        auto response = HttpResponse::newHttpResponse();
        auto jsonPtr = request->getJsonObject();
        if(jsonPtr){
            auto json     = (*jsonPtr);
            auto user     = json["user"].as<std::string>();
            auto password = json["password"].as<std::string>();
        
            app().getDbClient()->execSqlAsync("select * FROM users WHERE user=? AND password=?", 
            [user, password, response, callback, session = request->session() ](const orm::Result& slqResult)
            {
                auto userExists = slqResult.size() ? true : false;
                if(userExists)
                {
                    session->insert("isLogged", true);
                    callback(response);
                }
                else
                {
                    response->setStatusCode(k401Unauthorized);
                    callback(response);
                }
            },
            [response, callback](const drogon::orm::DrogonDbException& exc){
                response->setStatusCode(k401Unauthorized);
                callback(response);
            }, user, password);
        }
        else {
            response->setStatusCode(k401Unauthorized);
            callback(response);
        }
    }, {Post, Options});


    app().registerHandler("/register", 
    [](const HttpRequestPtr& request, std::function<void (const HttpResponsePtr&)> &&callback)
    {
        auto response = HttpResponse::newHttpResponse();
        auto jsonPtr = request->getJsonObject();
        if(jsonPtr){
            auto json     = (*jsonPtr);
            auto user     = json["user"].as<std::string>();
            auto password = json["password"].as<std::string>();
        
            app().getDbClient()->execSqlAsync("INSERT INTO users(user,password) VALUES (?,?)", 
            [user, password, response, callback, session = request->session() ](const orm::Result& slqResult)
            {
                auto userExists = slqResult.affectedRows() ? true : false;
                if(userExists)
                {
                    session->insert("isLogged", true);
                    response->setStatusCode(k200OK);
                    callback(response);
                }
                else
                {
                    response->setStatusCode(k401Unauthorized);
                    callback(response);
                }
            },
            [response, callback](const drogon::orm::DrogonDbException& exc){
                response->setStatusCode(k401Unauthorized);
                callback(response);
            }, user, password);
        }
        else {
            response->setStatusCode(k401Unauthorized);
            callback(response);
        }
    }, {Post, Options});

    app().createDbClient("mysql", "192.168.0.10", 3306, "pokemon", "wsl_root","");
    app().addListener("127.0.0.1", 2001).enableSession(1200, drogon::Cookie::SameSite::kNone).run();


    //Run HTTP framework,the method will block in the internal event loop
    drogon::app().run();
    return 0;
}
