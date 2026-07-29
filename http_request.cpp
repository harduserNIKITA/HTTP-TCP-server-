#include "http_request.h"
#include <sstream>

HttpRequest parseHttpRequest(const std::string& rawRequest){
	HttpRequest req;
	std::string startLine;
	std::istringstream streamRequest(rawRequest);

	if (std::getline(streamRequest, startLine)){
		if (!startLine.empty() && startLine.back() == '\r'){
			startLine.pop_back();
		}
		std::istringstream line(startLine);
		line >> req.method >> req.path >> req.version;
	}

	std::string header;
	while (std::getline(streamRequest, header)){
		if (!header.empty() && header.back() == '\r'){
			header.pop_back();
		}
		if (header.empty()){
			break;
		}

		size_t delimeterPos = header.find(':');
		if (delimeterPos != std::string::npos){
			std::string key = header.substr(0, delimeterPos);
			std::string value = header.substr(delimeterPos + 1);
			size_t start = value.find_first_not_of(" \t");
                	if (start != std::string::npos){
                        	value = value.substr(start);
                	}
                	req.headers[key] = value;
		}
	}

	return req;
}
