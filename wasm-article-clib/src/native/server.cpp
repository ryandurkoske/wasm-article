
extern "C" {
  	#include "running_total.h"
	#include "rng.h"
}

#include <stdlib.h>
#include <string_view>

/* We simply call the root header file "App.h", giving you uWS::App and uWS::SSLApp */
#include "App.h"

/* This is a simple WebSocket "sync" upgrade example.
 * You may compile it with "WITH_OPENSSL=1 make" or with "make" */

int main() {
    /* ws->getUserData returns one of these */
    struct PerSocketData {
        /* Define your user data */
        int32_t running_total;
		int32_t* chunk;
		rng_t* rng;
    };

    /* Keep in mind that uWS::SSLApp({options}) is the same as uWS::App() when compiled without SSL support.
     * You may swap to using uWS:App() if you don't need SSL */
    uWS::App()
	.ws<PerSocketData>("/*", {
        /* Settings */
        .compression = uWS::CompressOptions::DISABLED,
        .maxPayloadLength = 16 * 1024,
        .idleTimeout = 30,
        .maxBackpressure = 8 * 1024,
		.closeOnBackpressureLimit = true,
		.resetIdleTimeoutOnSend = true,
		.sendPingsAutomatically = false,
        /* Handlers */
        .upgrade = [](auto *res, auto *req, auto *context) {

            /* You may read from req only here, and COPY whatever you need into your PerSocketData.
             * PerSocketData is valid from .open to .close event, accessed with ws->getUserData().
             * HttpRequest (req) is ONLY valid in this very callback, so any data you will need later
             * has to be COPIED into PerSocketData here. */

			uint32_t seed = 412412412;

            /* Immediately upgrading without doing anything "async" before, is simple */
            res->template upgrade<PerSocketData>({
                /* We initialize PerSocketData struct here */
                .running_total = 0,
				.chunk = (int32_t*)malloc(sizeof(int32_t)*CHUNK_SIZE),
				.rng = rng_alloc(seed)
            }, req->getHeader("sec-websocket-key"),
                req->getHeader("sec-websocket-protocol"),
                req->getHeader("sec-websocket-extensions"),
                context);

            /* If you don't want to upgrade you can instead respond with custom HTTP here,
             * such as res->writeStatus(...)->writeHeader(...)->end(...); or similar.*/

            /* Performing async upgrade, such as checking with a database is a little more complex;
             * see UpgradeAsync example instead. */
        },
        .open = [](auto *ws) {
            /* Open event here, you may access ws->getUserData() which points to a PerSocketData struct.
             * Here we simply validate that indeed, something == 13 as set in upgrade handler. */

            std::cout << "Running Total is: " << static_cast<PerSocketData *>(ws->getUserData())->running_total << std::endl;
        },
        .message = [](auto *ws, std::string_view message, uWS::OpCode opCode) {
			if(opCode==uWS::OpCode::BINARY){
				
				if(message.length() == sizeof(int32_t)){
					PerSocketData* data = ws->getUserData();
					int32_t sum = *(int32_t*)message.data();
					int32_t total = data->running_total;
					
					if(sum == total){
						
						int32_t new_sum = generate_chunk(data->chunk, data->rng);
						data->running_total += new_sum;
						
						ws->send(std::string_view((char*)data->chunk, sizeof(int32_t)*CHUNK_SIZE));

						std::cout << "Running Total is: " << data->running_total << std::endl;

					}else{
						ws->end(1000, "Running total desync.");
					}

				}else{
					ws->end(4003, "Bad packet (wrong size).");
				}
			}else{
				ws->end(1003, "Binary only endpoint.");
			}
        },
        .drain = [](auto */*ws*/) {
            /* Check ws->getBufferedAmount() here */
        },
        .ping = [](auto */*ws*/, std::string_view) {
            /* You don't need to handle this one, we automatically respond to pings as per standard */
        },
        .pong = [](auto */*ws*/, std::string_view) {
            /* You don't need to handle this one either */
        },
        .close = [](auto */*ws*/, int /*code*/, std::string_view /*message*/) {
            /* You may access ws->getUserData() here, but sending or
             * doing any kind of I/O with the socket is not valid. */

			 std::cout << "Connection closed" << std::endl;
        }
    }).listen(9001, [](auto *listen_socket) {
        if (listen_socket) {
            std::cout << "Listening on port " << 9001 << std::endl;
        }
    }).run();
}
