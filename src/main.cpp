#include <notes_client/client.h>
#include <notes_client/librarian.h>
#include <notes_client/out_formatter.h>
#include <notes_client/cli_input.h>
#include <lm/file_logger.h>
#include <lm/log.h>
#include <tools/string_utils.h>
#include <tools/file_utils.h>
#include <appenv/env.h>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <memory>

void sync(notes_client::client&, notes_client::librarian&);
void new_note(notes_client::client&, notes_client::librarian&, lm::logger&);
void delete_note(notes_client::client&, notes_client::librarian&, lm::logger&, int);
void read_note(notes_client::librarian&, notes_client::note_formatter&, int);
void edit_note(notes_client::client&, notes_client::librarian&, lm::logger&, int);

int main(int argc, char ** argv) {

	if(4!=argc) {

		std::cout<<"notes-client uri username pass"<<std::endl;
		return 0;
	}
	
	if(nullptr==getenv("EDITOR")) {

		std::cerr<<"error: EDITOR environment variable is not set"<<std::endl;
		return 1;
	}

	try {

		appenv::env env{".notes-client", nullptr};
		env.create_user_dir();

		lm::file_logger logger(env.build_user_path("notes-client.log").c_str());
		notes_client::client client(argv[1], logger);
		notes_client::librarian librarian("library.json", logger);

		std::string username{argv[2]}, pass{argv[3]};

		//there will be no offline workflow and stuff, let's keep this simple
		client.login(username, pass);
		sync(client, librarian);
		
		std::unique_ptr<notes_client::note_formatter> formatter{new notes_client::out_formatter{}};
		notes_client::cli_input in{};

		while(true) {

			for(const auto& card : librarian.get_cards()) {

				formatter->format_note_card(card, std::cout);
			}

			in.show_prompt();

			auto input=in.get_input();
			switch(input.command) {

				case notes_client::user_input::commands::exit:
					goto cleanup; //lol
				case notes_client::user_input::commands::sync:
					sync(client, librarian);
					continue;
				case notes_client::user_input::commands::create:
					new_note(client, librarian, logger);
					continue;
				case notes_client::user_input::commands::edit:
					edit_note(client, librarian, logger, input.id);
					continue;
				case notes_client::user_input::commands::remove:
					delete_note(client, librarian, logger, input.id);
					continue;
				case notes_client::user_input::commands::read:
					read_note(librarian, *(formatter.get()), input.id);
					continue;
				case notes_client::user_input::commands::error:
					std::cout<<"unrecognised command '"<<input.error<<"'"<<std::endl;
					continue;
			}
		}

		cleanup: //lol
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

	//TODO: This repeats...
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
	notes_client::note_formatter& _formatter,
	int _note_id
) {

	if(!_librarian.has_note(_note_id)) {

		std::cout<<"no note by such id ("<<_note_id<<") exists"<<std::endl;
		return;
	}

	const auto& note=_librarian.get_note(_note_id);
	_formatter.format_note_full(note, std::cout);
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

