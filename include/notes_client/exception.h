#pragma once
#include <stdexcept>

namespace notes_client {

class login_exception:
	public std::runtime_error {

	public:

	            login_exception(long, const std::string&);
	const char * what() const noexcept {return err.c_str();}

	private:

	std::string err;
};

class no_token_exception:
	public std::runtime_error {

	public:
	            no_token_exception();
};

class unauthorized_exception:
	public std::runtime_error {

	public:
	            unauthorized_exception(const std::string&);
};

class server_exception:
	public std::runtime_error {

	public:
	            server_exception(const std::string&);
};
}


