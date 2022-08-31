#include <notes_client/client.h>
#include <notes_client/librarian.h>
#include <lm/ostream_logger.h>
#include <lm/log.h>
#include <tools/string_utils.h>
#include <tools/file_utils.h>
#include <iostream>
#include <string>
#include <sys/wait.h>
#include <unistd.h>

void sync(notes_client::client&, notes_client::librarian&);
void new_note(notes_client::client&, notes_client::librarian&, lm::logger&);

int main(int argc, char ** argv) {

	if(3!=argc) {

		std::cout<<"notes-client username pass"<<std::endl;
		return 0;
	}
	
	if(nullptr==getenv("EDITOR")) {

		std::cerr<<"error: EDITOR environment variable is not set"<<std::endl;
		return 1;
	}

	try {

		lm::ostream_logger logger(std::cout);
		notes_client::client client("http://notes.dpastor.com/api", logger);
		notes_client::librarian librarian("library.json", logger);

		std::string username{argv[1]}, pass{argv[2]};

//		std::string username, pass;
//		std::cout<<"username: ";
//		std::getline(std::cin, username);
//		std::cout<<"pass: ";
//		std::getline(std::cin, pass);

		//there will be no offline workflow and stuff, let's keep this simple
		client.login(username, pass);
		sync(client, librarian);
		
		bool running=true;
		while(running) {

			for(const auto& card : librarian.get_cards()) {

				//TODO: This should be a formatter class.
				std::string contents=card.contents.substr(0, 80);
				tools::replace(contents, "\n", " ");
				tools::replace(contents, "\r", " ");

				std::cout<<"["<<card.id<<"] created at "<<card.created_at<<": "<<contents<<std::endl;
			}

			std::cout<<std::endl<<"new, edit [n], delete [n], display [n], sync, exit"<<std::endl<<">> ";

			std::string input{};
			std::getline(std::cin, input);

			tools::trim(input);

			if(input=="exit") {

				running=false;
				continue;
			}
			else if(input=="sync") {

				sync(client, librarian);
			}
			else if(input=="new") {

				new_note(client, librarian, logger);
			}
		}

		return 0;
	}
	catch(std::exception &e) {

		std::cerr<<"error: "<<e.what()<<std::endl;
		return 1;
	}
}

void sync(
	notes_client::client& _client,
	notes_client::librarian& _librarian
) {

	const auto notes=_client.get_notes();
	_librarian.make_index(notes);
}

void new_note(
	notes_client::client& _client,
	notes_client::librarian& _librarian,
	lm::logger& _logger
) {

	auto pid=fork();
	const std::string tmp_file="/tmp/notes-client.tmp";

	if(0==pid) {

		lm::log(_logger).info()<<"launching external editor..."<<std::endl;

		char *cmd[]={"$EDITOR", const_cast<char *>(tmp_file.c_str()), nullptr};
		auto result=execvp(getenv("EDITOR"), cmd);
		std::cerr<<"ERRNO: "<<errno<<" for result "<<result;
		exit(0);
	}

	int childflags=0;
	int options=0;
	waitpid(pid, &childflags, options);

	if(WIFEXITED(childflags)) {

		lm::log(_logger).info()<<"editor process terminated with "<<WEXITSTATUS(childflags)<<" status"<<std::endl;

		if(!tools::filesystem::exists(tmp_file)) {

			lm::log(_logger).info()<<"editor process did not save any file, returning"<<std::endl;
			return;
		}

		std::string contents=tools::dump_file(tmp_file);
		tools::trim(contents);
		if(!contents.size()) {

			lm::log(_logger).info()<<"editor process saved empty file, returning"<<std::endl;
			return;
		}

		//TODO: _client.post(contents);

		sync(_client, _librarian);
		return;
	}

	if(WIFSIGNALED(childflags)) {

		lm::log(_logger).notice()<<"editor process was terminated by signal "<<WTERMSIG(childflags)<<std::endl;
		return;
	}

	lm::log(_logger).notice()<<"editor process was neither finished, nor terminated. Go figure it out."<<std::endl;
}
