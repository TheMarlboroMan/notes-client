#include <notes_client/client.h>
#include <lm/ostream_logger.h>
#include <iostream>
#include <string>

int main(int argc, char ** argv) {

	if(3!=argc) {

		std::cout<<"notes-client username pass"<<std::endl;
		return 0;
	}
	
	try {

		lm::ostream_logger logger(std::cout);
		notes_client::client client("http://notes.dpastor.com/api", logger);

		std::string username{argv[1]}, pass{argv[2]};

//		std::string username, pass;
//		std::cout<<"username: ";
//		std::getline(std::cin, username);
//		std::cout<<"pass: ";
//		std::getline(std::cin, pass);

		client.login(username, pass);
		client.get_notes();

		return 0;
	}
	catch(std::exception &e) {

		std::cerr<<"error: "<<e.what()<<std::endl;
		return 1;
	}
}
