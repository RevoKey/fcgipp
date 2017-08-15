#include "Helper.h"
#include "RequestHandler.h"

RequestHandler::RequestHandler()
{
}

RequestHandler::~RequestHandler()
{
}

void RequestHandler::Handle(RequestPtr request)
{
	// to do

	// example
	request->ParseHeader();
	request->ParseBody();
	Json::Value response;
	response["hello"] = "world";
	ResponseJson(request, response);
}