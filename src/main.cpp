#include <notes_client/client.h>
#include <notes_client/librarian.h>
#include <lm/ostream_logger.h>
#include <lm/log.h>
#include <tools/string_utils.h>
#include <tools/file_utils.h>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/wait.h>
#include <unistd.h>

void sync(notes_client::client&, notes_client::librarian&);
void new_note(notes_client::client&, notes_client::librarian&, lm::logger&);
void delete_note(notes_client::client&, notes_client::librarian&, lm::logger&, int);
void read_note(notes_client::librarian&, int);
void edit_note(notes_client::client&, notes_client::librarian&, lm::logger&, int);

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

			std::cout<<std::endl<<"(n)ew, (e)dit [n], (d)elete [n], (r)ead [n], (s)ync, e(x)it"<<std::endl<<">> ";

			std::string input{}, command{};
			std::getline(std::cin, input);
			std::stringstream ss{input};

			ss>>command;

			auto get_note_id=[&ss]() -> int {

				int note_id=0;
				ss>>note_id;
				if(ss.fail() || note_id <=0 ) {

					std::cout<<"bad input, use edit [number] where [number] is the note id"<<std::endl;
					return 0;
				}
				
				return note_id;
			};

			if(command=="exit" || command=="quit" || command == "q" || command == "x") {

				running=false;
				continue;
			}
			else if(command=="sync" || command=="s") {

				sync(client, librarian);
			}
			else if(command=="new" || command=="n") {

				new_note(client, librarian, logger);
			}
			else if(command=="edit" || command=="e") {

				int note_id=get_note_id();
				if(!note_id) {

					continue;
				}

				edit_note(client, librarian, logger, note_id);
			}
			else if(command=="delete" || command=="d") {

				int note_id=get_note_id();
				if(!note_id) {

					continue;
				}
				delete_note(client, librarian, logger, note_id);
			}
			else if(command=="read" || command=="r") {

				int note_id=get_note_id();
				if(!note_id) {

					continue;
				}

				read_note(librarian, note_id);
			}
			else {

				std::cout<<"unrecognised command '"<<input<<"'"<<std::endl;
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
	_librarian.read_index();
}

void new_note(
	notes_client::client& _client,
	notes_client::librarian& _librarian,
	lm::logger& _logger
) {

	char tmp_template[]="/tmp/notes-client.XXXXXX";
	mkstemp(tmp_template);
	const std::string tmp_file(tmp_template);

	auto pid=fork();
	if(0==pid) {

		lm::log(_logger).info()<<"launching external editor into "<<tmp_file<<std::endl;

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
		tools::filesystem::remove(tmp_file);
		if(!contents.size()) {

			lm::log(_logger).info()<<"editor process saved empty file, returning"<<std::endl;
			return;
		}

		_client.create_note(contents);
		sync(_client, _librarian);
		return;
	}

	if(WIFSIGNALED(childflags)) {

		lm::log(_logger).notice()<<"editor process was terminated by signal "<<WTERMSIG(childflags)<<std::endl;
		return;
	}

	lm::log(_logger).notice()<<"editor process was neither finished, nor terminated. Go figure it out."<<std::endl;
}

void delete_note(
	notes_client::client& _client, 
	notes_client::librarian& _librarian,
	lm::logger& _logger,
	int _note_id
) {

	//even if it is poor form to check on this very method, let us do so.
	if(!_librarian.has_note(_note_id)) {

		std::cout<<"no note by such id ("<<_note_id<<") exists"<<std::endl;
		return;
	}
	
	_client.delete_note(_note_id);
	sync(_client, _librarian);
	lm::log(_logger).info()<<"deleted note with id "<<_note_id<<std::endl;
}

void read_note(
	notes_client::librarian& _librarian,
	int _note_id
) {

	if(!_librarian.has_note(_note_id)) {

		std::cout<<"no note by such id ("<<_note_id<<") exists"<<std::endl;
		return;
	}

	const auto& note=_librarian.get_note(_note_id);
	std::cout<<note.contents<<std::endl;
}


void edit_note(
	notes_client::client& _client,
	notes_client::librarian& _librarian,
	lm::logger& _logger,
	int _note_id
) {

	if(!_librarian.has_note(_note_id)) {

		std::cout<<"no note by such id ("<<_note_id<<") exists"<<std::endl;
		return;
	}

	const auto note=_librarian.get_note(_note_id);
	char tmp_template[]="/tmp/notes-client.XXXXXX";
	mkstemp(tmp_template);
	const std::string tmp_file(tmp_template);
	
	std::ofstream f{tmp_template};
	f<<note.contents;
	f.close();

	auto pid=fork();
	if(0==pid) {

		lm::log(_logger).info()<<"launching external editor into "<<tmp_file<<std::endl;

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
		tools::filesystem::remove(tmp_file);
		if(!contents.size()) {

			lm::log(_logger).info()<<"editor process saved empty file, returning"<<std::endl;
			return;
		}

		_client.patch_note(_note_id, contents);
		sync(_client, _librarian);
		return;
	}

	if(WIFSIGNALED(childflags)) {

		lm::log(_logger).notice()<<"editor process was terminated by signal "<<WTERMSIG(childflags)<<std::endl;
		return;
	}

	lm::log(_logger).notice()<<"editor process was neither finished, nor terminated. Go figure it out."<<std::endl;
}

