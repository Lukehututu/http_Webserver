#include "HttpServer.hpp"
#include "Database.hpp"

int main(int argc,char* argv[] ) {
    int port = 8080;
    if(argc > 1) {
        port = stoi(argv[1]);
    }
    Database db("user.db");
    HttpServer server(port,10,db);
    server.setupRoutes();
    server.start();
    return 0;
}