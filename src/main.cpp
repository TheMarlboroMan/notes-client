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
void new_note(notes_client::client&, notes_client::librarian&, lm::logger&, const std::string&);
void delete_note(notes_client::client&, notes_client::librarian&, lm::logger&, int);
void read_note(notes_client::librarian&, notes_client::note_formatter&, int);
void edit_note(notes_client::client&, notes_client::librarian&, lm::logger&, int, const std::string&);
std::string open_editor(const std::string&, const std::string&, lm::logger&);
std::string get_uri(int, char **, const appenv::env&);

int main(int argc, char ** argv) {
	
	//The uri is optional in case we already provided one...
	if(argc < 3 || argc > 4) {

		std::cout<<"notes-client username pass [uri]"<<std::endl;
		return 0;
	}
	
	const bool no_editor=nullptr==getenv("EDITOR");

	std::string editor=no_editor
		? "vim"
		: getenv("EDITOR");

	if(no_editor) {

		std::cout<<"error: EDITOR environment variable is not set, assuming vim"<<std::endl;
	}

	try {

		appenv::env env{".notes-client", nullptr};
		env.create_user_dir();

		lm::file_logger logger(env.build_user_path("notes-client.log").c_str());

		std::string uri=get_uri(argc, argv, env);
		lm::log(logger).info()<<"using '"<<uri<<"' as server uri"<<std::endl;

		notes_client::client client(uri, logger);

		std::string library_file{env.build_user_path("library.json")};
		notes_client::librarian librarian(library_file, logger);

		std::string username{argv[1]}, pass{argv[2]};

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
					new_note(client, librarian, logger, editor);
					continue;
				case notes_client::user_input::commands::edit:
					edit_note(client, librarian, logger, input.id, editor);
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

std::string open_editor(
	const std::string& _editor,
	const std::string& _contents,
	lm::logger& _logger
) {

	char tmp_template[]="/tmp/notes-client.XXXXXX";
	mkstemp(tmp_template);
	const std::string tmp_file(tmp_template);

	if(_contents.size()) {
	
		std::ofstream f{tmp_template};
		f<<_contents;
		f.close();
	}

	auto pid=fork();
	if(0==pid) {

		lm::log(_logger).info()<<"launching external editor into "<<tmp_file<<std::endl;

		char *cmd[]={
			const_cast<char *>(_editor.c_str()),
			const_cast<char *>(tmp_file.c_str()), 
			nullptr
		};

		auto result=execvp(_editor.c_str(), cmd);
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
			return "";
		}

		std::string contents=tools::dump_file(tmp_file);
		tools::trim(contents);
		tools::filesystem::remove(tmp_file);
		if(!contents.size()) {

			lm::log(_logger).info()<<"editor process saved empty file, returning"<<std::endl;
			return "";
		}

		return contents;
	}

	if(WIFSIGNALED(childflags)) {

		lm::log(_logger).notice()<<"editor process was terminated by signal "<<WTERMSIG(childflags)<<std::endl;
		return "";
	}

	lm::log(_logger).notice()<<"editor process was neither finished, nor terminated. Go figure it out."<<std::endl;
	return "";
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
	lm::logger& _logger,
	const std::string& _editor
) {

	const std::string contents=open_editor(_editor, "", _logger);

	if(!contents.size()) {

		return;
	}

	_client.create_note(contents);
	sync(_client, _librarian);
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
	int _note_id,
	const std::string& _editor
) {

	if(!_librarian.has_note(_note_id)) {

		std::cout<<"no note by such id ("<<_note_id<<") exists"<<std::endl;
		return;
	}

	const auto note=_librarian.get_note(_note_id);
	const std::string contents=open_editor(_editor, note.contents, _logger);

	if(!contents.size()) {

		return;
	}

	_client.patch_note(_note_id, contents);
	sync(_client, _librarian);
}

std::string get_uri(
	int argc, 
	char ** argv, 
	const appenv::env& _env
) {
	
	const std::string uri_file{_env.build_user_path("last_server")};
	if(argc==4) {

		std::string result{argv[3]};
	
		//store in the last server file...
		std::ofstream of{uri_file};
		of<<result;
		
		return result;
	}

	if(!tools::filesystem::exists(uri_file)) {

		throw std::runtime_error("unable to locate last_server file, please. specify a uri at startup");
	}

	return tools::dump_file(uri_file);
}
