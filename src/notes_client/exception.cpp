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

new_note_exception::new_note_exception(
):
	std::runtime_error("failed to create a new note, please, check the log")
{

}

delete_note_exception::delete_note_exception(
):
	std::runtime_error("failed to delete note, please, check the log")
{

}

no_note_exception::no_note_exception(
	int _note_id
):
	std::runtime_error("")
{

	std::stringstream ss;
	ss<<"could not find note with id "<<_note_id;
	err=ss.str();
}
