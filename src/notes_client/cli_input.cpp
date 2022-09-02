#include <notes_client/cli_input.h>
#include <sstream>
#include <iostream>

using namespace notes_client;

void cli_input::show_prompt() const {

	std::cout<<std::endl<<std::endl<<"(n)ew, (e)dit [n], (d)elete [n], (r)ead [n], (s)ync, e(x)it"<<std::endl<<">> ";
}

user_input cli_input::get_input() const {

	std::string input{}, command{};
	std::getline(std::cin, input);
	std::stringstream ss{input};
	ss>>command;

	auto get_note_id=[&ss]() -> int {

		int note_id=0;
		ss>>note_id;
		if(ss.fail() || note_id <=0 ) {

			return 0;
		}
				
		return note_id;
	};

	if(command=="exit" || command=="quit" || command == "q" || command == "x") {

		return user_input{user_input::commands::exit, 0, ""};
	}

	if(command=="sync" || command=="s") {

		return user_input{user_input::commands::sync, 0, ""};
	}

	if(command=="new" || command=="n") {

		return user_input{user_input::commands::create, 0, ""};
	}

	if(command=="edit" || command=="e") {

		int note_id=get_note_id();
		if(!note_id) {

			return user_input{user_input::commands::error, 0, "bad input, use edit [number] where [number] is the note id"};
		}

		return user_input{user_input::commands::edit, note_id, ""};
	}

	if(command=="delete" || command=="d") {

		int note_id=get_note_id();
		if(!note_id) {

			return user_input{user_input::commands::error, 0, "bad input, use delete [number] where [number] is the note id"};
		}

		return user_input{user_input::commands::remove, note_id, ""};
	}

	if(command=="read" || command=="r") {

		int note_id=get_note_id();
		if(!note_id) {

			return user_input{user_input::commands::error, 0, "bad input, use read [number] where [number] is the note id"};
		}

		return user_input{user_input::commands::read, note_id, ""};
	}

	ss.str("");
	ss<<"unrecognised command '"<<input<<"'";
	return user_input{user_input::commands::error, 0, ss.str()}; 
}

