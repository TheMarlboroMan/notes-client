#include <notes_client/exception.h>
#include <sstream>

using namespace notes_client;

login_exception::login_exception(
	long _status_code,
	const std::string& _body
):
	std::runtime_error("")
{

	std::stringstream ss;
	ss<<"login failed with status code '"<<_status_code<<"', body was '"<<_body<<"'";
	err=ss.str();
}

no_token_exception::no_token_exception()
	:std::runtime_error("could not retrieve auth token from response") 
{

}

unauthorized_exception::unauthorized_exception(
	const std::string& _err
):
	std::runtime_error(std::string{"got unauthorized response @"}+_err)
{

}

server_exception::server_exception(
	const std::string& _err
):
	std::runtime_error(std::string{"got server error @"}+_err)
{

}
